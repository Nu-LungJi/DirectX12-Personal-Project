#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

ENGINE_START
class GameInstance;
class ENGINE_DLL EngineUtility : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit EngineUtility() = default;
    ~EngineUtility() override = default;

public:
    static HRESULT ErrorMassage(HRESULT hr, std::wstring_view location);
    
public:
    template<typename T>
    [[nodiscard("Warning : Store Casted Object.")]]
    static T* Cast(Interface::EngineResource* resource) noexcept {
        return !resource || !resource->IsA<T>() ? nullptr : static_cast<T*>(resource);
    }

    template<typename T>
    [[nodiscard("Warning : Store Casted Object.")]]
    static const T* Cast(const Interface::EngineResource* resource) noexcept {
        return !resource || !resource->IsA<T>() ? nullptr : static_cast<const T*>(resource);
    }
    
public:
    static constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};
ENGINE_END
