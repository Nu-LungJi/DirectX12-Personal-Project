#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "ShaderResource.h"

ENGINE_START
class GraphicDevice;
class ShaderManager;
class RenderManager;

struct LightingConstants {
    DirectX::XMFLOAT4X4 InverseViewProjection;

    DirectX::XMFLOAT4 CameraPosition{0.f, 0.f, -2.f, 0.f};

    // xyz: 빛이 진행하는 방향, w: 세기.
    DirectX::XMFLOAT4 LightDirectionIntensity{0.f, -1.f, 1.f, 3.f};

    // rgb: 선형 색상, w: 간접광 근사 세기.
    DirectX::XMFLOAT4 LightColorAmbient{1.f, 1.f, 1.f, 0.03f};

    LightingConstants() { DirectX::XMStoreFloat4x4(&InverseViewProjection, DirectX::XMMatrixIdentity()); }
};
static_assert(sizeof(LightingConstants) == 112);

class ENGINE_DLL LightPass final : public Interface::EngineResource, public  Interface::Uncopyable {
public:
    LightPass() = default;
    ~LightPass() override;
    
public:
    HRESULT Initialize(const GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& ShaderPath, DXGI_FORMAT OutputFormat);

    HRESULT Render(RenderManager& Manager, const LightingConstants& Constants);

    void ShutDown();
private:
    HRESULT Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Create_RootSignature(ID3D12Device* Device);

    HRESULT Create_PipelineState(const GraphicDevice& Device, DXGI_FORMAT OutputFormat);

    HRESULT Bind_Inputs(RenderManager& Manager, ID3D12GraphicsCommandList* CommandList);
    
private:
    std::unique_ptr<ShaderResource> m_VertexShader;
    std::unique_ptr<ShaderResource> m_PixelShader;

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
ENGINE_END