Texture2D<float4> SourceTexture : register(t0);
SamplerState PointSampler : register(s0);

cbuffer DebugViewBuffer : register(b0)
{
    uint Mode;
    float NearZ;
    float FarZ;
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

float LinearToSRGB(float Value)
{
    Value = saturate(Value);

    return Value <= 0.0031308f
        ? Value * 12.92f
        : 1.055f * pow(Value, 1.f / 2.4f) - 0.055f;
}

float4 PSMain(VSOutput Input) : SV_Target0
{
    float4 Value = SourceTexture.SampleLevel(PointSampler, Input.UV, 0.f);

    if (Mode == 1) {
        if (dot(Value.xyz, Value.xyz) < 0.000001f)
            return float4(0.f, 0.f, 0.f, 1.f);

        return float4(normalize(Value.xyz) * 0.5f + 0.5f, 1.f);
    }

    // Material: R=Metallic, G=Roughness, B=AO.
    if (Mode == 2) return float4(Value.rgb, 1.f);

    // 일반 깊이 방식: ClearDepth=1, LESS.
    if (Mode == 3) {
        if (Value.r >= 1.f) return float4(0.f, 0.f, 0.f, 1.f);

        float ViewDepth = NearZ * FarZ / (FarZ - Value.r * (FarZ - NearZ));

        // 넓은 깊이 범위를 확인하기 위한 로그 표시.
        float Gray = 1.f - saturate(log2(1.f + ViewDepth) / log2(1.f + FarZ));

        return float4(Gray.xxx, 1.f);
    }

    float3 Color = max(Value.rgb, 0.f);

    // SceneColor의 임시 HDR 미리보기.
    if (Mode == 4) Color = Color / (1.f + Color);

    return float4(LinearToSRGB(Color.r), LinearToSRGB(Color.g), LinearToSRGB(Color.b), 1.f);
}