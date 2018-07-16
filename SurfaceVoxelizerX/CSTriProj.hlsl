//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"
#include "CHDataSize.hlsli"

#define	main	VertShader
#include "VSTriProjTess.hlsl"
#undef main

#include "CSTriProj.hlsli"

static const uint g_uStrideVB = SIZE_OF_FLOAT3 * 2;

ByteAddressBuffer g_roIndices;
ByteAddressBuffer g_roVertices;
RWStructuredBuffer<DSOut> g_rwVertices;

groupshared VSOut g_VSOut[GROUP_SIZE];
groupshared HSOut g_HSOut[GROUP_SIZE];

VSIn LoadVSIn(uint uIdx)
{
	VSIn result;
	const uint uVid = g_roIndices.Load(SIZE_OF_UINT * uIdx);
	const uint uBase = uVid * g_uStrideVB;

	result.Pos = asfloat(g_roVertices.Load3(uBase));
	result.Nrm = asfloat(g_roVertices.Load3(uBase + SIZE_OF_FLOAT3));

	return result;
}

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GIdx : SV_GroupIndex)
{
	// Vertex shader
	g_VSOut[GIdx] = VertShader(LoadVSIn(DTid.x));
	GroupMemoryBarrierWithGroupSync();

	// Compute primitive related
	const uint uPrimId = GIdx / NUM_CONTROL_POINTS;
	const uint uPrimIdx = GIdx % NUM_CONTROL_POINTS;
	const uint uGIdxBase = uPrimId * NUM_CONTROL_POINTS;

	// Hull shader
	VSOut ip[3];
	[unroll]
	for (uint i = 0; i < NUM_CONTROL_POINTS; ++i)
		ip[i] = g_VSOut[uGIdxBase + i];
	g_HSOut[GIdx] = CSHullShader(ip, uPrimIdx);
	GroupMemoryBarrierWithGroupSync();

	// Domain shader
	DSIn patch[3];
	[unroll]
	for (i = 0; i < NUM_CONTROL_POINTS; ++i)
	{
		HSOut hsOut = g_HSOut[uGIdxBase + i];
		patch[i].Pos = hsOut.Pos;
		patch[i].TexLoc = hsOut.TexLoc;
		patch[i].PosLoc = ip[i].PosLoc;
		patch[i].Nrm = ip[i].Nrm;
	}
	const float3 domain = float3(uPrimIdx == 0, uPrimIdx == 1, uPrimIdx == 2);
	g_rwVertices[DTid.x] = CSDomainShader(patch, uPrimIdx, domain);
}