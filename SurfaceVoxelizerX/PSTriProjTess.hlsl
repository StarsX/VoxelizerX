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
	float4	Bound	: AABB;
};

cbuffer cbPerMipLevel
{
	float g_fGridSize;
};

globallycoherent
RWTexture3D<min16float>	g_RWGrids[4];
RWTexture3D<uint>		g_RWMutex;

void main(PSIn input)
{
	const uint3 vLoc = input.TexLoc * g_fGridSize;
	const float4 vBound = input.Bound * g_fGridSize;

	const min16float3 vNorm = min16float3(normalize(input.Nrm));
	const bool bWrite = input.Pos.x + 1.0 > vBound.x && input.Pos.y + 1.0 > vBound.y &&
		input.Pos.x < vBound.z + 1.0 && input.Pos.y < vBound.w + 1.0;

	uint uLock = 0;
	[allow_uav_condition]
	for (uint i = 0; i < 0xffffffff; ++i)
	{
		InterlockedExchange(g_RWMutex[vLoc], 1, uLock);
		DeviceMemoryBarrier();
		if (uLock != 1)
		{
			// Critical section
			if (bWrite)
			{
				g_RWGrids[0][vLoc] += vNorm.x;
				g_RWGrids[1][vLoc] += vNorm.y;
				g_RWGrids[2][vLoc] += vNorm.z;
				g_RWGrids[3][vLoc] = 1.0;
			}

			g_RWMutex[vLoc] = 0;
			break;
		}
	}
}
