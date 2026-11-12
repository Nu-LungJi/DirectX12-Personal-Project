#pragma once

#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "Engine_Struct.h"

ENGINE_START
class ENGINE_DLL MeshResource final : public Interface::EngineResource {
public:
    MeshResource() = default;
    ~MeshResource() override = default;
    
    MeshResource(const MeshResource&) = delete;
    MeshResource& operator=(const MeshResource&) = delete;

public:
    HRESULT Initialize(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const Vertex* VertexList, uint32_t VertexCount,
        const uint32_t* IndexList, uint32_t IndexCount);
    
    HRESULT Initialize(ID3D12Device* Device, ID3D12GraphicsCommandList* CommandList, const std::vector<Vertex>& VertexList, uint32_t VertexCount,
        const std::vector<uint32_t>& IndexList, uint32_t IndexCount);
    
    HRESULT CreateBuffer(ID3D12Device* Device, const void* Data, uint64_t ByteCount, Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer, Microsoft::WRL::ComPtr<ID3D12Resource>& UploadBuffer);
    
    VOID    CopyBuffer(ID3D12GraphicsCommandList* CommandList,ID3D12Resource* Buffer, ID3D12Resource* UploadBuffer, uint64_t ByteCount, D3D12_RESOURCE_STATES FinalState);
    
    VOID Release_UploadBuffer() const;
    
public:
    const D3D12_VERTEX_BUFFER_VIEW Get_VertexBufferView() const noexcept { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW Get_IndexBufferView() const noexcept   { return m_IndexBufferView;  }
    
    const uint32_t Get_VertexCount() const noexcept { return m_iVertexCount; }
    const uint32_t Get_IndexCount() const noexcept { return m_iIndexCount; }
    
public:
    static const TypeInfo& StaticType() noexcept {
        static const TypeInfo type{ "ENGINE::MeshResource", &Interface::EngineResource::StaticType() };
        return type;
    }

    const TypeInfo& GetType() const noexcept override { return StaticType(); }
    
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexUploadBuffer;
    
    D3D12_VERTEX_BUFFER_VIEW    m_VertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW     m_IndexBufferView{};
    
    uint32_t m_iVertexCount;
    uint32_t m_iIndexCount;
};
ENGINE_END