//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

Texture3D<min16float>	g_txGrid[4];
RWTexture3D<uint>		g_RWGrid;

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(32, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	min16float4 vData;
	vData.x = g_txGrid[0][DTid];
	vData.y = g_txGrid[1][DTid];
	vData.z = g_txGrid[2][DTid];
	vData.w = g_txGrid[3][DTid];

	uint uResult = 0;

	if (vData.w > 0.0)
	{
		const min16float3 vNorm = normalize(vData.xyz);

		uResult |= dot(vNorm, min16float3(-1.0, 0.0.xx)) < 0.0 ? 1 : 0;
		uResult |= dot(vNorm, min16float3(1.0, 0.0.xx)) < 0.0 ? (1 << 1) : 0;
		uResult |= dot(vNorm, min16float3(0.0, -1.0, 0.0)) < 0.0 ? (1 << 2) : 0;
		uResult |= dot(vNorm, min16float3(0.0, 1.0, 0.0)) < 0.0 ? (1 << 3) : 0;
		uResult |= dot(vNorm, min16float3(0.0.xx, -1.0)) < 0.0 ? (1 << 4) : 0;
		uResult |= dot(vNorm, min16float3(0.0.xx, 1.0)) < 0.0 ? (1 << 5) : 0;
	}
	else uResult = 0xffffffff;

	g_RWGrid[DTid] = uResult;
}
