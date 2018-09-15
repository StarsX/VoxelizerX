// --------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#ifndef U_DEPTH
#define	U_DEPTH			u5
#endif

RWTexture2DArray<uint>	g_RWKBufDepth : register(U_DEPTH);

void DepthPeel(uint uDepth, uint2 vLoc, const uint uNumLayer)
{
	uint uDepthPrev;

	for (uint i = 0; i < uNumLayer; ++i)
	{
		const uint3 vTex = { vLoc, i };
		InterlockedMin(g_RWKBufDepth[vTex], uDepth, uDepthPrev);

		if (uDepthPrev == uDepth || uDepth == 0xffffffff) break;

		uDepth = max(uDepth, uDepthPrev);
	}
}
