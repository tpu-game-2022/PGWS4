float4 BasicPS(float4 pos:POSITION) :SV_TARGET{
	return float4((float2(3,0) + pos.xy) * 0.5f,1,1);
}

//float4 BasicPS() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}