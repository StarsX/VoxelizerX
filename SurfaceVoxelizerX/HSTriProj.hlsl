//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

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

#define NUM_CONTROL_POINTS 3

// Patch Constant Function
HSConstDataOut CalcHSPatchConstants()
{
	HSConstDataOut Output;

	// Insert code to compute Output here
	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] = Output.InsideTessFactor = 1.0f; // e.g. could calculate dynamic tessellation factors instead

	return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSOut main(InputPatch<VSOut, NUM_CONTROL_POINTS> ip, uint i : SV_OutputControlPointID)
{
	HSOut output;

	// Calculate AABB
	const float3 vAABBMin = min(min(ip[0].Pos, ip[1].Pos), ip[2].Pos);
	const float3 vAABBMax = max(max(ip[0].Pos, ip[1].Pos), ip[2].Pos);
	const float3 vAABBExt = vAABBMax - vAABBMin;

	// Calculate projected AABB sizes for 3 views
	const float fSizeXY = dot(vAABBExt.xy, vAABBExt.xy);
	const float fSizeYZ = dot(vAABBExt.yz, vAABBExt.yz);
	const float fSizeZX = dot(vAABBExt.zx, vAABBExt.zx);

	// Select the view with maximal projected AABB
	output.Pos.xy = fSizeXY > fSizeYZ ?
		(fSizeXY > fSizeZX ? ip[i].Pos.xy : ip[i].Pos.zx) :
		(fSizeYZ > fSizeZX ? ip[i].Pos.yz : ip[i].Pos.zx);
	output.Pos.zw = float2(0.5, 1.0);

	// Other attributes
	output.PosLoc = ip[i].PosLoc;
	output.Nrm = ip[i].Nrm;

	// Texture 3D space
	output.TexLoc = ip[i].Pos * 0.5 + 0.5;
	output.TexLoc.y = 1.0 - output.TexLoc.y;

	// Record projected AABB
	output.Bound.xy = fSizeXY > fSizeYZ ?
		(fSizeXY > fSizeZX ? vAABBMin.xy : vAABBMin.zx) :
		(fSizeYZ > fSizeZX ? vAABBMin.yz : vAABBMin.zx);
	output.Bound.zw = fSizeXY > fSizeYZ ?
		(fSizeXY > fSizeZX ? vAABBMax.xy : vAABBMax.zx) :
		(fSizeYZ > fSizeZX ? vAABBMax.yz : vAABBMax.zx);
	output.Bound = output.Bound * 0.5 + 0.5;
	output.Bound.yw = 1.0 - output.Bound.wy;

	return output;
}
