#include"BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION,float2 uv:TEXCOORD)
{
	Output output; //ピクセルシェーダーに渡す値
	output.svpos = mul(mat,pos); //mul関数を使うとmatrixで変換されたものが表示される
	output.uv = uv;

	return output;
}