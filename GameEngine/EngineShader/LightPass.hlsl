Texture2D<float4> AlbedoTexture   : register(t0);
Texture2D<float4> NormalTexture   : register(t1);
Texture2D<float4> MaterialTexture : register(t2);
Texture2D<float>  DepthTexture    : register(t3);

cbuffer LightingBuffer : register(b0)
{
    row_major float4x4 InverseViewProjection;
    float4 CameraPosition;
    float4 LightDirectionIntensity;
    float4 LightColorAmbient;
};

struct VSOutput
{
    float4 Position : SV_Position;
};

VSOutput VSMain(uint VertexID : SV_VertexID)
{
    VSOutput Output;

    float2 UV = float2(
        (VertexID << 1) & 2,
        VertexID & 2);

    Output.Position = float4(
        UV.x * 2.f - 1.f,
        1.f - UV.y * 2.f,
        0.f, 1.f);

    return Output;
}

float3 SafeNormalize(float3 Value)
{
    return Value * rsqrt(max(dot(Value, Value), 0.00000001f));
}

float3 ReconstructWorldPosition(float2 UV, float Depth)
{
    float4 ClipPosition = float4(
        UV.x * 2.f - 1.f,
        1.f - UV.y * 2.f,
        Depth,
        1.f);

    float4 WorldPosition =
        mul(ClipPosition, InverseViewProjection);

    return WorldPosition.xyz / WorldPosition.w;
}

float4 PSMain(VSOutput Input) : SV_Target0
{
    int2 Pixel = int2(Input.Position.xy);
    float Depth = DepthTexture.Load(int3(Pixel, 0));

    if (Depth >= 1.f)
        return float4(0.f, 0.f, 0.f, 1.f);

    uint Width, Height;
    DepthTexture.GetDimensions(Width, Height);

    float2 UV = Input.Position.xy / float2(Width, Height);

    float3 Position = ReconstructWorldPosition(UV, Depth);
    float3 Albedo = AlbedoTexture.Load(int3(Pixel, 0)).rgb;
    float3 Normal = SafeNormalize(
        NormalTexture.Load(int3(Pixel, 0)).xyz);

    float3 Material = saturate(
        MaterialTexture.Load(int3(Pixel, 0)).rgb);

    float Metallic = Material.r;
    float Roughness = max(Material.g, 0.04f);
    float AO = Material.b;

    float3 L = SafeNormalize(-LightDirectionIntensity.xyz);
    float3 V = SafeNormalize(CameraPosition.xyz - Position);
    float3 H = SafeNormalize(L + V);

    float NdotL = saturate(dot(Normal, L));
    float NdotV = saturate(dot(Normal, V));
    float NdotH = saturate(dot(Normal, H));

    // 간단한 Blinn-Phong 기반 반사광.
    float Shininess = max(2.f / (Roughness * Roughness) - 2.f, 1.f);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);

    float3 Diffuse = Albedo * (1.f - Metallic) / 3.14159265f;
    float3 Specular =
        F0 * ((Shininess + 8.f) / (8.f * 3.14159265f))
        * pow(NdotH, Shininess);

    float3 Radiance =
        LightColorAmbient.rgb * LightDirectionIntensity.w;

    float3 Direct = (Diffuse + Specular) * Radiance * NdotL;

    if (NdotV <= 0.f)
        Direct = 0.f;

    float3 Ambient =
        Albedo * (1.f - Metallic) * LightColorAmbient.w * AO;

    // HDR 선형 색상 유지. 여기서 감마 변환하지 않음.
    return float4(Direct + Ambient, 1.f);
}