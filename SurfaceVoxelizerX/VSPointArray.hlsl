//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

struct VSOut
{
	float4		Pos	: SV_POSITION;
	min16float4	Nrm	: NORMAL;
};

cbuffer cbMatrices
{
	matrix	g_mWorldViewProj;
	matrix	g_mWorld;
	matrix	g_mWorldIT;
};

Texture3D<min16float4> g_txGrid;

VSOut main(const uint vID : SV_VertexID, const uint SliceID : SV_InstanceID)
{
	VSOut output;

	const uint uGridSize = GRID_SIZE >> SHOW_MIP;
	const uint3 vLoc = { vID % uGridSize, vID / uGridSize, SliceID };
	float3 vPos = (vLoc * 2 + 1) / (uGridSize * 2.0);
	vPos = vPos * float3(2.0, -2.0, 2.0) + float3(-1.0, 1.0, -1.0);

	const min16float4 vGrid = g_txGrid.mips[SHOW_MIP][vLoc];

	output.Pos = mul(float4(vPos, 1.0), g_mWorldViewProj);
	output.Nrm = min16float4(mul(vGrid.xyz * 2.0 - 1.0, (float3x3)g_mWorldIT), vGrid.w);

	output.Pos.w = vGrid.w > 0.0 ? output.Pos.w : 0.0;

	return output;
}
