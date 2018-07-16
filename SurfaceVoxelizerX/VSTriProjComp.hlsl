//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "CHDataSize.hlsli"

struct VSIn
{
	float2	Pos	;
	float3	PosLoc;
	float3	Nrm;
	float3	TexLoc;
	float4	Bound;
};

struct VSOut
{
	float4	Pos		: SV_POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
	float4	Bound	: AABB;
};

ByteAddressBuffer g_roIndices;
StructuredBuffer<VSIn> g_roVertices;

VSOut main(uint vid : SV_VertexID)
{
	VSOut output;

	const uint uVid = g_roIndices.Load(SIZE_OF_UINT * vid);
	VSIn input = g_roVertices[uVid];
	output.Pos = float4(input.Pos, 0.5, 1.0);
	output.PosLoc = input.PosLoc;
	output.Nrm = input.Nrm;
	output.TexLoc = input.TexLoc;
	output.Bound = input.Bound;

	return output;
}
