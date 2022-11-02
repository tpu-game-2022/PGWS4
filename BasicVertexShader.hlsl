#include "BasicShaderHeader.hlsli"

//struct Output {
//	float4 pos:POSITION;
//	float4 svpos:SV_POSITION;
//};

Output BasicVS(float4 pos:POSITION,float2 uv:TEXCOORD) 
{
	Output output;
	/*output.pos = pos;*/
	output.svpos = mul(mat,pos);
	output.uv = uv;
	return output;
}

//float4 BasicVS( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}