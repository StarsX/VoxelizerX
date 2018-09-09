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
	const int iValue = asint(g_txGridVal[DTid]);

	if (g_txGrid[Gid] > 0.0 && iValue < 0)
	//if (vData.w <= 0.0)
	//if (vDataIn.w > 0.0 && vData.w <= 0.0)
	{
		bool bWrite = true;
		uint uEmpty = 0;

		const uint3 vAdjLocs[6] =
		{
			uint3(DTid.x + 1, DTid.yz),
			uint3(DTid.x - 1, DTid.yz),
			uint3(DTid.x, DTid.y + 1, DTid.z),
			uint3(DTid.x, DTid.y - 1, DTid.z),
			uint3(DTid.xy, DTid.z + 1),
			uint3(DTid.xy, DTid.z - 1)
		};

		[unroll]
		for (uint i = 0; i < 6; ++i)
		{
			const uint3 vAdjLoc = vAdjLocs[i];
			const uint uValue = g_txGridVal[vAdjLoc];
			if ((uValue & (1 << i)) == 0) bWrite = false;
			if (asint(uValue) < 0) ++uEmpty;
		}

		if (bWrite && uEmpty < 6)
		{
			g_RWGrid[DTid] = 0.0625;
		}
	}
	else if (iValue >= 0) g_RWGrid[DTid] = 1.0;
}
