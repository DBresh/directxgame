cbuffer TransformBuffer : register(b0)
{
    float4x4 world; // Ignored for instanced rendering
    float4x4 view;
    float4x4 projection;
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    
    // Instance Data (Mapped to Slot 1 in C++)
    float4 instMatrix0 : TEXCOORD4;
    float4 instMatrix1 : TEXCOORD5;
    float4 instMatrix2 : TEXCOORD6;
    float4 instMatrix3 : TEXCOORD7;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    float4 color : TEXCOORD3;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    
    // Construct the 4x4 instance matrix from the four input registers.
    // Because we pass the raw DirectXMath matrix (row-major) directly into the 
    // vertex buffer without transposing, this mapping works perfectly.
    float4x4 instanceWorld = float4x4(
        input.instMatrix0,
        input.instMatrix1,
        input.instMatrix2,
        input.instMatrix3
    );
    
    float4 localPos = float4(input.position, 1.0f);
    
    float4 worldPos4 = mul(localPos, instanceWorld);
    float4 viewPos4 = mul(worldPos4, view);
    float4 projPos4 = mul(viewPos4, projection);
    
    o.position = projPos4;
    o.worldPos = worldPos4.xyz;
    
    // Transform normals using the instance matrix
    o.normal = normalize(mul(input.normal, (float3x3) instanceWorld));
    
    o.texcoord = input.texcoord;
    o.color = input.color;
    
    return o;
}