#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(6, 1, -1));
	float brightness = dot(-light, input.normal);
	return float4(brightness, 153.0f / 255.0f + (255.0f - 153.0f) / 255.0f * brightness, 119.0f / 255.0f  + (255.0f - 119.0f) / 255.0f * brightness, 1);
}