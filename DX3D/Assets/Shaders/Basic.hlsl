
#define USE_GAMMA 0          // 1 щоб увімкнути sRGB->Linear->sRGB
//#define DEBUG_NL

static const float SpecStrength = 0.3f;
static const float SpecPower = 32.0f;

Texture2D diffuseTexture : register(t0);
SamplerState samplerLinear : register(s0);

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 view;
    row_major float4x4 projection;
};
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
    float ambientIntensity;
    int lightCount;
    float3 _padding;
}

struct Light
{
    float4 posRange; // xyz = position, w = range
    float4 dirSpot; // xyz = direction (normalized), w = spotAngle (radians or degrees — як у тебе)
    float4 colInt; // rgb = color (linear), w = intensity
    int type; // 0=Dir, 1=Point, 2=Spot
    int3 _pad; // вирівнювання до 16 байт
};
StructuredBuffer<Light> Lights : register(t1);

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
    float3 worldPos : POSITION1;
    float3 normal : NORMAL0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(world, float4(input.position, 1.0f));
    o.worldPos = wp.xyz;

    o.normal = normalize(mul((float3x3) world, input.normal));

    float4 vp = mul(view, wp);
    o.position = mul(projection, vp);
    o.texcoord = input.texcoord;
    o.color = input.color;
    return o;
}

float3 ComputeLighting(float3 baseColor, float3 N, float3 V, float3 worldPos)
{
    float3 sum = 0.0f;

    [loop]
    for (int i = 0; i < lightCount; ++i)
    {
        Light l = Lights[i];

        float3 L;
        float atten = 1.0f;

        if (l.type == 0) // Directional
        {
            L = normalize(-l.dirSpot.xyz);
        }
        else
        {
            float3 toL = l.posRange.xyz - worldPos;
            float dist = length(toL);
            L = (dist > 1e-4f) ? (toL / dist) : float3(0, 0, 0);

    // квадратичне затухання
            float att = 1.0f / (1.0f + 0.22f * dist + 0.20f * dist * dist);

    // м’яке відсікання по радіусу
            float rangeGate = 1.0f - smoothstep(l.posRange.w * 0.8f, l.posRange.w, dist);
            atten = att * rangeGate;
        }

        float NdotL = max(dot(N, L), 0.0f);

// Diffuse
        float3 diffuse = baseColor * l.colInt.rgb * NdotL;

// Specular
        float3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0f), SpecPower) * NdotL;
        float3 specular = l.colInt.rgb * spec * SpecStrength;

        sum += (diffuse + specular) * l.colInt.w * atten; // інтенсивність у colInt.w
    }

    return sum;
}

float3 toLinear(float3 c)
{
    return pow(c, 2.2f);
}
float3 toGamma(float3 c)
{
    return pow(c, 1.0f / 2.2f);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 tex = diffuseTexture.Sample(samplerLinear, input.texcoord);
    float4 baseColor = (any(tex.rgb > 0.001) ? tex : 1.0) * input.color;

#if USE_GAMMA
    float3 baseLin = toLinear(baseColor.rgb);
#else
    float3 baseLin = baseColor.rgb;
#endif

    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPos - input.worldPos);

    float3 ambient = baseLin * ambientIntensity;
    float3 lighting = ComputeLighting(baseLin, N, V, input.worldPos);
    float3 finalLin = ambient + lighting;

#if USE_GAMMA
    float3 final = toGamma(saturate(finalLin));
#else
    float3 final = saturate(finalLin);
#endif

#ifdef DEBUG_NL
    float3 debugNL = dot(N, float3(0, -1, 0)).xxx;
    return float4(debugNL.xxx * 0.5 + 0.5, 1.0);
#endif
    
    return float4(final, baseColor.a);
}
