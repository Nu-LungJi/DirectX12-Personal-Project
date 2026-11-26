#pragma once
#include <array>
#include <span>

#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "GraphicDevice.h"

#include "ShaderResource.h"
#include "DebugView.h"
#include "LightPass.h"
#include "ToneMappingPass.h"

#include "RenderTargetResource.h"
#include "DepthTargetResource.h"

#include "UploadBuffer.h"

ENGINE_START
    enum class TargetSizePolicy : uint8_t { Relative, Fixed };

enum class RenderTargetID   : uint8_t { Albedo, Normal, Material, SceneColor, Bloom, Count };
enum class DepthTargetID    : uint8_t { SceneDepth, ShadowDepth, Count };

struct TargetSizeDesc final {
    TargetSizePolicy m_Policy = TargetSizePolicy::Relative;
    
    float    m_fRenderTargetScale = 1.f;
    
    uint32_t m_iWidth  = 0;
    uint32_t m_iHeight = 0;
};

struct RenderTargetRegistration final {
    TargetSizeDesc m_SizeDesc;
    RenderTargetDesc m_RenderTargetDesc{};
};
struct DepthTargetRegistration final {
    TargetSizeDesc m_SizeDesc;
    DepthTargetDesc m_RenderTargetDesc{};
};

class GameInstance;
class ShaderResource;
class MeshResource;

inline constexpr uint32_t MaxGeometryDrawCount = 256;

struct GeometryDrawItem {
    const MeshResource* Mesh = nullptr;
    TransformConstants Transform{};
};

class GeometryPass final {
public:
    [[nodiscard("Warning : Check RenderManager Initialize Result.")]]
    HRESULT Initialize(class RenderManager& Manager, const class ShaderManager& Shaders, const std::wstring& ShaderPath);
    
    [[nodiscard("Warning : Check RenderManager Render Result.")]]
    HRESULT Render(class RenderManager& RM, std::span<const GeometryDrawItem> Items);
    
    void    ShutDown();
    
private:
    HRESULT Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath);

    HRESULT Create_RootSignature(ID3D12Device* Device);
    HRESULT Create_PipelineState(RenderManager& Manager);
    
private:
    HRESULT Initialize_TransformBuffer(const GraphicDevice& Device);

    HRESULT Upload_Transform(uint32_t FrameIndex, const TransformConstants& Transform);

    void    Bind_Transform(ID3D12GraphicsCommandList* CommandList, uint32_t FrameIndex) const;
    
private:
    HRESULT Prepare_Draws(uint32_t FrameIndex, std::span<const GeometryDrawItem> Items);
    
    HRESULT Validate_Mesh(const MeshResource& Mesh) const;
    
    void Bind_Mesh(
        ID3D12GraphicsCommandList* CommandList,
        const MeshResource& Mesh) const;

    void Draw_Mesh(
        ID3D12GraphicsCommandList* CommandList,
        const MeshResource& Mesh) const;
    
private:
    inline static constexpr std::array ColorTargets{
        RenderTargetID::Albedo,
        RenderTargetID::Normal,
        RenderTargetID::Material
    };
    
    UploadBuffer m_TransformBuffer;
    
    std::unique_ptr<ShaderResource> m_VertexShader;
    std::unique_ptr<ShaderResource> m_PixelShader;

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
}; 

class ENGINE_DLL RenderManager final : public Interface::Uncopyable {
    friend class GameInstance; friend class GeometryPass; 
    friend class DebugView;
    friend class LightPass;
    friend class ToneMappingPass;
    
private:
    RenderManager() = default;
    ~RenderManager() override;
	
public:
    [[nodiscard("Warning : Check RenderManager Initialize Result.")]]
    HRESULT Initialize(GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& GeometryShaderPath);
	
    void    Update();
    
    HRESULT Render();
    
    void    ShutDown();
    
    HRESULT Resize() const;
 
private:
    HRESULT Begin_Pass(std::span<const RenderTargetID> ColorTargets, DepthTargetID DepthTarget = DepthTargetID::Count, bool Clear = true);
    
public:
    HRESULT Register_RenderTarget(const RenderTargetRegistration& Desc, RenderTargetID ID);
    HRESULT Register_DepthTarget(const DepthTargetRegistration& Desc, DepthTargetID ID);
    
    RenderTargetResource*   Get_RenderTarget(const RenderTargetID ID) const;
    DepthTargetResource*    Get_DepthTarget(const DepthTargetID ID)  const;

    HRESULT Submit_Mesh(const MeshResource& Mesh, const TransformConstants& Transform);

    void Clear_DrawList() noexcept { m_DrawCount = 0; }
    
public:
    void Set_LightingConstants(const LightingConstants& Constants) {
        m_LightingConstants = Constants;
    }

    void Set_DebugView(DebugViewMode Mode) {
        m_DebugViewPass.Set_Mode(Mode);
        m_DebugViewEnabled = true;
    }

    void Set_DebugViewEnabled(bool Enabled) {
        m_DebugViewEnabled = Enabled;
    }

    HRESULT Set_Exposure(float Exposure) {
        return m_ToneMappingPass.Set_Exposure(Exposure);
    }

    void Set_DebugDepthRange(float NearZ, float FarZ) {
        m_DebugViewPass.Set_DepthRange(NearZ, FarZ);
    }
    
private:
    HRESULT Initialize_DefaultTargets();
    
private:
    struct RenderTargetEntry {
        RenderTargetRegistration m_Desc{};
        std::unique_ptr<RenderTargetResource> m_pRenderTargetResource;
    };

    struct DepthTargetEntry {
        DepthTargetRegistration m_Desc{};
        std::unique_ptr<DepthTargetResource> m_pDepthTargetResource;
    };
    
    HRESULT Resolve_TargetSize(const TargetSizeDesc& Desc, uint32_t& Width, uint32_t& Height) const;

    HRESULT Create_RenderTarget(RenderTargetEntry& Entry) const;
    HRESULT Create_DepthTarget(DepthTargetEntry& Entry) const;
    
private:
    struct PassTargets {
        std::array<RenderTargetResource*, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> Colors{};

        DepthTargetResource* Depth = nullptr;

        uint32_t ColorCount = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };
    
    HRESULT Resolve_PassTargets(std::span<const RenderTargetID> ColorTargets, DepthTargetID DepthTarget,
        PassTargets& Targets) const;

    void    Bind_PassTargets(ID3D12GraphicsCommandList* CommandList, const PassTargets& Targets);

    void    Clear_PassTargets(const PassTargets& Targets);
    
private:
    GraphicDevice* m_pGraphicDevice = nullptr;
    
    std::array<RenderTargetEntry, static_cast<size_t>(RenderTargetID::Count)> m_RenderTargetList{};
    std::array<DepthTargetEntry, static_cast<size_t>(DepthTargetID::Count)>   m_DepthTargetList{};
    
private:
    GeometryPass m_GeometryPass;
    DebugView m_DebugViewPass;
    LightPass m_LightingPass;
    ToneMappingPass m_ToneMappingPass;
    bool m_DebugViewEnabled = false;
    LightingConstants m_LightingConstants{};
    
    std::array<GeometryDrawItem, MaxGeometryDrawCount> m_DrawItems{};
    uint32_t m_DrawCount = 0;
};
ENGINE_END