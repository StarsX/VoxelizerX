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
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	min16float4 vData;
	vData.x = g_txGrid[0][DTid];
	vData.y = g_txGrid[1][DTid];
	vData.z = g_txGrid[2][DTid];
#if USING_SRV
	vData.w = g_txGrid[3][DTid];
#else
	vData.w = any(vData.xyz) ? 1.0 : 0.0;
#endif
	g_Block[GTid.x][GTid.y][GTid.z] = vData;
	GroupMemoryBarrierWithGroupSync();

	g_Block[GTid.x][GTid.y][GTid.z] = GTid.x ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x - 1][GTid.y][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.y ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y - 1][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.z ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y][GTid.z - 1] : g_Block[GTid.x][GTid.y][GTid.z];

	if (all(GTid))
	{
		vData = g_Block[GTid.x][GTid.y][GTid.z];
		g_RWGrid[0][Gid] = vData.x;
		g_RWGrid[1][Gid] = vData.y;
		g_RWGrid[2][Gid] = vData.z;
		g_RWGrid[3][Gid] = vData.w;
	}
}
