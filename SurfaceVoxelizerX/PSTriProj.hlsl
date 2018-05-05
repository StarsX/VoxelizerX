//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"

struct PSIn
{
	float4	Pos		: SV_POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
};

RWTexture3D<unorm min16float4> g_RWGrid;

void main(PSIn input)
{
	const uint3 vLoc = input.TexLoc * GRID_SIZE;

	g_RWGrid[vLoc] = min16float4(normalize(input.Nrm), 1.0);
}
