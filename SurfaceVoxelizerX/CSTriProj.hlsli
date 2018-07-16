//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#define CONSERVATION_AMT	1.0 / 3.0	// Measured by pixels, e.g. 1.0 / 3.0 means 1/3 pixel size
#define NUM_CONTROL_POINTS	3

struct HSOut
{
	float2	Pos;
	//float3	PosLoc;
	//float3	Nrm;
	float3	TexLoc;
};

// Cross-product-like operation in 2D space
float cross2D(float2 v1, float2 v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

HSOut CSHullShader(VSOut ip[NUM_CONTROL_POINTS], uint i : PrimitiveVertexIndex)
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

	// Skip pass through
	//output.PosLoc = ip[i].PosLoc;
	//output.Nrm = ip[i].Nrm;

	// Texture 3D space
	output.TexLoc = ip[i].Pos * 0.5 + 0.5;
	output.TexLoc.y = 1.0 - output.TexLoc.y;

	return output;
}

struct DSIn
{
	float2	Pos;
	float3	PosLoc;
	float3	Nrm;
	float3	TexLoc;
};

struct DSOut
{
	float2	Pos;
	float3	PosLoc;
	float3	Nrm;
	float3	TexLoc;
	float4	Bound;
};

cbuffer cbPerMipLevel
{
	float g_fGridSize;
};

DSOut CSDomainShader(DSIn patch[NUM_CONTROL_POINTS], uint i : PrimitiveVertexIndex, float3 domain : DomainLocation)
{
	DSOut output;

	// Distance to centroid in rasterizer space
	const float2 vCentroid = (patch[0].Pos + patch[1].Pos + patch[2].Pos) / 3.0;
	const float fDistance = distance(patch[i].Pos, vCentroid) * g_fGridSize * 0.5;

	// Change domain location with offset for extrapolation
	const float3 f1PixelOffset = (domain - 1.0 / 3.0) / fDistance;
	domain += CONSERVATION_AMT * f1PixelOffset;

	// Extrapolations
	output.Pos = patch[0].Pos * domain.x + patch[1].Pos * domain.y + patch[2].Pos * domain.z;
	output.PosLoc = patch[0].PosLoc * domain.x + patch[1].PosLoc * domain.y + patch[2].PosLoc * domain.z;
	output.Nrm = patch[0].Nrm * domain.x + patch[1].Nrm * domain.y + patch[2].Nrm * domain.z;
	output.TexLoc = patch[0].TexLoc * domain.x + patch[1].TexLoc * domain.y + patch[2].TexLoc * domain.z;
	// output.Pos.zw = float2(0.5, 1.0);	// Set by the vertex shader next pass

	// Calculate projected AABB
	output.Bound.xy = min(min(patch[0].Pos, patch[1].Pos), patch[2].Pos);
	output.Bound.zw = max(max(patch[0].Pos, patch[1].Pos), patch[2].Pos);
	output.Bound = output.Bound * 0.5 + 0.5;
	output.Bound.yw = 1.0 - output.Bound.wy;

	return output;
}
