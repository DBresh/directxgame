//#define DEBUG_NL
#define USE_GAMMA 0
#define MAX_LIGHTS 64
//#define SPOT_ANGLE_IS_DEGREES 1   // зараз ми передаємо half-angle в радіанах, тому макрос неактуальний

//#define SHADOW_EXP_ORTHO_W          // експеримент 1: "ортографічна" тінь
//#define SHADOW_EXP_DEPTH_SCALE_UV   // експеримент 2: стискання тіні з глибиною
//#define SHADOW_DBG_SHOW_SHADOW_VAL  // дебаг: показати значення shadow замість кольору
//#define SHADOW_DBG_SHOW_UV          // дебаг: показати UV як колір
//#define SHADOW_DBG_SHOW_DEPTH       // дебаг: показати p.z як колір


static const float SpecStrength = 0.3f;
static const float SpecPower = 32.0f;

Texture2D diffuseTexture : register(t0);
SamplerState samplerLinear : register(s0);

Texture2D shadowMap : register(t2);
SamplerComparisonState shadowSampler : register(s1);

// ==== MATRICES ====
// СТАНДАРТ:
//  - На CPU: матриці рахуємо DirectXMath-ом (row-major), ПЕРЕД аплоадом робимо XMMatrixTranspose.
//  - У cbuffers лежать вже transpose-нуті матриці (GPU-ready column-major).
//  - У HLSL всюди використовуємо mul(vector, matrix) (vec * mat).
cbuffer TransformBuffer : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
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
    float4x4 lightViewProj[MAX_LIGHTS]; // також transpose-нуті на CPU
};

struct Light
{
    float4 posRange; // xyz = position, w = range
    float4 dirSpot; // xyz = direction (normalized), w = spotHalfAngleRadians
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
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    float4 color : TEXCOORD3;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;

    float4 localPos = float4(input.position, 1.0f);

    // У cb лежать transpose-нуті матриці -> vec * mat
    float4 worldPos4 = mul(localPos, world);
    float4 viewPos4 = mul(worldPos4, view);
    float4 projPos4 = mul(viewPos4, projection);

    o.position = projPos4;
    o.worldPos = worldPos4.xyz;

    // Для uniform scale нормалі можна множити на world (top-left 3x3).
    o.normal = normalize(mul(input.normal, (float3x3) world));

    o.texcoord = input.texcoord;
    o.color = input.color;
    return o;
}

float ComputeShadowFromCoord(float4 lightClip, float bias)
{
    // Перехід у NDC
    float3 p = lightClip.xyz / lightClip.w; //max(lightClip.w, 1e-6f);
    float2 uv;
    uv.x = p.x * 0.5f + 0.5f;
    uv.y = -p.y * 0.5f + 0.5f;
    
    // NDC [-1,1] -> UV [0,1]
    //uv = p.xy * 0.5f + 0.5f;
    //uv.y = 1.0f - uv.y; // якщо проекція без фліпа по Y

    // За межами shadow map – вважаємо, що немає тіні
    if (uv.x < 0.0f || uv.x > 1.0f ||
        uv.y < 0.0f || uv.y > 1.0f ||
        p.z < 0.0f || p.z > 1.0f)
    {
        return 1.0f;
    }

    float currentDeep = p.z;
    float deepScale = currentDeep;
    float scaledBias = bias * deepScale;
    float comparedDeep = currentDeep - scaledBias;
    
    //float refZ = p.z - bias;
    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, comparedDeep);
}

float3 ComputeLighting(float3 baseColor, float3 N, float3 V, float3 worldPos)
{
    float3 result = 0.0f;

    [loop]
    for (int i = 0; i < lightCount; ++i)
    {
        Light l = Lights[i];

        // --------------------------
        // 1. Compute light direction L and attenuation
        // --------------------------
        float3 L = 0.0f;
        float atten = 1.0f;

        if (l.type == 0)
        {
            // Directional light
            L = normalize(-l.dirSpot.xyz);
        }
        else
        {
            // Vector from point to light
            float3 toL = l.posRange.xyz - worldPos;
            float dist = length(toL);
            L = (dist > 1e-4f) ? (toL / dist) : float3(0, 0, 0);

            // Distance attenuation
            float distAtt = 1.0f / (1.0f + 0.22f * dist + 0.20f * dist * dist);

            // Smooth cutoff by range
            float rangeGate = 1.0f - smoothstep(l.posRange.w * 0.8f, l.posRange.w, dist);

            atten = distAtt * rangeGate;

            // -----------------
            // Spot cone attenuation
            // -----------------
            if (l.type == 2)
            {
                float halfAngle = l.dirSpot.w;

                float inner = halfAngle * 0.85f;
                float outer = halfAngle;

                float3 lightDir = normalize(l.dirSpot.xyz);

                // direction from light to point
                float3 Lp = normalize(worldPos - l.posRange.xyz);

                float cosTheta = dot(Lp, lightDir);
                float cosInner = cos(inner);
                float cosOuter = cos(outer);

                float spotFactor = saturate((cosTheta - cosOuter) /
                                             max(1e-4f, (cosInner - cosOuter)));

                atten *= spotFactor;
            }
        }

        // --------------------------
        // 2. Diffuse + Specular
        // --------------------------
        float NdotL = max(dot(N, L), 0.0f);

        float3 diffuse = baseColor * l.colInt.rgb * NdotL;

        float3 H = normalize(L + V);
        float specPow = pow(max(dot(N, H), 0.0f), SpecPower) * NdotL;
        float3 specular = l.colInt.rgb * specPow * SpecStrength;

        // --------------------------
        // 3. Shadow calculation (spot only)
        // --------------------------
        float shadow = 1.0f;

        if (l.type == 2)
        {
            float3 vecFromLight = worldPos - l.posRange.xyz;
            float distL = length(vecFromLight);

            float3 lightDir = normalize(l.dirSpot.xyz);
            float3 toL = l.posRange.xyz - worldPos;
            float3 toPointDir = (distL > 1e-4f) ? normalize(vecFromLight) : lightDir;

            float cosTheta = dot(toPointDir, lightDir);
            float cosOuter = cos(l.dirSpot.w);
            
            bool inRange = (distL <= l.posRange.w);
            bool inCone = (cosTheta >= cosOuter);

            
            if (inRange && inCone)
            {
                float4 lightClip = mul(float4(worldPos, 1.0f), lightViewProj[i]);
                
                float normalDeep = lightClip.z / lightClip.w;
               
                float3 toLnorm = normalize(toL);
                float cosAngle = saturate(dot(N, toLnorm));
                
                float slopeScale = sqrt(1.0f - cosAngle * cosAngle) / max(cosAngle, 0.001f);
                float depthScale = normalDeep;
                float finalBias = 0.0001f * depthScale * (1.0f + slopeScale);
                
                shadow = ComputeShadowFromCoord(lightClip, finalBias);
            }
            else
            {
                shadow = 1.0f;
            }
        }

        // --------------------------
        // 4. Accumulate
        // --------------------------
        result += shadow * (diffuse + specular) * l.colInt.w * atten;
    }

    return result;
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

    //float4 lightClipDbg = mul(float4(input.worldPos, 1.0f), lightViewProj[0]);
    //float shadowDbg = ComputeShadowFromCoord(lightClipDbg, 0.0045f);
    //return float4(shadowDbg.xxx, 1.0);
    
    
#if USE_GAMMA
    float3 final = toGamma(saturate(finalLin));
#else
    float3 final = saturate(finalLin);
#endif

    return float4(final, baseColor.a);
}



