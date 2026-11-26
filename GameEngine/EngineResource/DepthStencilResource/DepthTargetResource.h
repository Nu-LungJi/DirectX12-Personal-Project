#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "GraphicDevice.h"

ENGINE_START
struct DepthTargetDesc final  {
    uint32_t DepthTargetWidth = 0;
    uint32_t DepthTargetHeight = 0;
    
    float ClearDepth = 1.f;
};

class ENGINE_DLL DepthTargetResource final : public Interface::EngineResource, public Interface::Uncopyable {
public:
    DepthTargetResource() = default;
    ~DepthTargetResource() override;
    
public:
    HRESULT Initialize(GraphicDevice& Device, const DepthTargetDesc& Desc);
    
    void Transition(D3D12_RESOURCE_STATES State);
    
    void Clear();
    
    void ShutDown();
    
public:
    D3D12_CPU_DESCRIPTOR_HANDLE Get_DSV() const noexcept { return m_DSV.CPU; }
    D3D12_GPU_DESCRIPTOR_HANDLE Get_SRV() const noexcept { return m_SRV.GPU; }
    
    const DepthTargetDesc& Get_DepthTargetDesc() const noexcept { return m_Desc; }
    
public:
    static const TypeInfo& StaticType() noexcept {
        static const TypeInfo type{ "ENGINE::DepthTargetResource", &Interface::EngineResource::StaticType() };
        return type;
    }

    const TypeInfo& GetType() const noexcept override { return StaticType(); }
    
private:
    HRESULT Validate_Desc() const;
    HRESULT Create_Resource();
    HRESULT Create_Views();
    
private:
    ComPtr<ID3D12Resource>  m_pResource;
    DepthTargetDesc         m_Desc{};
    
    DescriptorAllocation    m_DSV{};
    DescriptorAllocation    m_SRV{};
    
    D3D12_RESOURCE_STATES   m_ResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    
    GraphicDevice*          m_pGraphicDevice = nullptr;
};
ENGINE_END