#include "ShaderManager.h"

#include <filesystem>

#include "ShaderResource.h"
ENGINE_USE

HRESULT ShaderManager::Initialize() {
    HRESULT Result = DxcCreateInstance(CLSID_DxcUtils,IID_PPV_ARGS(m_pDXCUtils.GetAddressOf()));
    if (FAILED(Result)) return Result;
    
    Result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_pDXCCompiler.GetAddressOf()));
    if (FAILED(Result)) return Result;
    
    return m_pDXCUtils->CreateDefaultIncludeHandler(m_pDXCIncludeHandler.GetAddressOf());
}

HRESULT ShaderManager::Compile_Shader(ShaderResource& Res) const {
    const ShaderDesc& Desc = Res.m_ShaderDesc;
    
    ComPtr<IDxcBlobEncoding> EncodingByteCode;
    UINT32 EncodingVersion = DXC_CP_UTF8;
    
    HRESULT Result = m_pDXCUtils->LoadFile(Desc.m_sFilePath.c_str(), &EncodingVersion, EncodingByteCode.GetAddressOf());
    if (FAILED(Result)) return Result;
    
    DxcBuffer Buffer = {EncodingByteCode->GetBufferPointer(), EncodingByteCode->GetBufferSize(), EncodingVersion };
    
    const auto parent = std::filesystem::path(Desc.m_sFilePath).parent_path();
    const std::wstring includePath = parent.empty() ? L"." : parent.wstring();
    
    ComPtr<IDxcResult> result;

    LPCWSTR arguments[]{
        Desc.m_sFilePath.c_str(),
        L"-E", Desc.m_sEntryPoint.c_str(),
        L"-T", Desc.m_sTargetProfile.c_str(),
        L"-I", includePath.c_str()
    };
    
    Result = m_pDXCCompiler->Compile(&Buffer, arguments, static_cast<UINT32>(_countof(arguments)),
        m_pDXCIncludeHandler.Get(), IID_PPV_ARGS(result.GetAddressOf()));
    if (FAILED(Result)) return Result;
    
    ComPtr<IDxcBlobUtf8> Errors;

    Result = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(Errors.GetAddressOf()), nullptr);
    if (FAILED(Result)) return Result;
    
    if (Errors && Errors->GetStringLength() > 0) OutputDebugStringA(Errors->GetStringPointer());

    HRESULT Status = E_FAIL;
    Result = result->GetStatus(&Status);
    if (FAILED(Result)) return Result;
    if (FAILED(Status)) return Status;
    
    ComPtr<IDxcBlob> ByteCode;

    Result = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(ByteCode.GetAddressOf()), nullptr);
    if (FAILED(Result)) return Result;

    Res.m_pByteCode = ByteCode;
    return S_OK;
}

void ShaderManager::Update() {
#ifdef _DEBUG
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastShaderCheck < m_CheckInterval) return; // if Lower than 3s, it is not monitored.

    m_lastShaderCheck = now;
    
    for (auto& watched : m_MonitoredShader) {
        std::error_code Error;
        const auto CurWriteTime = std::filesystem::last_write_time(watched.Shader->Get_ShaderDesc().m_sFilePath, Error);
        if (Error || CurWriteTime == watched.LastWriteTime) continue;

        watched.LastWriteTime = CurWriteTime;

        const HRESULT Result = Compile_Shader(*watched.Shader);
        OutputDebugStringA(FAILED(Result) ? "Log : Shader Reload Failed.\n" : "Log : Shader Recompiled.\n");
    }
#endif
}

#ifdef _DEBUG
void  ShaderManager::Monitor_ShaderChanges(ShaderResource* Res) {
    if (nullptr == Res) return;
    
    for (const auto& Shader : m_MonitoredShader) 
        if (Shader.Shader == Res) return ;
    
    std::error_code error;
    const auto CurrentChangeTime = std::filesystem::last_write_time(Res->Get_ShaderDesc().m_sFilePath, error);

    m_MonitoredShader.emplace_back(WatchedShader{Res, error ? std::filesystem::file_time_type::min() : CurrentChangeTime });
}
#endif