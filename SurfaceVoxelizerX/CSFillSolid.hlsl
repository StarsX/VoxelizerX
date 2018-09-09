//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

#if USING_SRV
Texture3D<min16float>	g_txGrid[4];
#else
RWTexture3D<min16float>	g_txGrid[3];
#endif
RWTexture3D<min16float>	g_RWGrid[4];

groupshared min16float4	g_Block[2][2][2];

//--------------------------------------------------------------------------------------
// Up sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	min16float4 vData;
	vData.x = g_RWGrid[0][DTid];
	vData.y = g_RWGrid[1][DTid];
	vData.z = g_RWGrid[2][DTid];
	vData.w = any(vData.xyz) ? 1.0 : 0.0;

#if 0
	/*if (vData.w <= 0.0)
	{
		uint3 vLoc = DTid;
		for (uint i = 0; i < 2; ++i)
		{
			vLoc >>= 1;
			vData.w = g_txGrid[3].mips[i][vLoc];

			if (vData.w > 0.0) break;
		}

		if (vData.w > 0.0)
		{
			vData.x = g_txGrid[0].mips[i][vLoc];
			vData.y = g_txGrid[1].mips[i][vLoc];
			vData.z = g_txGrid[2].mips[i][vLoc];

			const float3 vPos = DTid + 0.5;
			const float3 vPosSurf = vLoc << (i + 1) + (1 << i);

			min16float3 vDir = min16float3(normalize(vPosSurf - vPos));
			vDir.y = -vDir.y;

			const min16float3 vNorm = normalize(vData.xyz);
			if (dot(vNorm, vDir) > 0.0)
			{
				g_RWGrid[0][DTid] = vData.x;
				g_RWGrid[1][DTid] = vData.y;
				g_RWGrid[2][DTid] = vData.z;
				g_RWGrid[3][DTid] = vData.w;
			}
		}
	}*/
	bool bWrite = false;
	if (vData.w <= 0.0)
	{
		float3 vDir = normalize(DTid - 32.0);
		for (uint i = 1; i < 16; ++i)
		{
			const uint3 vLoc = Gid + round(vDir * i);
			vData.x = g_txGrid[0][vLoc];
			vData.y = g_txGrid[1][vLoc];
			vData.z = g_txGrid[2][vLoc];
			vData.w = g_txGrid[3][vLoc];

			if (vData.w > 0.0)
			{
				float3 vDirW = { vDir.x, -vDir.y, vDir.z };
				const min16float3 vNorm = normalize(vData.xyz);
				if (dot(vNorm, vDirW) > 0.0) bWrite = true;
				break;
			}
		}
	}

	DeviceMemoryBarrierWithGroupSync();
	if (bWrite)
	{
		g_RWGrid[0][DTid] = vData.x;
		g_RWGrid[1][DTid] = vData.y;
		g_RWGrid[2][DTid] = vData.z;
		g_RWGrid[3][DTid] = vData.w;
	}

	uint3 vLoc = DTid >> 2;
	g_RWGrid[0][DTid] = g_txGrid[0].mips[1][vLoc];
	g_RWGrid[1][DTid] = g_txGrid[1].mips[1][vLoc];
	g_RWGrid[2][DTid] = g_txGrid[2].mips[1][vLoc];
	g_RWGrid[3][DTid] = g_txGrid[3].mips[1][vLoc];

#else
	min16float4 vDataIn;
	vDataIn.x = g_txGrid[0][Gid];
	vDataIn.y = g_txGrid[1][Gid];
	vDataIn.z = g_txGrid[2][Gid];
	vDataIn.w = any(vDataIn.xyz) ? 1.0 : 0.0;

	g_Block[GTid.x][GTid.y][GTid.z] = vData;
	GroupMemoryBarrierWithGroupSync();

	if (vDataIn.w > 0.0 && vData.w <= 0.0)
	{
		//const min16float3 vNormAvg = normalize(vDataIn.xyz);
		bool bWrite = true;

		for (uint i = 0; i < 2 && bWrite; ++i)
		{
			for (uint j = 0; j < 2 && bWrite; ++j)
			{
				for (uint k = 0; k < 2 && bWrite; ++k)
				{
					if (g_Block[i][j][k].w > 0.0)
					{
						const float3 vPos = GTid;
						const float3 vPosSurf = float3(i, j, k);

						min16float3 vDir = min16float3(normalize(vPosSurf - vPos));
						vDir.y = -vDir.y;

						const min16float3 vNorm = normalize(g_Block[i][j][k].xyz);
						if (dot(vNorm, vDir) <= 0.0) bWrite = false;
					}
				}
			}
		}

		if (bWrite)
		{
			g_RWGrid[0][DTid] = vDataIn.x;
			g_RWGrid[1][DTid] = vDataIn.y;
			g_RWGrid[2][DTid] = vDataIn.z;
			g_RWGrid[3][DTid] = vDataIn.w;
		}
	}
#endif
}
