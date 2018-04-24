//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

struct DSInOut
{
	float4	Pos		: SV_POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
	float4	Bound	: AABB;
};

// Output patch constant data.
struct HSConstDataOut
{
	float EdgeTessFactor[3]	: SV_TessFactor;		// e.g. would be [4] for a quad domain
	float InsideTessFactor	: SV_InsideTessFactor;	// e.g. would be Inside[2] for a quad domain
};

#define NUM_CONTROL_POINTS	3

[domain("tri")]
DSInOut main(HSConstDataOut input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<DSInOut, NUM_CONTROL_POINTS> patch)
{
	DSInOut output;

#if 0
	// Distance to centroid in rasterizer space
	const float2 vVertex = patch[0].Pos.xy * domain.x + patch[1].Pos.xy * domain.y + patch[2].Pos.xy * domain.z;
	const float2 vCentroid = (patch[0].Pos.xy + patch[1].Pos.xy + patch[2].Pos.xy) / 3.0;
	const float fRadius = distance(vVertex, vCentroid) * GRID_SIZE * 0.5;

	// Change domain location with offset for extrapolation
	domain += (domain - 1.0 / 3.0) / fRadius;
#else
	// Change domain location with offset for extrapolation
	domain += normalize(domain - 1.0 / 3.0) * 8.1;	// TODO: always 8.1?
#endif

	// Extrapolations
	output.Pos = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	output.PosLoc = patch[0].PosLoc * domain.x + patch[1].PosLoc * domain.y + patch[2].PosLoc * domain.z;
	output.Nrm = patch[0].Nrm * domain.x + patch[1].Nrm * domain.y + patch[2].Nrm * domain.z;
	output.TexLoc = patch[0].TexLoc * domain.x + patch[1].TexLoc * domain.y + patch[2].TexLoc * domain.z;
	output.Bound = patch[0].Bound;

	return output;
}
