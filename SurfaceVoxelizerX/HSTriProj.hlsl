//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define NUM_CONTROL_POINTS 3

// Input control point
struct VSOut
{
	float3	Pos		: POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
};

// Output control point
struct HSOut
{
	float2	Pos		: POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
};

// Output patch constant data.
struct HSConstDataOut
{
	float EdgeTessFactor[3]	: SV_TessFactor;		// e.g. would be [4] for a quad domain
	float InsideTessFactor	: SV_InsideTessFactor;	// e.g. would be Inside[2] for a quad domain
};

// Patch Constant Function
HSConstDataOut CalcHSPatchConstants()
{
	HSConstDataOut Output;

	// Insert code to compute Output here
	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] = Output.InsideTessFactor = 1.0f; // e.g. could calculate dynamic tessellation factors instead

	return Output;
}

// Cross-product-like operation in 2D space
float cross2D(float2 v1, float2 v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOut main(InputPatch<VSOut, NUM_CONTROL_POINTS> ip, uint i : SV_OutputControlPointID)
{
	HSOut output;

	// Calculate projected edges of 3-views respectively
	const float3 vEdge1 = ip[1].Pos - ip[0].Pos;
	const float3 vEdge2 = ip[2].Pos - ip[1].Pos;

	// Calculate projected triangle sizes (equivalent to area) for 3 views
	const float fSizeXY = abs(cross2D(vEdge1.xy, vEdge2.xy));
	const float fSizeYZ = abs(cross2D(vEdge1.yz, vEdge2.yz));
	const float fSizeZX = abs(cross2D(vEdge1.zx, vEdge2.zx));

	// Select the view with maximal projected AABB
	output.Pos = fSizeXY > fSizeYZ ?
		(fSizeXY > fSizeZX ? ip[i].Pos.xy : ip[i].Pos.zx) :
		(fSizeYZ > fSizeZX ? ip[i].Pos.yz : ip[i].Pos.zx);

	// Other attributes
	output.PosLoc = ip[i].PosLoc;
	output.Nrm = ip[i].Nrm;

	// Texture 3D space
	output.TexLoc = ip[i].Pos * 0.5 + 0.5;
	output.TexLoc.y = 1.0 - output.TexLoc.y;

	return output;
}
