//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "CSMutex.hlsli"

struct PSIn
{
	float4	Pos		: SV_POSITION;
	float3	PosLoc	: POSLOCAL;
	float3	Nrm		: NORMAL;
	float3	TexLoc	: TEXLOCATION;
#ifdef _CONSERVATIVE_
	float4	Bound	: AABB;
#endif
};

cbuffer cbPerMipLevel
{
	float g_fGridSize;
};

globallycoherent
RWTexture3D<min16float>	g_RWGrids[4]	: register (u0);

void PSTriProj(const PSIn input, const uint3 vLoc)
{
	const min16float3 vNorm = min16float3(normalize(input.Nrm));

#ifdef _CONSERVATIVE_
	const float4 vBound = input.Bound * g_fGridSize;
	const bool bWrite = input.Pos.x + 1.0 > vBound.x && input.Pos.y + 1.0 > vBound.y &&
		input.Pos.x < vBound.z + 1.0 && input.Pos.y < vBound.w + 1.0;
#endif

	mutexLock(vLoc);
	// Critical section
#ifdef _CONSERVATIVE_
	if (bWrite)
#endif
	{
		g_RWGrids[0][vLoc] += vNorm.x;
		g_RWGrids[1][vLoc] += vNorm.y;
		g_RWGrids[2][vLoc] += vNorm.z;
		g_RWGrids[3][vLoc] = 1.0;
	}
	mutexUnlock(vLoc);
}
