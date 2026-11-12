#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <dxcapi.h>

#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "Engine_Struct.h"

ENGINE_START
class ENGINE_DLL ShaderResource final : public Interface::EngineResource {
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
    
public:
    static const TypeInfo& StaticType() noexcept {
        static const TypeInfo type{ "ENGINE::ShaderResource", &Interface::EngineResource::StaticType() };
        return type;
    }

    const TypeInfo& GetType() const noexcept override { return StaticType(); }
    
private:
    ShaderDesc  m_ShaderDesc{};
    Microsoft::WRL::ComPtr<IDxcBlob> m_pByteCode;
};
ENGINE_END