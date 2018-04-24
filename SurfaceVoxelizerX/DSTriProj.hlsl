//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

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

	// Change domain location with offset for extrapolation
	const float3 vExtDir = normalize(domain - 1.0 / 3.0);
	domain += vExtDir * 4.0;	// TODO: always 4.0?

	// Extrapolations
	output.Pos = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	output.PosLoc = patch[0].PosLoc * domain.x + patch[1].PosLoc * domain.y + patch[2].PosLoc * domain.z;
	output.Nrm = patch[0].Nrm * domain.x + patch[1].Nrm * domain.y + patch[2].Nrm * domain.z;
	output.TexLoc = patch[0].TexLoc * domain.x + patch[1].TexLoc * domain.y + patch[2].TexLoc * domain.z;
	output.Bound = patch[0].Bound;

	return output;
}
