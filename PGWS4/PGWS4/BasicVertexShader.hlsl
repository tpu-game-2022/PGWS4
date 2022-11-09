#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT)
{
	Output output; // ピクセルシェーダーに渡す値
	output.svpos = mul(mat, pos);
	output.normal = normal;
	output.uv = uv;
	return output;
}