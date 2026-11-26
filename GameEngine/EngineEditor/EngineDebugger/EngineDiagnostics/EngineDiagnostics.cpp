#include "EngineDiagnostics.h"
#include "EngineDiagnosticsLookup.h"
#include "EngineUtility.h"
#include <iomanip>
#include <sstream>

#include "EngineLogger.h"
#include "LogTypeLookUp.h"
ENGINE_USE

namespace {
    std::wstring Get_DebugName(const wchar_t* WideName, const char* NarrowName) {
        if (WideName && *WideName) return WideName;
        if (NarrowName && *NarrowName) return EngineUtility::Convert_WideString(NarrowName);

        return L"(Unnamed)";
    }
    
    // Writes resource allocations associated with a GPU page fault.
    void Append_Allocations(std::wostringstream& Stream, std::wstring_view Label, const D3D12_DRED_ALLOCATION_NODE* Node) {
        Stream << L"  " << Label << L":\n";

        if (!Node) {
            Stream << L"    No matching allocations reported.\n";
            return;
        }

        constexpr UINT MaxEntries = 16;
        UINT Count = 0;

        while (Node && Count < MaxEntries) {
            std::wstring Name = Get_DebugName(Node->ObjectNameW, Node->ObjectNameA);

            Stream
                << L"    [" << Count + 1 << L"] Name : " << Name
                << L" (" << DiagnosticsLookup::Get_AllocationTypeName(Node->AllocationType) << L")\n";

            Node = Node->pNext;
            ++Count;
        }

        if (Node) Stream << L"    ... additional allocations omitted\n";
    }
}


HRESULT EngineDiagnostics::Initialize(ID3D12Device* Device, bool BreakOnWarning) {
    if (!Device) return E_POINTER;
    
    ComPtr<ID3D12Device> NewDevice = Device;
    ComPtr<ID3D12InfoQueue> NewInfoQueue;
    
    // InfoQueue is optional when the Debug Layer is unavailable.
    HRESULT Result = NewDevice.As(&NewInfoQueue);   // Request the InfoQueue interface from the D3D12 device.
    if (Result == E_NOINTERFACE) {
        m_pDevice = std::move(NewDevice);
        m_pInfoQueue.Reset();
        return S_FALSE;
    } // InfoQueue is unavailable, but the device can still be used.
    if (FAILED(Result)) return Result;              // Return if another error occurred.
    
    // Breakpoints are enabled only when a debugger is attached. 
    // (ig. run with .exe or Ctrl + F5(Run without Debug) in Visual Studio or other IDE return FALSE)
    const BOOL EnableBreak = IsDebuggerPresent() ? TRUE : FALSE;

    // D3D12 Debug Layer returns message with "Severity class"
    // if Severity class is CORRUPTION / ERROR, a break when a debugger is attached. 
    // WARNING triggers a break only when BreakOnWarning is enabled.
    // other message (lower severity class) will be record on EngineLogger through Dump_Messages(). 
    
    Result = NewInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, EnableBreak);
    if (FAILED(Result)) return Result;
    
    Result = NewInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, EnableBreak);
    if (FAILED(Result)) return Result;
    
    Result = NewInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, 
        BreakOnWarning && EnableBreak ? TRUE : FALSE);

    m_pDevice    = std::move(NewDevice);
    m_pInfoQueue = std::move(NewInfoQueue);
    
    return Result;
}

HRESULT EngineDiagnostics::Configure_DRED(bool AutoBreadcrumbs, bool PageFaults) {
    // Interface which can activate DRED Func and configure Breadcrumbs, PageFault.
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> Settings; 
    
    const HRESULT Result = D3D12GetDebugInterface(IID_PPV_ARGS(&Settings)); // Request the D3D12 Debug/Diagnostics interface
    if (FAILED(Result)) return Result;
    
    // Activate or Deactivate Auto Breadcrumbs Func, Page Fault Reporting Func with parameters
    
    // Auto Breadcrumbs Func : record GPU work process
    Settings->SetAutoBreadcrumbsEnablement(
        AutoBreadcrumbs ? D3D12_DRED_ENABLEMENT_FORCED_ON : D3D12_DRED_ENABLEMENT_FORCED_OFF);
    
    // Page Fault Reporting Func : provide diagnostics information relate with GPU memory access.
    Settings->SetPageFaultEnablement(
        PageFaults ? D3D12_DRED_ENABLEMENT_FORCED_ON : D3D12_DRED_ENABLEMENT_FORCED_OFF);
    
    // this method must be called before D3D12CreateDevice() to apply DRED Options
    
    return S_OK;
}

void EngineDiagnostics::Dump_Messages() const {
    if (!m_pInfoQueue) return;

    const UINT64 MessageCount = m_pInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    bool AllMessagesRead = true;
    
    std::vector<std::uint8_t> Buffer;
    
    // Read Debug messages one by one
    for (UINT64 Index = 0; Index < MessageCount; ++Index) {
        SIZE_T MessageSize = 0;

        // Every Messages Length are different, so have to recognize the Length 
        // and reserve the Buffer with message size before Get Message.
        HRESULT Result = m_pInfoQueue->GetMessage(Index, nullptr, &MessageSize);
        if (FAILED(Result) || MessageSize < sizeof(D3D12_MESSAGE)) {
            AllMessagesRead = false;
            break;
        }
        // Resize the existing buffer instead of creating a new one.
        Buffer.resize(MessageSize);
        
        auto* Message = reinterpret_cast<D3D12_MESSAGE*>(Buffer.data());

        // Retrieve the message.
        Result = m_pInfoQueue->GetMessage(Index, Message, &MessageSize);

        if (FAILED(Result)) {
            AllMessagesRead = false;
            break;
        }
        // Format the retrieved message.
        std::wstring Description = L"(No description)";

        if (Message->pDescription && Message->DescriptionByteLength > 0) {
            std::string_view Text{ Message->pDescription, Message->DescriptionByteLength };

            if (!Text.empty() && Text.back() == '\0')
                Text.remove_suffix(1);

            Description = EngineUtility::Convert_WideString(Text);
        }
        
        std::wostringstream Stream;

        Stream
            << L"\n[DX12][" << DiagnosticsLookup::Get_SeverityName(Message->Severity) << L"] "
            << DiagnosticsLookup::Get_MessageIDName(Message->ID) << L" (#" << static_cast<UINT>(Message->ID) << L")\n"
            << L"  Category : " << DiagnosticsLookup::Get_CategoryName(Message->Category) << L'\n'
            << L"  Message  : " << Description << L'\n';

        const LogLevel Level = DiagnosticsLookup::Convert_LogLevel(Message->Severity);
        
        // Logs messages according to their severity.
        if (!EngineLogger::Write(Level, LogChannels::Diagnostics, Stream.str())) {
            AllMessagesRead = false;
            break;
        }

        // ================== Log Example ==================
        // [DX12][ERROR] RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH (#527)
        // Category : RESOURCE_MANIPULATION
        // Message  : Resource barrier state mismatch...
    }
    
    // Clear only when the snapshot was read successfully.
    if (AllMessagesRead) m_pInfoQueue->ClearStoredMessages();
}

void EngineDiagnostics::Report_DeviceRemoval() const {
    if (!m_pDevice) return;
    
    const HRESULT Reason = m_pDevice->GetDeviceRemovedReason();
    // When device removal has not occurred(S_OK), pass this process.
    // DXGI_ERROR_DEVICE_REMOVED : Device Removed.
    // DXGI_ERROR_DEVICE_HUNG : GPU cannot complete task properly.
    // DXGI_ERROR_DEVICE_RESET : GPU Device is reset.
    if (SUCCEEDED(Reason)) return;
    
    const auto& RemovalInfo = DiagnosticsLookup::Get_DeviceRemovalInfo(Reason);
    
    std::wostringstream Stream;
    Stream
            << L"\n[DX12][DEVICE REMOVED]\n"
            << L"  Reason  : " << RemovalInfo.Name
            << L" (0x" << std::hex << std::uppercase << std::setw(8) 
            << std::setfill(L'0') << static_cast<std::uint32_t>(Reason) << L")\n"
            << std::dec
            << L"  Meaning : " << RemovalInfo.Meaning
            << L'\n';
    
    // ================== Log Example ==================
    // [DX12][DEVICE REMOVED]
    // Reason  : DXGI_ERROR_DEVICE_HUNG (0x887A0006)
    // Meaning : GPU command execution failed. Inspect command validity, resource usage, and synchronization.

    ComPtr<ID3D12DeviceRemovedExtendedData> DRED;
    
    if (FAILED(m_pDevice.As(&DRED))) {
        Stream << L"  DRED : Interface unavailable\n";
        (void)EngineLogger::Critical(LogChannels::Diagnostics, Stream.str());
        return;
    }
    
    Append_Breadcrumbs(DRED.Get(), Stream);
    Append_PageFault(DRED.Get(), Stream);

    (void)EngineLogger::Critical(LogChannels::Diagnostics, Stream.str());
}

void EngineDiagnostics::Append_Breadcrumbs(ID3D12DeviceRemovedExtendedData* DRED, std::wostringstream& Stream) {
    // Auto Breadcrumbs
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT Breadcrumbs{};
    // retrieves information about which commands the GPU has executed and the extent to which they have been completed.
    HRESULT Result = DRED->GetAutoBreadcrumbsOutput(&Breadcrumbs);

    Stream << L"\n[DX12][AUTO BREADCRUMBS]\n";

    if (FAILED(Result)) {
        Stream << L"  Status : Breadcrumb data unavailable.\n";
        return;
    }
    
    // DRED provide GPU task records with D3D12_AUTO_BREADCRUMB_NODE struct
    const D3D12_AUTO_BREADCRUMB_NODE* Node = Breadcrumbs.pHeadAutoBreadcrumbNode;
    if (nullptr == Node) {
        Stream << L"  Status : No outstanding command list data reported.\n";
        return;
    }
    
    constexpr UINT MaxNodes = 32;

    UINT NodeCount = 0;
    
    while (Node && NodeCount < MaxNodes) {
        const UINT Total = Node->BreadcrumbCount;
        Stream
            << L"  Command List     : " << Get_DebugName(Node->pCommandListDebugNameW, Node->pCommandListDebugNameA) << L'\n'
            << L"  Command Queue    : " << Get_DebugName(Node->pCommandQueueDebugNameW, Node->pCommandQueueDebugNameA) << L'\n'
            << L"  Recorded Ops     : " << Total << L'\n';

        if (Node->pLastBreadcrumbValue) {
            const UINT Completed = *Node->pLastBreadcrumbValue;

            Stream << L"  Completed Ops    : " << Completed << L'\n';

            if (Completed <= Total) {
                constexpr UINT HistoryCapacity = 65536;
                Stream << L"  Remaining Ops    : " << Total - Completed << L'\n';

                const UINT FirstRetained = Total > HistoryCapacity ? Total - HistoryCapacity : 0;

                if (Node->pCommandHistory) {
                    // Last successfully completed operation.
                    if (Completed == 0) Stream << L"  Last Completed Op: None confirmed\n";
                    else if (Completed - 1 >= FirstRetained) {
                        const UINT Index = (Completed - 1) % HistoryCapacity;
                        Stream << L"  Last Completed Op: " << DiagnosticsLookup::Get_BreadcrumbName(Node->pCommandHistory[Index]) 
                            << L" (#" << Completed - 1 << L")\n";
                    }
                    else Stream << L"  Last Completed Op: History overwritten\n";

                    // Next recorded operation, not a confirmed failure.
                    if (Completed == Total) Stream << L"  Next Recorded Op : None\n";
                    else if (Completed >= FirstRetained) {
                        const UINT Index = Completed % HistoryCapacity;

                        Stream
                            << L"  Next Recorded Op : " << DiagnosticsLookup::Get_BreadcrumbName(Node->pCommandHistory[Index])
                            << L" (#" << Completed << L")\n";
                    }
                    else Stream << L"  Next Recorded Op : History overwritten\n";
                }
                else Stream << L"  Operation History: Unavailable\n";
            }
            else Stream << L"  Status           : Invalid progress counter reported.\n";
        }
        else Stream << L"  Completed Ops    : Unavailable\n";

        Stream << L'\n';

        Node = Node->pNext;
        ++NodeCount;
    }

    if (Node) Stream << L"  ... additional command lists omitted\n";
    
    Stream
        << L"  Note: Remaining operations are not confirmed complete.\n"
        << L"        The next recorded operation is not necessarily\n"
        << L"        the operation that caused the GPU failure.\n";
    
    // ================== Log Example ==================
    /*
    [DX12][AUTO BREADCRUMBS]
    Command List     : Main Graphics Command List
    Command Queue    : Graphics Queue
    Recorded Ops     : 150
    Completed Ops    : 124
    Remaining Ops    : 26
    Last Completed Op: DrawInstanced (#123)
    Next Recorded Op : Dispatch (#124)
    
    Command List     : Shadow Pass Command List
    Command Queue    : Graphics Queue
    Recorded Ops     : 85
    Completed Ops    : 85
    Remaining Ops    : 0
    Last Completed Op: DrawIndexedInstanced (#84)
    Next Recorded Op : None
    
    Note: Remaining operations are not confirmed complete.
        The next recorded operation is not necessarily
        the operation that caused the GPU failure.
    */
}

void EngineDiagnostics::Append_PageFault(ID3D12DeviceRemovedExtendedData* DRED, std::wostringstream& Stream) {
    // GPU Page Fault
    D3D12_DRED_PAGE_FAULT_OUTPUT PageFault{};

    const HRESULT Result = DRED->GetPageFaultAllocationOutput(&PageFault);

    Stream << L"\n[DX12][PAGE FAULT]\n";

    if (FAILED(Result)) {
        Stream << L"  Status : Page fault data unavailable.\n";
        return;
    }
    
    if (PageFault.PageFaultVA == 0 && !PageFault.pHeadExistingAllocationNode && !PageFault.pHeadRecentFreedAllocationNode) {
        Stream << L"  Status : No page fault data reported.\n";
        return;
    }
    
    Stream
        << L"  GPU Virtual Address : 0x" << std::hex << std::uppercase << std::setw(16) << std::setfill(L'0')
        << PageFault.PageFaultVA << std::dec << L'\n'
        << L"  Meaning             : GPU memory access fault was reported.\n\n";

    Append_Allocations(Stream, L"Existing Allocations (Active)", PageFault.pHeadExistingAllocationNode);
    Stream << L'\n';
    Append_Allocations(Stream, L"Recently Freed Allocations (Released)", PageFault.pHeadRecentFreedAllocationNode);
    
    Stream
        << L"\n  Note: These allocations match the faulting address.\n"
        << L"        They are candidates for investigation,\n"
        << L"        not confirmed causes of the GPU failure.\n";
    
    // ================== Log Example ==================
    /* 
    [DX12][PAGE FAULT]
    GPU Virtual Address : 0x0000000012340000
    Meaning             : GPU memory access fault was reported.

    Existing Allocations (Active):
    [1] Name : MainRenderTarget (RESOURCE)
    [2] Name : SceneDescriptorHeap (DESCRIPTOR_HEAP)

    Recently Freed Allocations (Released):
    [1] Name : SceneTexture (RESOURCE)

    Note: These allocations match the faulting address.
        They are candidates for investigation,
        not confirmed causes of the GPU failure.
     */
}
