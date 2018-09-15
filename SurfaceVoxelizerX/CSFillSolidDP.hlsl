//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

#define _TEST_	0

cbuffer cbPerMipLevel
{
	float g_fGridSize;
};

Texture3D<uint>			g_txKBufDepth;
RWTexture3D<min16float>	g_RWGrid;

//--------------------------------------------------------------------------------------
// Fill solid voxels
//--------------------------------------------------------------------------------------
[numthreads(32, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	min16float4 vData;
	//vData.x = g_RWGrid[0][DTid];
	//vData.y = g_RWGrid[1][DTid];
	//vData.z = g_RWGrid[2][DTid];
	//vData.w = g_RWGrid[3][DTid];
	vData.w = g_RWGrid[DTid];

	if (vData.w <= 0.0)
	{
		const uint uNumLayer = g_fGridSize * DEPTH_SCALE;

		bool bFill = false;
		uint uDepth, uDepthPrev = 0xffffffff;

#if _TEST_
		//[allow_uav_condition]
		for (uint i = 1; i < 32; ++i)
		{
			const uint3 vTex = { DTid.xy, i };
			const uint uDepth = g_txKBufDepth[vTex];

			if (uDepth == 0xffffffff) break;
		}

		uDepthPrev = g_txKBufDepth[uint3(DTid.xy, 0)];
		uDepth = g_txKBufDepth[uint3(DTid.xy, i - 1)];

		bFill = uDepthPrev != 0xffffffff && uDepthPrev < DTid.z;
		bFill = bFill && (uDepth != 0xffffffff && uDepth > DTid.z);
#else
		//[allow_uav_condition]
		for (uint i = 0; i < uNumLayer; ++i)
		{
			const uint3 vTex = { DTid.xy, i };
			uDepth = g_txKBufDepth[vTex];

			if (uDepth == 0xffffffff)
			{
				bFill = false;
				break;
			}
			if (uDepth > DTid.z) break;
			bFill = uDepthPrev == uDepth - 1 ? bFill : !bFill;
			uDepthPrev = uDepth;
		}
#endif

		if (bFill)
		{
			g_RWGrid[DTid] = 1.0;
		}
	}
}
