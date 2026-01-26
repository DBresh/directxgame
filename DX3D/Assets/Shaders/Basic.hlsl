#define USE_GAMMA 0
#define MAX_LIGHTS 64

static const float SpecStrength = 0.3f;
static const float SpecPower = 32.0f;

Texture2D diffuseTexture : register(t0);
SamplerState samplerLinear : register(s0);

Texture2DArray shadowMap : register(t2);
SamplerComparisonState shadowSampler : register(s1);

// ==== MATRICES ====
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
    float4x4 lightViewProj[MAX_LIGHTS];
};

struct Light
{
    float4 posRange; // xyz = position, w = range
    float4 dirSpot; // xyz = direction (normalized), w = spotHalfAngleRadians
    float4 colInt; // rgb = color, w = intensity
    int type; // 0=Dir, 1=Point, 2=Spot
    int shadowMapIndex;
    int2 _pad;
};
StructuredBuffer<Light> Lights : register(t1);

cbuffer MaterialBuffer : register(b3)
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 _matPad;
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
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    float4 color : TEXCOORD3;
};

static const float PI = 3.14159265359f;

VSOutput VSMain(VSInput input)
{
    VSOutput o;

    float4 localPos = float4(input.position, 1.0f);

    float4 worldPos4 = mul(localPos, world);
    float4 viewPos4 = mul(worldPos4, view);
    float4 projPos4 = mul(viewPos4, projection);

    o.position = projPos4;
    o.worldPos = worldPos4.xyz;

    o.normal = normalize(mul(input.normal, (float3x3) world));

    o.texcoord = input.texcoord;
    o.color = input.color;
    return o;
}

// --- TONE MAPPING ---
// Narkowicz implementation of ACES Filmic Tone Mapping
// This curve compresses bright HDR values into the 0.0 - 1.0 range gracefully.
float3 ACESFilmicToneMapping(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Calculates the reflection ratio based on viewing angle
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(max(1.0f - cosTheta, 0.0f), 5.0f);
}

// Normal DistributionFunction (GGX)
// Approximates the alignment of microfacets (how "rough" the reflection is)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001f); // prevent divide by zero
}

// Geometry Schlick-GGX (Helper for Smith)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

// Geometry Smith
// Approximates self-shadowing of microfacets
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float ComputeShadowFromCoord(float4 lightClip, int sliceIndex, float3 N, float3 L)
{
    float3 p = lightClip.xyz / max(lightClip.w, 1e-6f);

    float2 uv;
    uv.x = p.x * 0.5f + 0.5f;
    uv.y = -p.y * 0.5f + 0.5f;

    if (uv.x < 0.0f || uv.x > 1.0f ||
        uv.y < 0.0f || uv.y > 1.0f ||
        p.z < 0.0f || p.z > 1.0f)
    {
        return 1.0f;
    }

    // Auto-Bias Calculation (Slope-Scaled)
    float NdotL = saturate(dot(N, L));
    float bias = max(0.005f * (1.0f - NdotL), 0.0005f);
    float currentDepth = p.z - bias;

    // PCF SOFTENING (3x3 Loop)
    // Calculate the size of one texel (1.0 / 2048.0)
    uint width, height, elements;
    shadowMap.GetDimensions(width, height, elements);
    float2 texelSize = 1.0f / float2(width, height);

    float shadowSum = 0.0f;

    // Loop through a 3x3 grid around the current pixel
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            
            // Sample the shadow map at the offset position
            shadowSum += shadowMap.SampleCmp(shadowSampler,
                                             float3(uv + offset, (float) sliceIndex),
                                             currentDepth);
        }
    }

    // Average the 9 samples
    return shadowSum / 9.0f;
}

// Main PBR Lighting Loop
float3 ComputeLighting(float3 albedo, float3 N, float3 V, float3 worldPos, float roughness, float metallic)
{
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // F0: Surface reflection at zero incidence
    // 0.04 is standard for non-metals (dielectrics). 
    // For metals, we use the albedo color itself.
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);

    [loop]
    for (int i = 0; i < lightCount; ++i)
    {
        Light l = Lights[i];
        
        // --- Light Vector & Attenuation ---
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
            
            // Inverse Square Law Attenuation
            float att = 1.0f / (1.0f + 0.22f * dist + 0.20f * dist * dist);
            float rangeGate = 1.0f - smoothstep(l.posRange.w * 0.8f, l.posRange.w, dist);
            atten = att * rangeGate;

            if (l.type == 2) // Spot
            {
                float theta = l.dirSpot.w;
                float outer = theta;
                float inner = theta * 0.85f;
                float3 lightDir = normalize(l.dirSpot.xyz);
                float3 L_toPoint = normalize(-L);
                float cosThetaVal = dot(L_toPoint, lightDir);
                float cosInner = cos(inner);
                float cosOuter = cos(outer);
                float spotFactor = saturate((cosThetaVal - cosOuter) / max(1e-4f, (cosInner - cosOuter)));
                atten *= spotFactor;
            }
        }

        // --- Calculate Shadows ---
        float shadow = 1.0f;
        if (l.shadowMapIndex >= 0)
        {
            float4 lightClip = mul(float4(worldPos, 1.0f), lightViewProj[i]);
            
            shadow = ComputeShadowFromCoord(lightClip, l.shadowMapIndex, N, L);
        }

        // --- PBR Math (Cook-Torrance) ---
        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0f);

        // Calculate D, G, F components
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
           
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;
        
        // kS is just F (Fresnel). kD is the remaining energy.
        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        
        // Multiply kD by 1.0 - metallic. 
        // Metals absorb no diffuse light (they are purely specular).
        kD *= (1.0f - metallic);

        // Combine
        // Note: l.colInt.rgb is light color, l.colInt.w is intensity
        float3 radiance = l.colInt.rgb * l.colInt.w * atten * shadow;

        // NdotL because light hits surface at angle
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    return Lo;
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
    // TEXTURE SAMPLING
    float4 tex = diffuseTexture.Sample(samplerLinear, input.texcoord);
    float4 baseColor = (any(tex.rgb > 0.001) ? tex : 1.0) * input.color;

    // LINEAR CONVERSION
    // Most textures (like .png/.jpg) are saved in sRGB (Gamma Space).
    // We MUST convert them to Linear Space before doing any math.
    float3 baseLin = pow(max(baseColor.rgb, 0.0f), 2.2f);

    // GET MATERIAL DATA (From CBuffer)
    float localRough = roughness;
    float localMetal = metallic;

    // PBR LIGHTING
    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPos - input.worldPos);
    
    // Ambient (Simple Linear Ambient)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * baseLin * ambientIntensity;
    
    // Direct Lighting
    float3 Lo = ComputeLighting(baseLin, N, V, input.worldPos, localRough, localMetal);
    
    float3 finalLin = ambient + Lo;

    // --- POST PROCESSING ---
    
    // Exposure
    float exposure = 1.0f;
    finalLin *= exposure;

    // Tone Mapping (HDR -> LDR)
    // Converts high ranges (e.g., 0 to 500) to displayable (0 to 1)
    float3 finalLDR = ACESFilmicToneMapping(finalLin);

    // Gamma Correction (Linear -> sRGB)
    // Monitors expect sRGB data
    float3 final = pow(max(finalLDR, 0.0f), 1.0f / 2.2f);

    return float4(final, baseColor.a);
}