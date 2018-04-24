//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

struct PSIn
{
	float4	Pos		: SV_POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
	float4	Bound	: AABB;
};

RWTexture3D<unorm min16float4> g_RWGrid;

void main(PSIn input)
{
	float3 vDim;
	g_RWGrid.GetDimensions(vDim.x, vDim.y, vDim.z);

	const uint3 vLoc = input.TexLoc * vDim;
	const float4 vBound = input.Bound * vDim.x;	// TODO: if vDim.x, vDim.y, and vDim.z are different

	if (input.Pos.x + 1.0 > vBound.x && input.Pos.y + 1.0 > vBound.y &&
		input.Pos.x < vBound.z + 1.0 && input.Pos.y < vBound.w + 1.0)
	{
		min16float3 vNorm = min16float3(normalize(input.Nrm));
		g_RWGrid[vLoc] = min16float4(vNorm * 0.5 + 0.5, 1.0);
	}
}
