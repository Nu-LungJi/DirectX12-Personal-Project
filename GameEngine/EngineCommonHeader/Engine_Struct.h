#pragma once
#include "Engine_Default.h"

inline constexpr wchar_t VertexShader_MainEntryPoint[]   = L"VSMain";
inline constexpr wchar_t PixelShader_MainEntryPoint[]    = L"PSMain";
inline constexpr wchar_t ComputeShader_MainEntryPoint[]  = L"CSMain";

inline constexpr wchar_t VertexShader_DefaultProfile[]   = L"vs_6_0";
inline constexpr wchar_t PixelShader_DefaultProfile[]    = L"ps_6_0";
inline constexpr wchar_t ComputeShader_DefaultProfile[]  = L"cs_6_0";

ENGINE_START

struct ShaderDesc final {
    ShaderDesc() = default;
    ShaderDesc(const wstring& FilePath, const wstring& EntryPoint, const wstring& TargetProfile)
        : m_sFilePath(FilePath), m_sEntryPoint(EntryPoint), m_sTargetProfile(TargetProfile) {}
        
    
    std::wstring m_sFilePath;       // XXX.hlsl, XXX.hlsli
    std::wstring m_sEntryPoint;     // VSMain, PSMain, CSMain
    std::wstring m_sTargetProfile;  // vs_6_0, ps_6_0, cs_6_0
};

ENGINE_END