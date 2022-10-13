float4 BasicPS(float4 pos:POSITION) : SV_TARGET
{
	//yellow&orange
	return float4((float2(1,1) + pos.xy) * 0.5f,0.0f,1);
}