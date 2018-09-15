//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

cbuffer cbPerMipLevel
{
	float g_fGridSize;
};

Texture3D<min16float>	g_txGrid[3];
Texture2DArray<uint>	g_txKBufDepth;
RWTexture3D<min16float>	g_RWGrid;

//--------------------------------------------------------------------------------------
// Fill solid voxels
//--------------------------------------------------------------------------------------
[numthreads(32, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	min16float4 vData;
	vData.x = g_txGrid[0][DTid];
	vData.y = g_txGrid[1][DTid];
	vData.z = g_txGrid[2][DTid];
	vData.w = any(vData.xyz);

	if (vData.w <= 0.0)
	{
		const uint uNumLayer = g_fGridSize * DEPTH_SCALE;

		bool bFill = false;
		uint uDepthBeg = 0xffffffff, uDepthEnd;

		for (uint i = 0; i < uNumLayer; ++i)
		{
			const uint3 vTex = { DTid.xy, i };
			uDepthEnd = g_txKBufDepth[vTex];
#ifdef _USE_NORMAL_
			if (uDepthEnd > DTid.z) break;
#else
			if (uDepthEnd == 0xffffffff)
			{
				bFill = false;
				break;
			}
			if (uDepthEnd > DTid.z) break;
			bFill = uDepthBeg == uDepthEnd - 1 ? bFill : !bFill;
#endif
			uDepthBeg = uDepthEnd;
		}

#ifdef _USE_NORMAL_
		if (uDepthBeg != 0xffffffff && uDepthEnd != 0xffffffff)
		{
			const min16float vNormBegZ = g_txGrid[2][uint3(DTid.xy, uDepthBeg)];
			const min16float vNormEndZ = g_txGrid[2][uint3(DTid.xy, uDepthEnd)];
			bFill = vNormBegZ < 0.0 || vNormEndZ > 0.0;
		}
#endif

		if (bFill)
		{
			g_RWGrid[DTid] = 1.0;
		}
	}
}
