//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define WRITE_BIT(d, m, s)	(((d) & ~(m)) | ((s) & (m)))
#define MERGE(d, m, c, s)	d = (c) ? WRITE_BIT(d, (m).x | (m).y | (m).z, (d) & (s)) : (d)

RWTexture3D<uint>	g_txGrid;
RWTexture3D<uint>	g_RWGrid;

groupshared uint	g_Block[2][2][2];

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	uint uData = g_txGrid[DTid];
	g_Block[GTid.x][GTid.y][GTid.z] = uData;
	GroupMemoryBarrierWithGroupSync();

	const uint4 vMask = { 1 << GTid.x, 1 << (2 + GTid.y), 1 << (4 + GTid.z), 1 << 31 };
	MERGE(g_Block[GTid.x][GTid.y][GTid.z], vMask.yzw, GTid.x, g_Block[GTid.x - 1][GTid.y][GTid.z]);
	MERGE(g_Block[GTid.x][GTid.y][GTid.z], vMask.zxw, GTid.y, g_Block[GTid.x][GTid.y - 1][GTid.z]);
	MERGE(g_Block[GTid.x][GTid.y][GTid.z], vMask.xyw, GTid.z, g_Block[GTid.x][GTid.y][GTid.z - 1]);
	
	if (all(GTid)) g_RWGrid[Gid] = g_Block[GTid.x][GTid.y][GTid.z];
}
