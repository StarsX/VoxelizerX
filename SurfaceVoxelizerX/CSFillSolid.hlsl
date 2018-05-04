//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

RWTexture3D<min16float4>		g_RWGridIn;
RWTexture3D<unorm min16float4>	g_RWGridOut;

groupshared min16float			g_Bound[2][2][2];

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	const min16float4 vDataIn = g_RWGridIn[Gid];
	
	const min16float4 vData = g_RWGridOut[DTid];
	g_Bound[GTid.x][GTid.y][GTid.z] = vData.w;
	GroupMemoryBarrierWithGroupSync();
		
	if (vDataIn.w > 0.0 && vData.w <= 0.0)
	{
		const min16float3 vNorm = vDataIn.xyz * 2.0 - 1.0;
		bool bWrite = true;

		for (uint i = 0; i < 2 && bWrite; ++i)
		{
			for (uint j = 0; j < 2 && bWrite; ++j)
			{
				for (uint k = 0; k < 2 && bWrite; ++k)
				{
					if (g_Bound[i][j][k] > 0.0)
					{
						const float3 vPos = GTid;
						const float3 vPosSurf = float3(i, j, k);

						min16float3 vDir = min16float3(normalize(vPosSurf - vPos));
						vDir.y = -vDir.y;

						if (dot(vNorm, vDir) <= 0.0) bWrite = false;
					}
				}
			}
		}

		if (bWrite) g_RWGridOut[DTid] = vDataIn;
	}
}
