#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <d3dcompiler.h>

#include <memory>
#include <utility>

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

#ifdef	ENGINE_EXPORTS
#define ENGINE_DLL		__declspec(dllexport)
#else
#define ENGINE_DLL		__declspec(dllimport)
#endif

#define			ENGINE_START	namespace ENGINE {
#define			ENGINE_END		}
#define			ENGINE_USE	    using namespace ENGINE;

#endif