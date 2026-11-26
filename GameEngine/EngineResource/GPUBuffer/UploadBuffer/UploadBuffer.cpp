#include "UploadBuffer.h"
ENGINE_USE

UploadBuffer::~UploadBuffer() { ShutDown(); }

HRESULT UploadBuffer::Initialize(ID3D12Device* Device, uint32_t ElementSize, uint32_t ElementCount, uint32_t Alignment) {
    if (!Device) return E_INVALIDARG;
    
    if (m_Resource) return E_UNEXPECTED;

    HRESULT Result = Initialize_Layout(ElementSize, ElementCount, Alignment);

    if (SUCCEEDED(Result)) Result = Create_Resource(Device);

    if (SUCCEEDED(Result)) Result = Map_Resource();

    if (FAILED(Result)) ShutDown();
    
    return Result;
}

HRESULT UploadBuffer::Write(uint32_t Index, const void* Data, uint32_t DataSize) {
    if (!m_MappedData) return E_UNEXPECTED;

    if (!Data || Index >= m_ElementCount || DataSize != m_ElementSize) return E_INVALIDARG;

    const size_t Offset = static_cast<size_t>(m_ElementStride * Index);

    std::memcpy(m_MappedData + Offset, Data, DataSize);

    return S_OK;
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::Get_GPUAddress(uint32_t Index) const {
    if (!m_Resource || Index >= m_ElementCount) return 0;

    return m_Resource->GetGPUVirtualAddress() + m_ElementStride * Index;
}

void UploadBuffer::ShutDown() {
    if (m_Resource && m_MappedData) m_Resource->Unmap(0, nullptr);

    m_MappedData = nullptr;
    m_Resource.Reset();

    m_ElementSize   = 0;
    m_ElementCount  = 0;
    m_ElementStride = 0;
    m_BufferSize    = 0;
}

HRESULT UploadBuffer::Initialize_Layout(uint32_t ElementSize, uint32_t ElementCount, uint32_t Alignment) {
    if (ElementSize == 0 || ElementCount == 0 || Alignment == 0 || (Alignment & (Alignment - 1)) != 0)
        return E_INVALIDARG;

    const uint64_t Mask = static_cast<uint64_t>(Alignment) - 1;
    const uint64_t Stride = (static_cast<uint64_t>(ElementSize) + Mask) & ~Mask;

    const uint64_t MaxSize = static_cast<uint64_t>((std::numeric_limits<size_t>::max)());

    if (Stride > MaxSize / ElementCount) return E_INVALIDARG;

    m_ElementSize   = ElementSize;
    m_ElementCount  = ElementCount;
    m_ElementStride = Stride;
    m_BufferSize    = Stride * ElementCount;
    
    return S_OK;
}

HRESULT UploadBuffer::Create_Resource(ID3D12Device* Device) {
    const auto Heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const auto Desc = CD3DX12_RESOURCE_DESC::Buffer(m_BufferSize);

    return Device->CreateCommittedResource(&Heap, D3D12_HEAP_FLAG_NONE, &Desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_Resource.GetAddressOf()));
}

HRESULT UploadBuffer::Map_Resource() {
    const D3D12_RANGE ReadRange{0, 0};
    void* Data = nullptr;

    const HRESULT Result = m_Resource->Map(0, &ReadRange, &Data);
    if (SUCCEEDED(Result)) m_MappedData = static_cast<uint8_t*>(Data);

    return Result;
}
