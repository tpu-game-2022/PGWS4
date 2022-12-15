#include "BasicShaderHeader.hlsli"

#define TOON

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1,-1,1));  //平行光線ベクトル
	float3 lightColor = float3(0.7, 0.7, 0.7);
	float brightness = dot(-light, input.normal.xyz);
	float3 refLight = normalize(reflect(light, normalize(input.normal.xyz)));
	float specularB = pow(saturate(dot(refLight, -normalize(input.ray))), specular.a);
	float2 normalUV = input.normal.xy * float2(0.5, -0.5) + 0.5;
	float3 normal = normalize(input.normal.xyz);
	float diffuseB = saturate(dot(-light, normal));
	float2 sphereMapUV = input.vnormal.xy * float2(0.5, -0.5) + 0.5;
	float4 color = tex.Sample(smp, input.uv);
	float4 texColor = tex.Sample(smp, input.uv);

#ifdef TOON
		float3 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB)).rgb;
		return float4(lightColor *// ライトカラー
			(texColor.rgb // テクスチャカラー
				* sph.Sample(smp, sphereMapUV).rgb // スフィアマップ（乗算）
				* (ambient + toonDif * diffuse.rgb) // 環境光＋ディフューズ色
				+ spa.Sample(smp, sphereMapUV).rgb// スフィアマップ（加算）
				+ specularB * specular.rgb) // スペキュラ
			, diffuse.a); // アルファ
#endif // TOON
	return float4(lightColor * //ライトカラー
		(texColor.rgb  //テクスチャカラー
			* sph.Sample(smp, sphereMapUV).rgb  //スフィアマップ（乗算）
			* (ambient + diffuseB * diffuse.rgb)  //環境光＋出フューズ色
			+ spa.Sample(smp, sphereMapUV).rgb  //スフィアマップ（加算）
			+ specularB)  //スペキュラ
		, diffuse.a);  //アルファ
}
