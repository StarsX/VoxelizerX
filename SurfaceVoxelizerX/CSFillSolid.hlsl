//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

RWTexture3D<min16float>	g_RWGridIn[3];
RWTexture3D<min16float>	g_RWGridOut[4];

groupshared min16float4	g_Bound[2][2][2];

//--------------------------------------------------------------------------------------
// Down sampling
//--------------------------------------------------------------------------------------
[numthreads(2, 2, 2)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	min16float4 vDataIn;
	vDataIn.x = g_RWGridIn[0][Gid];
	vDataIn.y = g_RWGridIn[1][Gid];
	vDataIn.z = g_RWGridIn[2][Gid];
	vDataIn.w = any(vDataIn.xyz) ? 1.0 : 0.0;
	
	min16float4 vData;
	vData.x = g_RWGridOut[0][DTid];
	vData.y = g_RWGridOut[1][DTid];
	vData.z = g_RWGridOut[2][DTid];
	vData.w = any(vData.xyz) ? 1.0 : 0.0;

	g_Bound[GTid.x][GTid.y][GTid.z] = vData;
	GroupMemoryBarrierWithGroupSync();

	if (vDataIn.w > 0.0 && vData.w <= 0.0)
	{
		//const min16float3 vNormAvg = normalize(vDataIn.xyz);
		bool bWrite = true;

		for (uint i = 0; i < 2 && bWrite; ++i)
		{
			for (uint j = 0; j < 2 && bWrite; ++j)
			{
				for (uint k = 0; k < 2 && bWrite; ++k)
				{
					if (g_Bound[i][j][k].w > 0.0)
					{
						const float3 vPos = GTid;
						const float3 vPosSurf = float3(i, j, k);

						min16float3 vDir = min16float3(normalize(vPosSurf - vPos));
						vDir.y = -vDir.y;

						const min16float3 vNorm = normalize(g_Bound[i][j][k].xyz);
						if (dot(vNorm, vDir) <= 0.0) bWrite = false;
					}
				}
			}
		}

		if (bWrite)
		{
			g_RWGridOut[0][DTid] = vDataIn.x;
			g_RWGridOut[1][DTid] = vDataIn.y;
			g_RWGridOut[2][DTid] = vDataIn.z;
			g_RWGridOut[3][DTid] = vDataIn.w;
		}
	}
}
