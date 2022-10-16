float4 BasicPS(float4 pos:POSITION) : SV_TARGET
{
	// Šî–{Œ`
	// return float4((float2(0,1) + pos.xy) * 0.5f, 1, 1);

	if (pos.y < 0.0f) pos.y *= -1.0f;
	float3 col = saturate(abs(6 * pos.y - float3(3,2,4)) * float3(1,-1,-1) + float3(-1,2,2));

	return float4(col, 1);
}