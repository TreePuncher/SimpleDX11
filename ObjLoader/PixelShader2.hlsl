


struct VOUT
{
	float4 pos : SV_Position;
	float3 UVW : BaryCentric;
	float vertexId : ID;
};



float edgeFactor(float3 UVW)
{
	const float lineWidth = 1.0;

	
	float3 d = fwidth(UVW);
	float3 f = step(d * lineWidth, UVW);
	return min(min(f.x, f.y), f.z);
}

//float4 PMain(const VOUT IN) : SV_Target
//{
//	const float3 lineColor = float3(1.0, 1.0, 1.0);
//	return float4(min(edgeFactor(IN.UVW), lineColor), 1);
//}


struct PIN
{
	float4 pos : SV_Position;
	float3 d : DISTANCE;
};

float4 main(const PIN IN) : SV_Target
{
	const float d = min(IN.d[0], min(IN.d[1], IN.d[2])) / 2.0f;
	const float I = exp2(-2 * d * d);
	
	return	float4(0, 0, 0, 1) * I + 
			float4(0.5f, 0.5f, 0.5f, 1) * (1.0 - I);
}