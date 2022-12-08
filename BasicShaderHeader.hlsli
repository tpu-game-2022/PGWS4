//頂点シェーダーからピクセルシェーダーへのやり取りに使う
struct Output
{
	float4 svpos:SV_POSITION;//システム用頂点座標
	float4 normal:NORMAL0;//法線ベクトル
	float4 vnormal:NORMAL1;//ビュー変換後の法線ベクトル
	float2 uv:TEXCOORD;//uv値
	float3 ray : VECTOR;//ベクトル
};

Texture2D<float4>tex : register(t0);//0番スロットに設定されたテクスチャ
Texture2D<float4>sph : register(t1);//1番スロットに設定されたテクスチャ
Texture2D<float4>spa : register(t2);//2番スロットに設定されたテクスチャ
SamplerState smp:register(s0);//0番スロットに設定されたサンプラー

//定数バッファー
cbuffer basic: register(b0)
{
	matrix world;//ワールド変換行列
	matrix view;//ビュー行列
	matrix proj;//プロジェクション行列
	float3 eye;
};

//定数バッファー1
//マテリアル用
cbuffer Material : register(b1)
{
	float4 diffuse;//ディフューズ色
	float4 specular;//スペキュラ
	float3 ambient;//アンビエント
};