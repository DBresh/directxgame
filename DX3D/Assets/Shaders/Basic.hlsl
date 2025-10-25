Texture2D diffuseTexture : register(t0);
SamplerState samplerLinear : register(s0);

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 view;
    row_major float4x4 projection;
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(world, float4(input.position, 1.0f));
    float4 viewPos = mul(view, worldPos);
    output.position = mul(projection, viewPos);
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 texColor = diffuseTexture.Sample(samplerLinear, input.texcoord);
    bool hasTex = any(texColor.rgb > 0.001);
    float4 base = hasTex ? texColor * input.color : input.color;
    return base;
}