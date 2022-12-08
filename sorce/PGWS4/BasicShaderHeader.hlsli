struct Output
{
	float4 svpos:SV_POSITION;
	float4 normal:NORMAL0; //法線ベクトル
	float4 vnormal : NORMAL1;//ビュー変換後の法線ベクトル
	float3 ray : VECTOR;//ベクトル
	float2 uv:TEXCOORD;
};

Texture2D<float4>tex : register(t0); //0番スロットに設定されたテクスチャ
Texture2D<float4>sph : register(t1); //1番スロットに設定されたテクスチャ
Texture2D<float4>spa : register(t2); //1番スロットに設定されたテクスチャ

SamplerState smp:register(s0); //0番スロットに設定されたサンプラー

//cbuffer:定数バッファをまとめるキーワード
//b0:CPUのスロット番号=シェーダのレジスタ番号(bは定数レジスタを表す)(mainで定めたSRVの番号に対応)
cbuffer cbuff0 : register(b0)
{
	matrix world;
	matrix view;
	matrix proj;
	float3 eye;
};

//定数バッファー1
//マテリアル用
cbuffer Material : register(b1)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
}