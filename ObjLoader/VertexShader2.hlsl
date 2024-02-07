cbuffer constants : register(b0)
{
	float4x4	pvt;
};

struct VOUT
{	
	float4 pos : SV_Position;
	float3 UVW : BaryCentric;
};



VOUT main(float3 pos : POSITION, uint idx : SV_VertexID) 
{
	float3 UVW[] =
	{
		float3(1.0f, 0.0f, 0.0f),
		float3(0.0f, 1.0f, 0.0f),
		float3(0.0f, 0.0f, 1.0f),
	};

	VOUT OUT;
	OUT.pos			= mul(pvt, float4(pos, 1));
	OUT.UVW			= UVW[idx % 3];
	
	return OUT;
}