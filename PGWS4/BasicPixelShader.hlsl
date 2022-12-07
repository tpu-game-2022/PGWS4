#include "BasicShaderHeader.hlsli"

#define TOON

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));

	float3 lightColor = float3(1.0, 1.0, 1.0) * 0.7;

	float3 normal = normalize(input.normal.xyz);
	float diffuseB = saturate(dot(-light, normal));

	float3 refLight = reflect(light, normal);
	float specularB = pow(saturate(dot(refLight, -normalize(input.ray))), specular.a);

	float2 sphereMapUV = normalize(input.vnormal).xy * float2(0.5, -0.5) + 0.5;

	float4 texColor = tex.Sample(smp, input.uv);

#ifdef TOON
	float3 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB)).rgb;
	return float4(lightColor *
		(texColor.rgb
			* sph.Sample(smp, sphereMapUV).rgb
			* (ambient + toonDif * diffuse.rgb)
			+ spa.Sample(smp, sphereMapUV).rgb
			+ specularB * specular.rgb)
		, diffuse.a);
#endif

	return float4(lightColor *
		(texColor.rgb
			* sph.Sample(smp, sphereMapUV).rgb
			* (ambient + diffuseB * diffuse.rgb)
			+ spa.Sample(smp, sphereMapUV).rgb
			+ specularB * specular.rgb)
		, diffuse.a);
}