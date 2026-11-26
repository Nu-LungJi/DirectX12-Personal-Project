#include "MeshResource.h"

#include "EngineUtility.h"
ENGINE_USE


HRESULT MeshResource::Initialize(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const Vertex* VertexList,
                                 uint32_t VertexCount, const uint32_t* IndexList, uint32_t IndexCount) {
    if (!Device || !CommandList || !VertexList || VertexCount == 0) return E_INVALIDARG;
    
    if (IndexCount > 0 && !IndexList)           return E_INVALIDARG;
    if (m_pVertexBuffer || m_pIndexBuffer)   return E_UNEXPECTED;
    
    constexpr uint64_t VertexSize = sizeof(Vertex);
    constexpr uint64_t uint32Size = sizeof(uint32_t);
    
    const uint64_t VertexByteCount  = VertexSize  * static_cast<uint64_t>(VertexCount);
    const uint64_t IndexByteCount   = uint32Size  * static_cast<uint64_t>(IndexCount);

    if (VertexByteCount > UINT_MAX || IndexByteCount > UINT_MAX) return E_INVALIDARG;

    HRESULT Result = CreateBuffer(Device, VertexList, VertexByteCount, m_pVertexBuffer, m_pVertexUploadBuffer);
    if (SUCCEEDED(Result) && IndexCount > 0) {
        Result = CreateBuffer(Device, IndexList, IndexByteCount, m_pIndexBuffer, m_pIndexUploadBuffer);
    }
    
    if (FAILED(Result)) {
        m_pVertexBuffer.Reset();
        m_pIndexBuffer.Reset();
        Release_UploadBuffer();
        return Result;
    }
    
    CopyBuffer(CommandList, m_pVertexBuffer.Get(), m_pVertexUploadBuffer.Get(), VertexByteCount, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    m_VertexBufferView = {
        m_pVertexBuffer->GetGPUVirtualAddress(),
        static_cast<uint32_t>(VertexByteCount),
        static_cast<uint32_t>(VertexSize)
    };

    if (IndexCount > 0)
    {
        CopyBuffer(CommandList, m_pIndexBuffer.Get(), m_pIndexUploadBuffer.Get(), IndexByteCount, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        m_IndexBufferView = {
            m_pIndexBuffer->GetGPUVirtualAddress(), 
            static_cast<uint32_t>(IndexByteCount), 
            DXGI_FORMAT_R32_UINT
        };
    }

    m_iVertexCount  = VertexCount;
    m_iIndexCount   = IndexCount;

    return S_OK;
}

HRESULT MeshResource::Initialize(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const std::vector<Vertex>& VertexList,
    uint32_t VertexCount, const std::vector<uint32_t>& IndexList, uint32_t IndexCount) {
    if (!Device || !CommandList || VertexCount == 0) return E_INVALIDARG;
    if (VertexCount > VertexList.size() || IndexCount > IndexList.size()) return E_INVALIDARG;

    if (m_pVertexBuffer || m_pIndexBuffer) return E_UNEXPECTED;

    constexpr uint64_t VertexSize = sizeof(Vertex);
    constexpr uint64_t uint32Size = sizeof(uint32_t);

    const uint64_t VertexByteCount  = VertexSize * VertexCount;
    const uint64_t IndexByteCount   = uint32Size * IndexCount;

    if (VertexByteCount > UINT_MAX || IndexByteCount > UINT_MAX) return E_INVALIDARG;

    HRESULT Result = CreateBuffer(Device, VertexList.data(), VertexByteCount, m_pVertexBuffer, m_pVertexUploadBuffer);

    if (SUCCEEDED(Result) && IndexCount > 0) {
        Result = CreateBuffer(Device, IndexList.data(), IndexByteCount, m_pIndexBuffer, m_pIndexUploadBuffer);
    }

    if (FAILED(Result)) {
        m_pVertexBuffer.Reset();
        m_pIndexBuffer.Reset();
        Release_UploadBuffer();
        return Result;
    }

    CopyBuffer(CommandList, m_pVertexBuffer.Get(), m_pVertexUploadBuffer.Get(), VertexByteCount, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_VertexBufferView = {
        m_pVertexBuffer->GetGPUVirtualAddress(),
        static_cast<uint32_t>(VertexByteCount),
        static_cast<uint32_t>(VertexSize)
    };

    if (IndexCount > 0) {
        CopyBuffer(CommandList, m_pIndexBuffer.Get(), m_pIndexUploadBuffer.Get(), IndexByteCount, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        m_IndexBufferView = {
            m_pIndexBuffer->GetGPUVirtualAddress(),
            static_cast<uint32_t>(IndexByteCount),
            DXGI_FORMAT_R32_UINT
        };
    }

    m_iVertexCount  = VertexCount;
    m_iIndexCount   = IndexCount;

    return S_OK;
}

void MeshResource::CopyBuffer(ID3D12GraphicsCommandList* CommandList, ID3D12Resource* Buffer, ID3D12Resource* UploadBuffer, uint64_t ByteCount, D3D12_RESOURCE_STATES FinalState) {
    CommandList->CopyBufferRegion(Buffer, 0, UploadBuffer, 0, ByteCount);

    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Buffer, D3D12_RESOURCE_STATE_COPY_DEST, FinalState);

    CommandList->ResourceBarrier(1, &Barrier);
}

HRESULT MeshResource::CreateBuffer(ID3D12Device* Device, const void* Data, uint64_t ByteCount, Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer,
    Microsoft::WRL::ComPtr<ID3D12Resource>& UploadBuffer) {
    
    const CD3DX12_HEAP_PROPERTIES   DefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_HEAP_PROPERTIES   UploadHeap  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC     BufferDesc  = CD3DX12_RESOURCE_DESC::Buffer(ByteCount);

    HRESULT Result = Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &BufferDesc,
       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(Buffer.GetAddressOf()));

    if (FAILED(Result)) return Result;

    Result = Device->CreateCommittedResource(&UploadHeap, D3D12_HEAP_FLAG_NONE, &BufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(UploadBuffer.GetAddressOf()));

    if (FAILED(Result)) return Result;

    void* Mapped = nullptr;
    const D3D12_RANGE ReadRange{0, 0};

    Result = UploadBuffer->Map(0, &ReadRange, &Mapped);
    if (FAILED(Result)) return Result;
    
    std::memcpy(Mapped, Data, static_cast<size_t>(ByteCount));
    UploadBuffer->Unmap(0, nullptr);

    return S_OK;
}

void MeshResource::Release_UploadBuffer() {
    m_pVertexUploadBuffer.Reset();
    m_pIndexUploadBuffer.Reset();
}
