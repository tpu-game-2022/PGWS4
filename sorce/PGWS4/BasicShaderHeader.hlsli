struct Output
{
	float4 svpos:SV_POSITION;
	float2 uv:TEXCOORD;
};

Texture2D<float4>tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp:register(s0); //0番スロットに設定されたサンプラー

//cbuffer:定数バッファをまとめるキーワード
//b0:CPUのスロット番号=シェーダのレジスタ番号(bは定数レジスタを表す)(mainで定めたSRVの番号に対応)
cbuffer cbuff0 : register(b0)
{
	matrix mat;
};