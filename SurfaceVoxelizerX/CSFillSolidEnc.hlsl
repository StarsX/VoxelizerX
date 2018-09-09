//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

RWTexture3D<uint>		g_txGridVal;
RWTexture3D<min16float>	g_txGrid;
RWTexture3D<min16float>	g_RWGrid;

//groupshared min16float4	g_Block[2][2][2];

//--------------------------------------------------------------------------------------
// Up sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	if (g_txGrid[Gid] > 0.0 && asint(g_txGridVal[DTid]) < 0)
	//if (vData.w <= 0.0)
	//if (vDataIn.w > 0.0 && vData.w <= 0.0)
	{
		bool bWrite = true;

		const uint3 vAdjLocs[6] =
		{
			uint3(DTid.x + 1, DTid.yz),
			uint3(DTid.x - 1, DTid.yz),
			uint3(DTid.x, DTid.y + 1, DTid.z),
			uint3(DTid.x, DTid.y - 1, DTid.z),
			uint3(DTid.xy, DTid.z + 1),
			uint3(DTid.xy, DTid.z - 1)
		};

		uint uEmpty = 0;
		uint3 vAdjLoc;
		[unroll]
		for (uint i = 0; i < 6; ++i)
		{
			vAdjLoc = vAdjLocs[i];

			const uint uValue = g_txGridVal[vAdjLoc];
			if (asint(uValue) >= 0 && (uValue & (1 << i)) == 0) bWrite = false;
		}

		if (bWrite)
		{
			g_RWGrid[DTid] = 1.0;
		}
	}
	else if (asint(g_txGridVal[DTid]) >= 0) g_RWGrid[DTid] = 1.0;
}
