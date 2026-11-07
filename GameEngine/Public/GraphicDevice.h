#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

struct GraphicDeviceDesc final {
	HWND		WindowHandle = nullptr;
	
	uint16_t	WindowWidth = 0, WindowHeight = 0;
	
	uint8_t		BackBufferCount = 3;			
	// BackBufferCount : Number of Screen output images. Flip Model Need 2 BackBuffer at least.
	uint8_t		CommandRecorderCount = 8;		
	// CommandRecorderCount : Number of Command Slot Per Frame(CammandList). It records Multiple Render Tasks
	
	DXGI_FORMAT			BackBufferFormat	= DXGI_FORMAT_R8G8B8A8_UNORM;	// SDR Output
	D3D_FEATURE_LEVEL	MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_0;	// Minimum GPU Feature Level
	D3D_SHADER_MODEL	MinimumShaderModel	= D3D_SHADER_MODEL_6_0;		// Minimum Shader Model Level
	
	bool EnableVSync = true;			// Use Vertical Synchronization
	bool AllowTearing = true;			// Use Tearing Option
	bool EnableGpuValidation = false;	// Use GPU-based Verification - Tracking Bug
	
#ifdef _DEBUG
	bool EnableDebugLayer = true;
#else
	bool EnableDebugLayer = false;
#endif
};

struct GraphicDeviceSupport final {
	D3D_FEATURE_LEVEL	MaximumFeatureLevel{};
	D3D_SHADER_MODEL	MaximumShaderModel{};
	
	
	D3D12_RESOURCE_BINDING_TIER 	 ResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_1;
	
	// Support Level : DXR
	D3D12_RAYTRACING_TIER			 RaytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	 
	// Support Level : Mesh Shader
	D3D12_MESH_SHADER_TIER			 MeshShaderTier = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;

	// Support Level : Variable Rate Shading
	D3D12_VARIABLE_SHADING_RATE_TIER VariableShadingRateTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
	
	// DXGI AllowTearing Flag
	bool SupportsTearing = false;
};

struct GraphicDeviceView final {
	ID3D12Resource* BackBuffer = nullptr;
	D3D12_VIEWPORT	Viewport{};
	D3D12_RECT		ScissorRect{};
};

class GameInstance;
class ENGINE_DLL GraphicDevice : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit GraphicDevice() = default;
	~GraphicDevice() = default;

};
ENGINE_END