//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "PSTriProj.hlsli"

void main(const PSIn input)
{
	const uint3 vLoc = input.TexLoc * g_fGridSize;
	
	PSTriProj(input, vLoc);
}
