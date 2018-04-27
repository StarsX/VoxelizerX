//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

#define CONSERVATION_AMT	1.0 / 3.0				// Measured by pixels, e.g. 1.0 / 3.0 means 1/3 pixel size
#define NUM_CONTROL_POINTS	3

struct DSIn
{
	float2	Pos		: POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
};

struct DSOut
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

[domain("tri")]
DSOut main(HSConstDataOut input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<DSIn, NUM_CONTROL_POINTS> patch)
{
	DSOut output;

	// Distance to centroid in rasterizer space
	const float2 vVertex = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	const float2 vCentroid = (patch[0].Pos + patch[1].Pos + patch[2].Pos) / 3.0;
	const float fDistance = distance(vVertex, vCentroid) * GRID_SIZE * 0.5;

	// Change domain location with offset for extrapolation
	const float3 f1PixelOffset = (domain - 1.0 / 3.0) / fDistance;
	domain += CONSERVATION_AMT * f1PixelOffset;

	// Extrapolations
	output.Pos.xy = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	output.PosLoc = patch[0].PosLoc * domain.x + patch[1].PosLoc * domain.y + patch[2].PosLoc * domain.z;
	output.Nrm = patch[0].Nrm * domain.x + patch[1].Nrm * domain.y + patch[2].Nrm * domain.z;
	output.TexLoc = patch[0].TexLoc * domain.x + patch[1].TexLoc * domain.y + patch[2].TexLoc * domain.z;
	output.Pos.zw = float2(0.5, 1.0);

	// Calculate projected AABB
	output.Bound.xy = min(min(patch[0].Pos, patch[1].Pos), patch[2].Pos);
	output.Bound.zw = max(max(patch[0].Pos, patch[1].Pos), patch[2].Pos);
	output.Bound = output.Bound * 0.5 + 0.5;
	output.Bound.yw = 1.0 - output.Bound.wy;

	return output;
}
