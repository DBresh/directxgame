cbuffer TransformBuffer : register(b0)
{
    float4x4 lightViewProj;
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    
    float4 instMatrix0 : TEXCOORD4;
    float4 instMatrix1 : TEXCOORD5;
    float4 instMatrix2 : TEXCOORD6;
    float4 instMatrix3 : TEXCOORD7;
};

struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    
    float4x4 instanceWorld = float4x4(
        input.instMatrix0,
        input.instMatrix1,
        input.instMatrix2,
        input.instMatrix3
    );
    
    float4 localPos = float4(input.position, 1.0f);
    float4 worldPos = mul(localPos, instanceWorld);
    
    o.position = mul(worldPos, lightViewProj);
    
    return o;
}