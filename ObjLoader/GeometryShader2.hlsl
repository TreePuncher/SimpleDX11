

struct VOUT
{
	float4 pos : SV_Position;
	float3 UVW : BaryCentric;
};


struct PIN
{
	float4 pos : SV_Position;
	float3 dist : DISTANCE;
};


[maxvertexcount(4)]
void main(triangle VOUT input[3], inout TriangleStream<PIN> OutputStream)
{
	float2 WIN_SCALE = float2(800, 600);
	
	float2 p0 = WIN_SCALE * input[0].pos.xy / input[0].pos.w;
	float2 p1 = WIN_SCALE * input[1].pos.xy / input[1].pos.w;
	float2 p2 = WIN_SCALE * input[2].pos.xy / input[2].pos.w;
	
	float2 v0 = p2 - p1;
	float2 v1 = p2 - p0;
	float2 v2 = p1 - p0;
	
	PIN V;
	float area	= abs(v1.x * v2.y - v1.y * v2.x);
	V.dist		= float3(area / length(v0), 0, 0);
	V.pos		= input[0].pos;
	OutputStream.Append(V);
	
	V.dist		= float3(0, area / length(v1), 0);
	V.pos		= input[1].pos;
	OutputStream.Append(V);

	V.dist		= float3(0, 0, area / length(v2));
	V.pos		= input[2].pos;
	OutputStream.Append(V);
	OutputStream.RestartStrip();
}


