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
	const min16float3 vNorm = min16float3(normalize(input.Nrm));

	uint uLock;
	[allow_uav_condition]
	for (uint i = 0; i < 0xffffffff; ++i)
	{
		InterlockedExchange(g_RWMutex[vLoc], 1, uLock);
		DeviceMemoryBarrier();
		if (uLock != 1)
		{
			// Critical section
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
