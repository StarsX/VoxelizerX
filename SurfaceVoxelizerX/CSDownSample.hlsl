//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

RWTexture3D<min16float>	g_RWGridIn[3];
RWTexture3D<min16float>	g_RWGridOut[3];

groupshared min16float3	g_Block[2][2][2];

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	g_Block[GTid.x][GTid.y][GTid.z].x = g_RWGridIn[0][DTid];
	g_Block[GTid.x][GTid.y][GTid.z].y = g_RWGridIn[1][DTid];
	g_Block[GTid.x][GTid.y][GTid.z].z = g_RWGridIn[2][DTid];
	//GroupMemoryBarrierWithGroupSync();

	g_Block[GTid.x][GTid.y][GTid.z] = GTid.x ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x - 1][GTid.y][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.y ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y - 1][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.z ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y][GTid.z - 1] : g_Block[GTid.x][GTid.y][GTid.z];

	if (all(GTid))
	{
		const min16float3 vDataOut = g_Block[GTid.x][GTid.y][GTid.z];
		g_RWGridOut[0][Gid] = vDataOut.x;
		g_RWGridOut[1][Gid] = vDataOut.y;
		g_RWGridOut[2][Gid] = vDataOut.z;
	}
}
