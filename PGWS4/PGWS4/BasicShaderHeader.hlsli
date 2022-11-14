// 頂点シェーダからピクセルシェーダへのやり取りに使われる
struct Output
{
	float4 svpos : SV_POSITION;   // システム用頂点座標
	float4 normal : NORMAL;   // 法線ベクトル
	float2 uv : TEXCOORD;   // UV値
};

Texture2D<float4> tex : register(t0); // 0 番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0 番スロットに設定されたサンプラー

// 定数バッファ
cbuffer cbuff0 : register(b0)
{
	matrix world;   // ワールド変換行列
	matrix viewproj;   // 
	float angile;
};
