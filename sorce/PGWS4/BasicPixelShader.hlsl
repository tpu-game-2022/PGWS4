#include"BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	//Nが法線
	//ライトの位置
	float3 light = normalize(float3(1,-1,1));
	//輝度(Id)を求める為dot(内積)
	float brightness = dot(-light, input.normal);

	return float4(brightness, brightness, brightness, 1) * diffuse;
}