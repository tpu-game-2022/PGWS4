float4 BasicPS(float4 pos:POSITION) : SV_TARGET
{
	return float4((float2(0,1) + pos.xy) * 0.5f, (float(1) - pos.y) * 0.5f, 1);
}