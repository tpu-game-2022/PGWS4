#include "BasicShaderHeader.hlsli"

//struct Output {
//	float4 pos:POSITION;
//	float4 svpos:SV_POSITION;
//};

Output BasicVS(
	float4 pos:POSITION,
	float4 normal:NORMAL,
	float2 uv:TEXCOORD,
	min16uint2 boneno:BONE_NO,
	min16uint weight:WEIGHT) 
{
	Output output;
	output.svpos = mul(mat,pos);
	output.normal = normal;
	output.uv = uv;
	return output;
}

//float4 BasicVS( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}