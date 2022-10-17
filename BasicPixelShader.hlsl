float4 BasicPS(float4 pos:POSITION) : SV_TARGET
{
	return float4(1,(float2(0,1) + pos.xy) * 0.5f,1);
}