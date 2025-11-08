cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 worldViewProj; // <- сюди CPU кладе worldT*viewT*projT
};

struct VSInput
{
    float3 position : POSITION;
};
struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.position = mul(worldViewProj, float4(input.position, 1.0f));
    return o;
}
