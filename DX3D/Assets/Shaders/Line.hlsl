cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    matrix mvp = mul(World, mul(View, Projection));
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.color = input.color;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return input.color;
}