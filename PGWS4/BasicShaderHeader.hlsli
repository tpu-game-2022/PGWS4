// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 normal : NORMAL0; // 法線ベクトル
	float4 vnormal : NORMAL1; // ビュー変換後の法線ベクトル
	float2 uv :TEXCOORD; // uv 値
	float3 ray : VECTOR; // ベクトル
};

Texture2D<float4> tex : register(t0); // 0 番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1); // 1 番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2); // 2 番スロットに設定されたテクスチャ
Texture2D<float4> toon : register(t3); // 3 番スロットに設定されたテクスチャ（トゥーン）
SamplerState smp : register(s0); // 0 番スロットに設定されたサンプラー
SamplerState smpToon : register(s1); // 1 番スロットに設定されたサンプラー（トゥーン用）

// 定数バッファー
cbuffer SceneData : register(b0)
{
	matrix view; // ビュー行列
	matrix proj; // プロジェクション行列
	float3 eye; // 視点
};

cbuffer Transform : register(b1)
{
	matrix world; // ワールド行列
	matrix bones[256];
};

// 定数バッファー2
// マテリアル用
cbuffer Material : register(b2)
{
	float4 diffuse;	// ディフューズ色
	float4 specular;// スペキュラ
	float3 ambient;	// アンビエント
};
