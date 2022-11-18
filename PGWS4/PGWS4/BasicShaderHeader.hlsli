//頂点シェーダからピクセルシェーダーへのやりとりに使用する構造体
struct Output
{
	float4 svpos:SV_POSITION;//システム用頂点座標
	float4 normal : NORMAL;
	float2 uv:TEXCOORD;//uv値
};

Texture2D<float4> tex : register(t0);//0番スロットに設定されたテクスチャ
SamplerState smp : register(s0);//0番スロットに設定されたサンプラー

//定数バッファー
cbuffer cbuff0 :register(b0)
{
	matrix world;
	matrix viewproj;
};

//定数バッファー1
//マテリアル用
cbuffer Material :register(b1)
{
	float4 diffuse;//ディフューズ色
	float4 specular;//スぺキュラ色
	float3 ambient;//アビエント色
};

//struct Matrix
//{
//	matrix mat;
//};
//
//ConstantBuffer<Matrix> m : register(b0);