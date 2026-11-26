#pragma once
#include "Engine_Default.h"

inline constexpr wchar_t VertexShader_MainEntryPoint[]   = L"VSMain";
inline constexpr wchar_t PixelShader_MainEntryPoint[]    = L"PSMain";
inline constexpr wchar_t ComputeShader_MainEntryPoint[]  = L"CSMain";

inline constexpr wchar_t VertexShader_DefaultProfile[]   = L"vs_6_7";
inline constexpr wchar_t PixelShader_DefaultProfile[]    = L"ps_6_7";
inline constexpr wchar_t ComputeShader_DefaultProfile[]  = L"cs_6_7";

ENGINE_START

struct ShaderDesc final {
    ShaderDesc() = default;
    ShaderDesc(const std::wstring& FilePath, const std::wstring& EntryPoint, const std::wstring& TargetProfile)
        : m_sFilePath(FilePath), m_sEntryPoint(EntryPoint), m_sTargetProfile(TargetProfile) {}
        
    
    std::wstring m_sFilePath;       // XXX.hlsl, XXX.hlsli
    std::wstring m_sEntryPoint;     // VSMain, PSMain, CSMain
    std::wstring m_sTargetProfile;  // vs_6_0, ps_6_0, cs_6_0
};

struct TypeInfo {
    std::string_view m_sName;
    const TypeInfo* m_pParent = nullptr;

    bool IsA(const TypeInfo& target) const noexcept {
        for (auto* type = this; type; type = type->m_pParent) 
            if (type->m_sName == target.m_sName) return true;

        return false;
    }
};

struct Vertex {
    DirectX::XMFLOAT3A Position;
    DirectX::XMFLOAT3A Normal;
    DirectX::XMFLOAT2A TexCoord;
};

struct TransformConstants final {
    DirectX::XMFLOAT4X4 World;
    DirectX::XMFLOAT4X4 ViewProjection;
    DirectX::XMFLOAT4X4 WorldInverseTranspose;

    TransformConstants() {
        DirectX::XMStoreFloat4x4(&World, DirectX::XMMatrixIdentity());

        ViewProjection = World;
        WorldInverseTranspose = World;
    }
};

static_assert(sizeof(TransformConstants) == 192);

ENGINE_END