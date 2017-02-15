
float4 main(float4 color) : SV_TARGET
{
	return float4(color[0], color[1], color[2], color[3]);
}