#include"BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv:TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT)
{
	Output output; //ピクセルシェーダーに渡す値
	output.svpos = mul(mat,pos); //mul関数を使うとmatrixで変換されたものが表示される
	output.normal = normal;
	output.uv = uv;

	return output;
}