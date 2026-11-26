#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "ShaderResource.h"

ENGINE_START
class GraphicDevice;
class ShaderManager;
class RenderManager;

class ENGINE_DLL ToneMappingPass final : public Interface::EngineResource, public  Interface::Uncopyable {
public:
    ToneMappingPass() = default;
    ~ToneMappingPass() override;
    
public:
    HRESULT Initialize(const GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Render(RenderManager& Manager);
    void    ShutDown();

    HRESULT Set_Exposure(float Exposure);
    
private:
    HRESULT Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Create_RootSignature(ID3D12Device* Device);
    HRESULT Create_PipelineState(const GraphicDevice& Device);

    void Bind_BackBuffer(ID3D12GraphicsCommandList* CommandList, const GraphicDevice& Device) const;
    
private:
    std::unique_ptr<ShaderResource> m_VertexShader;
    std::unique_ptr<ShaderResource> m_PixelShader;

    ComPtr<ID3D12RootSignature>     m_RootSignature;
    ComPtr<ID3D12PipelineState>     m_PipelineState;
    
    float m_fExposure = 1.f;
};
ENGINE_END