#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(cos(lightAngle), -1, sin(lightAngle)));  //cos(lightAngle), -1, sin(lightAngle)
	float brightness = dot(-light, input.normal);   // ì‡êœ
	return float4(brightness, brightness, brightness, 1) * diffuse;
}