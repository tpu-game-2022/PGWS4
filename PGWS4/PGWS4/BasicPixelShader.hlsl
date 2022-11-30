#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(-cos(lightangle), -1, sin(lightangle)));
	float brightness = dot(-light, input.normal);
	return float4(brightness, brightness, brightness, 1)
		* diffuse * tex.Sample(smp, input.uv);
}