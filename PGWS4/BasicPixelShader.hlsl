float4 BasicPS(float4 pos:POSITION) : SV_TARGET
{
	//ダイヤモンドに輝きを持たせるために0.5fから1.0fに変更し、乗算する
	return float4((float2(0.5f,1.0f) + pos.yx) * 1.0f,1.0f,1.0f);
}