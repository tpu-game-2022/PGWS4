#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));  //cos(lightAngle), -1, sin(lightAngle)
	float brightness = dot(-light, input.normal);   // 内積
	float2 normalUV = input.normal.xy * float2(0.5, -0.5) + 0.5;
	return float4(brightness, brightness, brightness, 1)
		* diffuse   // ディフューズ色
		* tex.Sample(smp, input.uv)   // テクスチャカラー
		* sph.Sample(smp, normalUV);   // スフィアマップ( 乗算)
}