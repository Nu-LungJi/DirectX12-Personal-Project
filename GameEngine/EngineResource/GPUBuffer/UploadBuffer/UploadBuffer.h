#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class ENGINE_DLL UploadBuffer final : public Interface::EngineResource, Interface::Uncopyable {
public:
    UploadBuffer() = default;
    ~UploadBuffer() override;
    
public:
    HRESULT Initialize(ID3D12Device* Device, uint32_t ElementSize, uint32_t ElementCount,
        uint32_t Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    
    HRESULT Write(uint32_t Index, const void* Data, uint32_t DataSize);

    D3D12_GPU_VIRTUAL_ADDRESS Get_GPUAddress(uint32_t Index) const;

    void    ShutDown();
    
private:
    HRESULT Initialize_Layout(uint32_t ElementSize, uint32_t ElementCount, uint32_t Alignment);

    HRESULT Create_Resource(ID3D12Device* Device);
    HRESULT Map_Resource();
    
private:
    ComPtr<ID3D12Resource> m_Resource;

    uint8_t* m_MappedData = nullptr;

    uint32_t m_ElementSize   = 0;
    uint32_t m_ElementCount  = 0;
    uint64_t m_ElementStride = 0;
    uint64_t m_BufferSize    = 0;
};
ENGINE_END