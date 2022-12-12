#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	// 光へ向かうベクトル（平行光線ベクトル）
	float3 light = normalize(float3(cos(lightAngle), -1, sin(lightAngle)));  //cos(lightAngle), -1, sin(lightAngle)//(1, -1, 1)

	// ライトのカラー（真っ白）
	float3 lightColor = float3(1.0, 1.0, 1.0);

	// ディフューズ計算
	float3 normal = normalize(input.normal.xyz);
	float diffuseB = saturate(dot(-light, normal));

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, normalize(input.normal.xyz)));
	float specularB = pow(saturate(dot(refLight, -normalize(input.ray))), specular.a);

	// スフィアマップ用uv
	float2 sphereMapUV = input.vnormal.xy * float2(0.5, -0.5) + 0.5;

	// テクスチャカラー
	float4 texColor = tex.Sample(smp, input.uv);

	return float4(lightColor * // ライトカラー
		(texColor.rgb // テクスチャカラー
		* sph.Sample(smp, sphereMapUV).rgb // スフィアマップ（乗算）
		* (ambient + diffuseB * diffuse.rgb) // 環境光＋ディフューズ色
		+ spa.Sample(smp, sphereMapUV).rgb // スフィアマップ（加算）
		+ specularB * specular.rgb) // スペキュラ
		, diffuse.a); // アルファ
}