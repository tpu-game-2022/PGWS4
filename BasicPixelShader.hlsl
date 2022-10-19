#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) :SV_TARGET{
	return float4(input.uv,1,1);
}

//float4 BasicPS() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}