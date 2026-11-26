#pragma once
#include <array>
#include <string_view>
#include <utility>

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi.h>

#include "LogTypeLookUp.h"

namespace ENGINE::DiagnosticsLookup
{
    template<typename T>
    struct LookupEntry {
        T Value;
        std::wstring_view Name;
    };
    
    struct SeverityEntry final {
        D3D12_MESSAGE_SEVERITY Value;
        std::wstring_view Name;
        LogLevel Level;
    };
    
    inline constexpr SeverityEntry UnknownSeverity{
        static_cast<D3D12_MESSAGE_SEVERITY>(-1),
        L"UNKNOWN",
        LogLevel::Warning
    };
    
    // D3D12 debug message severity.
    inline constexpr auto SeverityTable =
        std::to_array<SeverityEntry>({
            { D3D12_MESSAGE_SEVERITY_CORRUPTION, L"CORRUPTION", LogLevel::Critical },
            { D3D12_MESSAGE_SEVERITY_ERROR,      L"ERROR",      LogLevel::Error    },
            { D3D12_MESSAGE_SEVERITY_WARNING,    L"WARNING",    LogLevel::Warning  },
            { D3D12_MESSAGE_SEVERITY_INFO,       L"INFO",       LogLevel::Info     },
            { D3D12_MESSAGE_SEVERITY_MESSAGE,    L"MESSAGE",    LogLevel::Debug    }
        });
    
    // D3D12 debug message category.
    inline constexpr auto CategoryTable =
        std::to_array<LookupEntry<D3D12_MESSAGE_CATEGORY>>({
            { D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED,   L"APPLICATION_DEFINED" },
            { D3D12_MESSAGE_CATEGORY_MISCELLANEOUS,         L"MISCELLANEOUS" },
            { D3D12_MESSAGE_CATEGORY_INITIALIZATION,        L"INITIALIZATION" },
            { D3D12_MESSAGE_CATEGORY_CLEANUP,               L"CLEANUP" },
            { D3D12_MESSAGE_CATEGORY_COMPILATION,           L"COMPILATION" },
            { D3D12_MESSAGE_CATEGORY_STATE_CREATION,        L"STATE_CREATION" },
            { D3D12_MESSAGE_CATEGORY_STATE_SETTING,         L"STATE_SETTING" },
            { D3D12_MESSAGE_CATEGORY_STATE_GETTING,         L"STATE_GETTING" },
            { D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION, L"RESOURCE_MANIPULATION" },
            { D3D12_MESSAGE_CATEGORY_EXECUTION,             L"EXECUTION" },
            { D3D12_MESSAGE_CATEGORY_SHADER,                L"SHADER" }
        });
    
    // GPU breadcrumb operations.
    inline constexpr auto BreadcrumbTable =
        std::to_array<LookupEntry<D3D12_AUTO_BREADCRUMB_OP>>({
            { D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED,        L"DrawInstanced" },
            { D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED, L"DrawIndexedInstanced" },
            { D3D12_AUTO_BREADCRUMB_OP_DISPATCH,             L"Dispatch" },
            { D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE,         L"CopyResource" },
            { D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER,      L"ResourceBarrier" }
        });

    // Frequently used D3D12 message IDs.
    inline constexpr auto MessageIDTable =
        std::to_array<LookupEntry<D3D12_MESSAGE_ID>>({
            // Resource
            { D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH, L"RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH" },
            { D3D12_MESSAGE_ID_RESOURCE_BARRIER_INVALID_RESOURCE,      L"RESOURCE_BARRIER_INVALID_RESOURCE" },
            { D3D12_MESSAGE_ID_RESOURCE_BARRIER_INVALID_COMBINATION,   L"RESOURCE_BARRIER_INVALID_COMBINATION" },
            { D3D12_MESSAGE_ID_INVALID_SUBRESOURCE_STATE,              L"INVALID_SUBRESOURCE_STATE" },
    
            // Draw
            { D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_ROOT_SIGNATURE_NOT_SET,  L"COMMAND_LIST_DRAW_ROOT_SIGNATURE_NOT_SET" },
            { D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_ROOT_SIGNATURE_MISMATCH, L"COMMAND_LIST_DRAW_ROOT_SIGNATURE_MISMATCH" },
            { D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET,   L"COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET" },
            { D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_INDEX_BUFFER_NOT_SET,    L"COMMAND_LIST_DRAW_INDEX_BUFFER_NOT_SET" },
    
            // Command
            { D3D12_MESSAGE_ID_COMMAND_ALLOCATOR_CANNOT_RESET, L"COMMAND_ALLOCATOR_CANNOT_RESET" },
            { D3D12_MESSAGE_ID_COMMAND_LIST_CLOSED,            L"COMMAND_LIST_CLOSED" },
            { D3D12_MESSAGE_ID_COMMAND_LIST_OPEN,              L"COMMAND_LIST_OPEN" },
    
            // Descriptor
            { D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,   L"INVALID_DESCRIPTOR_HANDLE" },
            { D3D12_MESSAGE_ID_SET_DESCRIPTOR_HEAP_INVALID, L"SET_DESCRIPTOR_HEAP_INVALID" },
    
            // Pipeline State
            { D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_VERTEX_SHADER_NOT_SET,       L"CREATEGRAPHICSPIPELINESTATE_VERTEX_SHADER_NOT_SET" },
            { D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_INVALID_RENDER_TARGET_COUNT, L"CREATEGRAPHICSPIPELINESTATE_INVALID_RENDER_TARGET_COUNT" }
        });
    
    // DRED allocation object types.
    inline constexpr auto AllocationTypeTable =
        std::to_array<LookupEntry<D3D12_DRED_ALLOCATION_TYPE>>({
            { D3D12_DRED_ALLOCATION_TYPE_RESOURCE,          L"RESOURCE" },
            { D3D12_DRED_ALLOCATION_TYPE_HEAP,              L"HEAP" },
            { D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP,   L"DESCRIPTOR_HEAP" },
            { D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE,     L"COMMAND_QUEUE" },
            { D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR, L"COMMAND_ALLOCATOR" },
            { D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST,      L"COMMAND_LIST" },
            { D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE,    L"PIPELINE_STATE" },
            { D3D12_DRED_ALLOCATION_TYPE_FENCE,             L"FENCE" },
            { D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP,        L"QUERY_HEAP" },
            { D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT,      L"STATE_OBJECT" }
        });
    
    struct DeviceRemovalInfo {
        HRESULT Code;
        std::wstring_view Name;
        std::wstring_view Meaning;
    };
    
    inline constexpr auto DeviceRemovalTable =
        std::to_array<DeviceRemovalInfo>({
            {
                DXGI_ERROR_DEVICE_HUNG,
                L"DXGI_ERROR_DEVICE_HUNG",
                L"GPU command execution failed. Inspect command validity, resource usage, and synchronization."
            },
            {
                DXGI_ERROR_DEVICE_REMOVED,
                L"DXGI_ERROR_DEVICE_REMOVED",
                L"The D3D12 device was removed or became unavailable."
            },
            {
                DXGI_ERROR_DEVICE_RESET,
                L"DXGI_ERROR_DEVICE_RESET",
                L"The GPU device was reset after an execution failure."
            },
            {
                DXGI_ERROR_DRIVER_INTERNAL_ERROR,
                L"DXGI_ERROR_DRIVER_INTERNAL_ERROR",
                L"The graphics driver encountered an internal error."
            }
        });
    
    inline constexpr DeviceRemovalInfo UnknownDeviceRemoval{
        E_FAIL,
        L"UNKNOWN_DEVICE_REMOVAL_REASON",
        L"The device failed. Inspect DRED data for further details."
    };
    
    template<typename T, std::size_t N>
    [[nodiscard]]
    constexpr std::wstring_view Find_Name(const std::array<LookupEntry<T>, N>& Table, T Value, std::wstring_view Fallback) noexcept {
        for (const auto& Entry : Table) {
            if (Entry.Value == Value) return Entry.Name;
        }
    
        return Fallback;
    }
    
    [[nodiscard]]
    constexpr const SeverityEntry& Find_Severity(D3D12_MESSAGE_SEVERITY Severity) noexcept {
        for (const auto& Entry : SeverityTable) {
            if (Entry.Value == Severity) return Entry;
        }

        return UnknownSeverity;
    }
    
    [[nodiscard]]
    constexpr LogLevel Convert_LogLevel(D3D12_MESSAGE_SEVERITY Value) noexcept {
        return Find_Severity(Value).Level;
    }
    
    [[nodiscard]]
    constexpr std::wstring_view Get_SeverityName(D3D12_MESSAGE_SEVERITY Value) noexcept {
        return Find_Severity(Value).Name;
    }
    
    [[nodiscard]]
    constexpr std::wstring_view Get_CategoryName(D3D12_MESSAGE_CATEGORY Value) noexcept {
        return Find_Name(CategoryTable, Value, L"OTHER");
    }
    
    [[nodiscard]]
    constexpr std::wstring_view Get_BreadcrumbName(D3D12_AUTO_BREADCRUMB_OP Value) noexcept {
        return Find_Name(BreadcrumbTable, Value, L"Other");
    }
    
    [[nodiscard]]
    constexpr std::wstring_view Get_MessageIDName(D3D12_MESSAGE_ID Value) noexcept {
        return Find_Name(MessageIDTable, Value, L"UNKNOWN_MESSAGE_ID");
    }
    
    [[nodiscard]]
    constexpr std::wstring_view Get_AllocationTypeName(D3D12_DRED_ALLOCATION_TYPE Value) noexcept {
        return Find_Name(AllocationTypeTable, Value, L"OTHER_ALLOCATION_TYPE");
    }
    
    // Returns the name and explanation in a single lookup.
    [[nodiscard]]
    constexpr const DeviceRemovalInfo& Get_DeviceRemovalInfo(HRESULT Reason) noexcept {
        for (const auto& Entry : DeviceRemovalTable) {
            if (Entry.Code == Reason) return Entry;
        }
    
        return UnknownDeviceRemoval;
    }
    
    static_assert(Convert_LogLevel(D3D12_MESSAGE_SEVERITY_CORRUPTION)       == LogLevel::Critical);
    static_assert(Convert_LogLevel(D3D12_MESSAGE_SEVERITY_WARNING)          == LogLevel::Warning);
    static_assert(Convert_LogLevel(D3D12_MESSAGE_SEVERITY_MESSAGE)          == LogLevel::Debug);
    
    static_assert(Get_SeverityName(D3D12_MESSAGE_SEVERITY_ERROR)            == L"ERROR");
    static_assert(Get_CategoryName(D3D12_MESSAGE_CATEGORY_EXECUTION)        == L"EXECUTION");
    static_assert(Get_MessageIDName(D3D12_MESSAGE_ID_COMMAND_LIST_CLOSED)   == L"COMMAND_LIST_CLOSED");
    static_assert(Get_DeviceRemovalInfo(DXGI_ERROR_DEVICE_HUNG).Name        == L"DXGI_ERROR_DEVICE_HUNG");
}
