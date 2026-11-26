#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "ShaderResource.h"

ENGINE_START
enum class DebugViewMode : uint32_t {
    Albedo,
    Normal,
    Material,
    Depth,
    SceneColor
};

struct DebugViewConstants {
    uint32_t Mode = 0;
    float NearZ = 0.1f;
    float FarZ = 1000.f;
};

static_assert(sizeof(DebugViewConstants) == 12);

class RenderManager;
class GraphicDevice;
class ShaderManager;

class ENGINE_DLL DebugView final : public Interface::EngineResource, public Interface::Uncopyable {
public:
    DebugView() = default;
    ~DebugView() override;
public:
    HRESULT Initialize(const GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Render(RenderManager& Manager);
    void    ShutDown();
    
public:
    void Set_Mode(DebugViewMode Mode) {
        m_View.Mode = static_cast<uint32_t>(Mode);
    }

    void Set_DepthRange(float NearZ, float FarZ) {
        m_View.NearZ = NearZ;
        m_View.FarZ = FarZ;
    }
    
private:
    HRESULT Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Create_RootSignature(ID3D12Device* Device);
    HRESULT Create_PipelineState(const GraphicDevice& Device);

    void    Bind_BackBuffer(ID3D12GraphicsCommandList* CommandList, const GraphicDevice& Device) const;
    
    HRESULT Bind_Source(RenderManager& Manager, ID3D12GraphicsCommandList* CommandList);
    
private:
    std::unique_ptr<ShaderResource> m_VertexShader;
    std::unique_ptr<ShaderResource> m_PixelShader;

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    
    DebugViewConstants m_View{};
};
ENGINE_END