#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos:POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT)
{
	Output output;  //ピクセルシェーダーに渡す値
	pos = mul(world, pos);
	output.ray = pos.xyz - eye;
	output.svpos = mul(proj,mul(view,mul(world,pos)));
	output.normal = normal;
	output.uv = uv;
	output.vnormal = mul(view, output.normal);
	return output;
}
