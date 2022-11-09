//頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;//システム用頂点座標
	float4 normal : NORMAL;//法線ベクトル
	float2 uv : TEXCOORD;//uv値
};

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

//定数バッファー
cbuffer cbuff0 : register(b0)
{
	matrix world;
	matrix viewproj;
};