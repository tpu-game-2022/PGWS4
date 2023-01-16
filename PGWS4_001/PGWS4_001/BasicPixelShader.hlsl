#include "BasicShaderHeader.hlsli"

#define TOON2

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1,-1,1));
	float3 lightColor = float3(1.0, 1.0, 1.0) * 0.7;
	float3 normal = normalize(input.normal.xyz);
	float diffuseB = saturate(dot(-light, normal));
	float3 refLight = normalize(reflect(light, normalize(input.normal.xyz)));
	float specularB = pow(saturate(dot(refLight, -normalize(input.ray))), specular.a);
	float2 sphereMapUV = normalize(input.vnormal).xy * float2(0.5, -0.5) + 0.5;
	float4 texColor = tex.Sample(smp, input.uv);

#ifdef TOON1
	float3 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB)).rgb;
	return float4(lightColor
		* (texColor.rgb
			* sph.Sample(smp, sphereMapUV).rgb
			* (ambient + toonDif * diffuse.rgb)
			+ spa.Sample(smp, sphereMapUV).rgb
			+ specularB * specular.rgb)
		, diffuse.a);
#endif

#ifdef TOON2
	float3 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB)).rgb;
	float3 toonSpec = toon.Sample(smpToon, float2(0, 1.0 - specularB)).rgb * 0.25;
	return float4(lightColor
		* (texColor.rgb
			* sph.Sample(smp, sphereMapUV).rgb
			* (ambient + toonDif * diffuse.rgb)
			+ spa.Sample(smp, sphereMapUV).rgb
			+ toonSpec * specular.rgb)
		, diffuse.a);
#endif

	return float4(lightColor
		* (texColor.rgb
			* sph.Sample(smp, sphereMapUV).rgb
			* (ambient + diffuseB * diffuse.rgb)
			+ spa.Sample(smp, sphereMapUV).rgb
			+ specularB * specular.rgb)
		, diffuse.a);
}