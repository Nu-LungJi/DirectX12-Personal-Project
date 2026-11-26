Texture2D<float4> SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer ToneMappingBuffer : register(b0)
{
    float Exposure;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
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

    Output.UV = UV;
    return Output;
}

float3 ToneMap(float3 Color)
{
    // ACES 스타일 근사 곡선.
    return saturate(
        (Color * (2.51f * Color + 0.03f)) /
        (Color * (2.43f * Color + 0.59f) + 0.14f));
}

float LinearToSRGB(float Value)
{
    return Value <= 0.0031308f
        ? Value * 12.92f
        : 1.055f * pow(Value, 1.f / 2.4f) - 0.055f;
}

float4 PSMain(VSOutput Input) : SV_Target0
{
    float3 Color = SceneTexture.SampleLevel(
        LinearSampler, Input.UV, 0.f).rgb;

    Color = ToneMap(max(Color, 0.f) * Exposure);

    return float4(
        LinearToSRGB(Color.r),
        LinearToSRGB(Color.g),
        LinearToSRGB(Color.b),
        1.f);
}