#include "BasicShaderHeader.hlsli"
//struct Output {
//	float4 pos:POSITION;
//	float4 svpos:SV_POSITION;
//};

Output BasicVS(float4 pos:POSITION,float2 uv:TEXCOORD) {
	Output output;  //ピクセルシェーダーに渡す値
	//output.pos = pos;
	output.svpos = pos;
	output.uv = uv;
	return output;
}

//float4 BasicVS( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}