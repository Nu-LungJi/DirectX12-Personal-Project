#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "GraphicDevice.h"

ENGINE_START
struct RenderTargetDesc final {
    uint32_t RenderTargetWidth = 0;
    uint32_t RenderTargetHeight = 0;

    DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    float ClearColor[4] = {0.f, 0.f, 0.f, 1.f};
};

class ENGINE_DLL RenderTargetResource final : public Interface::EngineResource, public Interface::Uncopyable {
public:
    RenderTargetResource() = default;
    ~RenderTargetResource() override;
    
public:
    HRESULT Initialize(GraphicDevice& Device, const RenderTargetDesc& Desc);
    
    void Transition(D3D12_RESOURCE_STATES State);
    
    void Clear();
    
    void ShutDown();
    
public:
    D3D12_CPU_DESCRIPTOR_HANDLE Get_RTV() const noexcept { return m_RTV.CPU; }
    D3D12_GPU_DESCRIPTOR_HANDLE Get_SRV() const noexcept { return m_SRV.GPU; }
    
    const RenderTargetDesc& Get_RenderTargetDesc() const noexcept { return m_Desc; }
    
public:
    static const TypeInfo& StaticType() noexcept {
        static const TypeInfo type{ "ENGINE::RenderTargetResource", &Interface::EngineResource::StaticType() };
        return type;
    }

    const TypeInfo& GetType() const noexcept override { return StaticType(); }
    
private:
    HRESULT Validate_Desc() const;
    HRESULT Create_Resource();
    HRESULT Create_Views();
    
private:
    ComPtr<ID3D12Resource>  m_pResource;
    RenderTargetDesc        m_Desc{};
    
    DescriptorAllocation    m_RTV{};
    DescriptorAllocation    m_SRV{};
    
    D3D12_RESOURCE_STATES   m_ResourceState = D3D12_RESOURCE_STATE_COMMON;
    
    GraphicDevice*          m_pGraphicDevice = nullptr;
};
ENGINE_END