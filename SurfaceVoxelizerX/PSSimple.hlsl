//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

struct PSIn
{
	float4	Pos		: SV_POSITION;
	min16float4	Nrm	: NORMAL;
};

min16float4 main(PSIn input) : SV_TARGET
{
	const float3 vLightPt = 1.0;
	const min16float3 vLightDir = min16float3(normalize(vLightPt));

	const min16float fLightAmt = saturate(dot(input.Nrm.xyz, vLightDir));
	const min16float fAmbient = lerp(0.25, 0.5, saturate(input.Nrm.y + 0.5));

	return min16float4((fLightAmt + 0.75 * fAmbient * (input.Nrm.w + 0.2)).xxx, 1.0);
	//return min16float4(input.Nrm * 0.5 + 0.5, 1.0);
}
