//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

typedef uint	packedhalf2;
typedef uint2	packedhalf4;

packedhalf2 packf32tof16(float2 v)
{
	return f32tof16(v.x) | (f32tof16(v.y) << 16);
}

float2 unpackf16tof32(packedhalf2 v)
{
	return float2(f16tof32(v), f16tof32(v >> 16));
}

packedhalf4 packf32tof16(float4 v)
{
	return packedhalf4(packf32tof16(v.xy), packf32tof16(v.zw));
}

float4 unpackf16tof32(packedhalf4 v)
{
	return float4(unpackf16tof32(v.x), unpackf16tof32(v.y));
}

float4 unpackf16tof32(packedhalf2 v1, packedhalf2 v2)
{
	return float4(unpackf16tof32(v1), unpackf16tof32(v2));
}

packedhalf2 packf16(min16float2 f)
{
	return packf32tof16(float2(f));
}

min16float2 unpackf16(packedhalf2 f)
{
	return min16float2(unpackf16tof32(f));
}

packedhalf4 packf16(min16float4 v)
{
	return packf32tof16(float4(v));
}

min16float4 unpackf16(packedhalf4 v)
{
	return min16float4(unpackf16tof32(v));
}

min16float4 unpackf16(packedhalf2 v1, packedhalf2 v2)
{
	return min16float4(unpackf16tof32(v1, v2));
}
