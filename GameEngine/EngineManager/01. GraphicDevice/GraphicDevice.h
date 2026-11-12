#pragma once
#include <vector>
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

struct GraphicDeviceDesc final {
	HWND		WindowHandle = nullptr;
	
	uint32_t 	WindowWidth = 0, WindowHeight = 0;
	
	uint8_t		BackBufferCount = 3;			
	// BackBufferCount : Number of Screen output images. Flip Model Need 2 BackBuffer at least.
	
	DXGI_FORMAT			BackBufferFormat	= DXGI_FORMAT_R8G8B8A8_UNORM;	// SDR Output
	D3D_FEATURE_LEVEL	MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_0;	// Minimum GPU Feature Level
	D3D_SHADER_MODEL	MinimumShaderModel	= D3D_SHADER_MODEL_6_0;		// Minimum Shader Model Level
	
	bool EnableVSync = true;			// Use Vertical Synchronization
	bool AllowTearing = true;			// Use Tearing Option
	bool EnableGpuValidation = false;	// Use GPU-based Verification - Tracking Bug
	
	uint32_t RTVDescriptorCount = 128;  // Include BackBuffer Slot
	uint32_t DSVDescriptorCount = 32;
	uint32_t ResourceDescriptorCount = 1024; // CBV / SRV / UAV
	
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
	
	bool SupportsWaveOps = false;
	bool SupportsNative16Bit = false;
};

struct GraphicDeviceView final {
	ID3D12Resource* BackBuffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE RTV{};
	D3D12_VIEWPORT	Viewport{};
	D3D12_RECT		ScissorRect{};
};

struct DescriptorAllocation
{
	uint32_t Index = UINT32_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE CPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE GPU{};
};


class ENGINE_DLL DescriptorAllocator final : public Interface::Uncopyable {
public:
	HRESULT Initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity);

	HRESULT Allocate(DescriptorAllocation& Allocation);
	
	HRESULT Free(DescriptorAllocation& Allocation);
	
	void	Shutdown();
	
public:
	ID3D12DescriptorHeap* Get_Heap() const noexcept { return m_pHeap.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> m_pHeap;

	std::vector<uint32_t> m_FreeIndexList;
	std::vector<uint8_t> m_AllocatedList;

	uint32_t m_iDescriptorSize = 0;
	bool m_bShaderVisible = false;
};

class GameInstance;
class ENGINE_DLL GraphicDevice final : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit GraphicDevice() = default;
	~GraphicDevice();

public:
	HRESULT Initialize(const GraphicDeviceDesc& Desc);
	
	HRESULT Begin_Frame();
	HRESULT End_Frame();

	HRESULT Wait_ForGPU();
	
public:
	HRESULT Resize(uint32_t Width, uint32_t Height);
	HRESULT Shutdown();
	
public:
	ID3D12Device* Get_Device() const noexcept { return m_pDevice.Get(); }
	
	ID3D12GraphicsCommandList*	Get_CommandList() const noexcept { return m_bFrameStarted ? m_pCommandList.Get() : nullptr; }

	GraphicDeviceView			Get_CurrentView() const noexcept;
	
	const GraphicDeviceSupport& Get_DeviceSupport() const noexcept { return m_Support; }
	
	DescriptorAllocator& Get_RTVAllocator() noexcept { return m_RTVAllocator; }

	DescriptorAllocator& Get_DSVAllocator() noexcept { return m_DSVAllocator; }

	DescriptorAllocator& Get_ResourceAllocator() noexcept { return m_ResourceAllocator; }

private:
	HRESULT Initialize_DescriptorHeaps();
	HRESULT Initialize_Factory(const GraphicDeviceDesc& Desc);
	HRESULT Initialize_Device(const GraphicDeviceDesc& Desc);
	
	HRESULT Query_DeviceResult(ID3D12Device* Device, GraphicDeviceSupport& Support);
	
	HRESULT Initialize_CommandQueue();
	HRESULT Initialize_Fence();
	HRESULT Initialize_FrameResources();
	HRESULT Initialize_SwapChain();
	HRESULT Initialize_BackBuffers();

	HRESULT Wait_ForFence(uint64_t Value);
	void	Transition_BackBuffer(D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);
	
	void	Reset_Initialization();
	
private:
	HRESULT Resize_BackBuffers(uint32_t Width, uint32_t Height);
	HRESULT Reset_CommandRecording();
	void	Release_BackBuffers();
	
private:
	struct FrameContext {
		ComPtr<ID3D12CommandAllocator> Allocator;
		uint64_t FenceValue = 0;
	};

	struct BackBufferContext {
		ComPtr<ID3D12Resource> Resource;
		DescriptorAllocation RTV;
	};
	
private:
	GraphicDeviceDesc		m_Desc{};
	GraphicDeviceSupport	m_Support{};

	ComPtr<IDXGIFactory6>	m_pFactory;
	ComPtr<IDXGIAdapter1>	m_pAdapter;
	ComPtr<ID3D12Device>	m_pDevice;
	
	ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
	ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
	ComPtr<ID3D12Fence>					m_pFence;
	ComPtr<IDXGISwapChain3>				m_pSwapChain;
	
	DescriptorAllocator     m_RTVAllocator;
	DescriptorAllocator     m_DSVAllocator;
	DescriptorAllocator     m_ResourceAllocator;	
	
	std::vector<FrameContext> m_FrameList;
	std::vector<BackBufferContext> m_BackBufferList;

	HANDLE			m_FenceEvent = nullptr;
	uint64_t		m_iNextFenceValue = 1;
	uint32_t		m_iFrameIndex = 0;

	D3D12_VIEWPORT	m_Viewport{};
	D3D12_RECT		m_ScissorRect{};

	bool m_bAllowTearing	= false;
	bool m_bInitialized		= false;
	bool m_bFrameStarted	= false;
	bool m_bMinimized		= false;
};
ENGINE_END