#pragma once
#include <Windows.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <filesystem>

#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

ENGINE_START

class GameInstance;
class ShaderResource;

class ENGINE_DLL ShaderManager final : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit ShaderManager() = default;
    ~ShaderManager() override = default;
    
public:
    [[nodiscard("Warning : Check ShaderManager Initialize Result.")]]
    HRESULT Initialize();
    
    void    Update();

    [[nodiscard("Warning : Check Shader Compiled Completely.")]]
    HRESULT ShaderCompile(ShaderResource& Res);
    
#ifdef _DEBUG
public:
    void  Monitor_ShaderChanges(const ShaderResource& Res);
#endif
    
private:
    Microsoft::WRL::ComPtr<IDxcUtils>           m_pDXCUtils;            // HLSL Files Load, Extra Work
    Microsoft::WRL::ComPtr<IDxcCompiler3>       m_pDXCCompiler;         // HLSL Shader Compiler Variable
    Microsoft::WRL::ComPtr<IDxcIncludeHandler>  m_pDXCIncludeHandler;   // Handle Files in HLSL (ig. #include .hlsl)
    
#ifdef _DEBUG
private:
    struct WatchedShader {
        ShaderResource* Shader;
        std::filesystem::file_time_type LastWriteTime;
    };
    std::vector<WatchedShader> m_MonitoredShader;
    std::chrono::steady_clock::time_point m_lastShaderCheck{};
#endif
};

ENGINE_END