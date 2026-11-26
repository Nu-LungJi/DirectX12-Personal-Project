cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;
    row_major float4x4 WorldInverseTranspose;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position    : SV_Position;
    float3 WorldNormal : NORMAL;
};

struct PSOutput
{
    float4 Albedo   : SV_Target0;
    float4 Normal   : SV_Target1;
    float4 Material : SV_Target2;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;

    const float4 WorldPosition =
        mul(float4(Input.Position, 1.f), World);

    Output.Position = mul(WorldPosition, ViewProjection);

    Output.WorldNormal = mul(
        Input.Normal,
        (float3x3)WorldInverseTranspose);

    return Output;
}

PSOutput PSMain(VSOutput Input)
{
    PSOutput Output;

    // 선형 공간의 고정 기본 색상.
    Output.Albedo = float4(0.7f, 0.7f, 0.7f, 1.f);

    // 월드 공간 노멀을 -1~1 그대로 저장.
    Output.Normal = float4(normalize(Input.WorldNormal), 0.f);

    // R: Metallic, G: Roughness, B: AO, A: 예약.
    Output.Material = float4(0.f, 0.5f, 1.f, 0.f);

    return Output;
}