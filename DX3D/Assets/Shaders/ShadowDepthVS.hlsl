cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 worldViewProj;
};

struct VSInput
{
    float3 position : POSITION0;
};
struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.position = mul(float4(input.position, 1.0f), worldViewProj);
    return o;
}
