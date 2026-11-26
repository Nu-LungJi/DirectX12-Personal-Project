#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

// EngineDiagnostics Role : Collects DirectX 12 debug messages and diagnoses GPU device failures.
//                          Provides detailed diagnostics through InfoQueue and DRED. 

// DRED(Device Removed Extended Data) : provides additional diagnostic information to help investigate GPU failures 
//                                      or device removal in DirectX 12.
// GPU Failure can mean Device Error / Shader or GPU Command tried to access wrong memory area / TimeOut(Work Time Over)

class ENGINE_DLL EngineDiagnostics final : public Interface::EngineEditor, public Interface::Uncopyable {
public:
    // Initializes InfoQueue and configures debug message severity settings.
    [[nodiscard("Warning : Check EngineDiagnostics Initialize Result.")]]
    HRESULT Initialize(ID3D12Device* Device, bool BreakOnWarning = false);
    
    // Configures DRED before creating the D3D12 device.
    // Activate Setting of Breadcrumbs and Page Fault Reporting.
    [[nodiscard("Warning : Check EngineDiagnostics Configure_DRED Result.")]]
    static HRESULT Configure_DRED(bool AutoBreadcrumbs = true, bool PageFaults = true);
    
public:
    // Collects D3D12 debug messages and delivers them to the logger.
    void Dump_Messages() const;
    
    // Collects GPU device removal diagnostics using DRED.
    void Report_DeviceRemoval() const;
    
private:
    // Collects and formats GPU execution breadcrumbs.
    static void Append_Breadcrumbs(ID3D12DeviceRemovedExtendedData* DRED, std::wostringstream& Stream);
    
    // Collects and formats GPU Page Fault Information.
    static void Append_PageFault(ID3D12DeviceRemovedExtendedData* DRED, std::wostringstream& Stream); 
    
private:
    // When GPU Failure occurs, call GetDeviceRemovedReason() method and access DRED through Device
    ComPtr<ID3D12Device>    m_pDevice;          
    ComPtr<ID3D12InfoQueue> m_pInfoQueue;
};
ENGINE_END