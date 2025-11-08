Texture2D shadowTex : register(t0);
SamplerState samLinear : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput o;
    o.position = float4(input.position.xy, 0.0f, 1.0f);
    o.texcoord = input.texcoord;
    return o;
}

float4 PSMain(PSInput input) : SV_Target
{
    float depth = shadowTex.Sample(samLinear, input.texcoord).r;
    return float4(depth.xxx, 1.0f);
}
