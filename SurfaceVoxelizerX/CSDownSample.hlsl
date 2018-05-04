//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

RWTexture3D<min16float4>		g_RWGridIn;
RWTexture3D<unorm min16float4>	g_RWGridOut;

groupshared min16float4	g_Block[2][2][2];

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	min16float4 vDataIn = g_RWGridIn[DTid];
	vDataIn.xyz = vDataIn.xyz * 2.0 - 1.0;
	g_Block[GTid.x][GTid.y][GTid.z] = min16float4(vDataIn.xyz * vDataIn.w, vDataIn.w);

	g_Block[GTid.x][GTid.y][GTid.z] = GTid.x ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x - 1][GTid.y][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.y ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y - 1][GTid.z] : g_Block[GTid.x][GTid.y][GTid.z];
	g_Block[GTid.x][GTid.y][GTid.z] = GTid.z ? g_Block[GTid.x][GTid.y][GTid.z] + g_Block[GTid.x][GTid.y][GTid.z - 1] : g_Block[GTid.x][GTid.y][GTid.z];

	if (all(GTid))
	{
		min16float4 vDataOut = g_Block[GTid.x][GTid.y][GTid.z];
		vDataOut.xyz = normalize(vDataOut.xyz);
		g_RWGridOut[Gid] = min16float4(vDataOut.xyz * 0.5 + 0.5, vDataOut.w);
	}
}
