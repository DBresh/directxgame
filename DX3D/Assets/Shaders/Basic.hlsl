//#define DEBUG_NL
#define USE_GAMMA 0
#define MAX_LIGHTS 64
#define SPOT_ANGLE_IS_DEGREES 1

static const float SpecStrength = 0.3f;
static const float SpecPower = 32.0f;

Texture2D diffuseTexture : register(t0);
SamplerState samplerLinear : register(s0);

Texture2D shadowMap : register(t2);
SamplerComparisonState shadowSampler : register(s1);

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
};

cbuffer LightMatrixBuffer : register(b2)
{
    row_major float4x4 lightViewProj[MAX_LIGHTS];
};

struct Light
{
    float4 posRange; // xyz = position, w = range
    float4 dirSpot; // xyz = direction, w = spotAngle
    float4 colInt; // rgb = color, w = intensity
    int type; // 0=Dir, 1=Point, 2=Spot
    int3 _pad;
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
    float4 posLight : TEXCOORD1;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(float4(input.position, 1.0f), world);
    o.normal = normalize(mul(input.normal, (float3x3) world));
    float4 vp = mul(wp, view);
    o.position = mul(vp, projection);
    o.posLight = mul(wp, lightViewProj[0]);
    o.worldPos = wp.xyz;
    o.texcoord = input.texcoord;
    o.color = input.color;
    return o;
}

float ComputeShadow(float4 worldPosLightSpace)
{
    float3 p = worldPosLightSpace.xyz / max(worldPosLightSpace.w, 1e-6f);
    if (p.x < -1.0f || p.x > 1.0f ||
        p.y < -1.0f || p.y > 1.0f ||
        p.z < 0.0f || p.z > 1.0f)
        return 1.0f;
    float2 uv = p.xy * float2(0.5f, -0.5f) + 0.5f;
    const float depthBias = 0.0015f;
    float refZ = p.z - depthBias;
    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, refZ);
}


float3 ComputeLighting(float3 baseColor, float3 N, float3 V, float3 worldPos, float4 posLight)
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
        else // Point або Spot
        {
            float3 toL = l.posRange.xyz - worldPos;
            float dist = length(toL);
            L = (dist > 1e-4f) ? (toL / dist) : float3(0, 0, 0);

            float att = 1.0f / (1.0f + 0.22f * dist + 0.20f * dist * dist);
            float rangeGate = 1.0f - smoothstep(l.posRange.w * 0.8f, l.posRange.w, dist);
            atten = att * rangeGate;

            if (l.type == 2) // Spot
            {
                float spotAngle = l.dirSpot.w;
#if SPOT_ANGLE_IS_DEGREES
                spotAngle = radians(spotAngle);
#endif
                float outer = max(1e-4f, spotAngle);
                float inner = outer * 0.85f;

                float3 lightDir = normalize(l.dirSpot.xyz);
                float cosTheta = dot(normalize(-L), lightDir);
                float cosInner = cos(inner);
                float cosOuter = cos(outer);
                float spotFactor = saturate((cosTheta - cosOuter) / max(1e-4f, (cosInner - cosOuter)));
                atten *= spotFactor;
            }
        }

        float NdotL = max(dot(N, L), 0.0f);
        float3 diffuse = baseColor * l.colInt.rgb * NdotL;

        float3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0f), SpecPower) * NdotL;
        float3 specular = l.colInt.rgb * spec * SpecStrength;

        float shadow = 1.0f;
        if (l.type == 2 && i == 0)
        {
            shadow = ComputeShadow(posLight);
        }
           
        sum += shadow * (diffuse + specular) * l.colInt.w * atten;
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
    float3 lighting = ComputeLighting(baseLin, N, V, input.worldPos, input.posLight);
    float3 finalLin = ambient + lighting;

#if USE_GAMMA
    float3 final = toGamma(saturate(finalLin));
#else
    float3 final = saturate(finalLin);
#endif

    return float4(final, baseColor.a);
}