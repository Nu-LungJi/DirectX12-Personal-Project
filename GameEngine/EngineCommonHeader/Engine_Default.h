#pragma once
#pragma warning(disable : 4251)

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <dxgicommon.h>
#include <d3d12sdklayers.h>
#include <d3dx12_check_feature_support.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <memory>
#include <utility>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <Windows.h>

#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

#define DIRECTINPUT_VERSION	0x0800
#include <dinput.h>

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DBG_NEW 

#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 

#endif
#endif
//////////////////////////////////////////////////// CRT Memory Leakage Detection

#ifdef	ENGINE_EXPORTS
#define ENGINE_DLL		__declspec(dllexport)
#else
#define ENGINE_DLL		__declspec(dllimport)
#endif
//////////////////////////////////////////////////// DLL Import

#define			ENGINE_START	namespace ENGINE {
#define			ENGINE_END		}
#define			ENGINE_USE	    using namespace ENGINE;
//////////////////////////////////////////////////// namespace Wrapper

