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

RWTexture3D<min16float4>	g_RWGrid;
RWTexture3D<uint>			g_RWMutex;

void main(PSIn input)
{
	const uint3 vLoc = input.TexLoc * GRID_SIZE;
	const float4 vBound = input.Bound * GRID_SIZE;

	if (input.Pos.x + 1.0 > vBound.x && input.Pos.y + 1.0 > vBound.y &&
		input.Pos.x < vBound.z + 1.0 && input.Pos.y < vBound.w + 1.0)
	{
		uint uLock;
		[allow_uav_condition]
		for (uint i = 0; i < 0xffffffff; ++i)
		{
			InterlockedExchange(g_RWMutex[vLoc], 1, uLock);
			DeviceMemoryBarrier();
			if (uLock != 1)
			{
				// Critical section
				g_RWGrid[vLoc] += min16float4(normalize(input.Nrm), 1.0);
				g_RWMutex[vLoc] = 0;
				break;
			}
		}
	}
}
