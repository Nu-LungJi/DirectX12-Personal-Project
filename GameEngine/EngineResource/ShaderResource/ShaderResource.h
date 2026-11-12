#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <dxcapi.h>

#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"
#include "EngineCommonHeader/Engine_Struct.h"

ENGINE_START
class ShaderResource final : public Interface::EngineResource {
    friend class ShaderManager;
public:
    explicit ShaderResource(const ShaderDesc& Desc) : m_ShaderDesc(Desc) {}
    ~ShaderResource() = default;
    
public:
    [[nodiscard("Warning : Use ShaderDesc Value.")]]
    const ShaderDesc& Get_ShaderDesc() const noexcept   { return m_ShaderDesc; }
    VOID  Set_ShaderDesc(const ShaderDesc& Desc)        { m_ShaderDesc = Desc; }
    
    [[nodiscard("Warning : Use ByteCode Value.")]]
    D3D12_SHADER_BYTECODE Get_ByteCode() const noexcept {
        if (!m_pByteCode) return {};
        return { m_pByteCode->GetBufferPointer(), m_pByteCode->GetBufferSize() };
    }
    
private:
    ShaderDesc  m_ShaderDesc{};
    Microsoft::WRL::ComPtr<IDxcBlob> m_pByteCode;
};
ENGINE_END