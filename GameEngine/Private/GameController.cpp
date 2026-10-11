#include "GameController.h"
#include "GameInstance.h"

#pragma comment(lib, "gameinput.lib")

ENGINE_USE

HRESULT GameController::Initialize() {
    if (m_input) return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

    HRESULT hr = GI::GameInputCreate(m_input.ReleaseAndGetAddressOf());

    if (FAILED(hr)) return Utility::ErrorMassage(hr, L"GameController::Initialize -> GameInputCreate");

    m_input->SetFocusPolicy(GI::GameInputDefaultFocusPolicy);

    constexpr std::array kinds {
        GI::GameInputKindKeyboard,
        GI::GameInputKindMouse,
        GI::GameInputKindGamepad
    };
    
    constexpr std::array<std::wstring_view, 3> deviceNames
    {
        L"Keyboard",
        L"Mouse",
        L"Gamepad"
    };

    for (std::size_t i = 0; i < kinds.size(); ++i) {
        auto& stream = m_streams[i];
        stream.kind = kinds[i];

        hr = m_input->CreateAggregateDevice(stream.kind, &stream.id);

        if (FAILED(hr)) {
            const auto location = std::format(L"GameController::Initialize -> CreateAggregateDevice [{}]", deviceNames[i]);
            Shutdown();
            return Utility::ErrorMassage(hr, location);
        }

        stream.aggregateCreated = true;

        hr = m_input->FindDeviceFromId(&stream.id, stream.device.ReleaseAndGetAddressOf());

        if (FAILED(hr)) {
            const auto location = std::format(L"GameController::Initialize -> FindDeviceFromId  [{}]", deviceNames[i]);
            Shutdown();
            return Utility::ErrorMassage(hr, location);
        }
    }

    m_work = {};
    m_havePreviousMouse = false;

    const auto focusToken = m_focusToken.load(std::memory_order_acquire);
    m_seenFocusToken = focusToken;
    m_work.focused = (focusToken & 1) != 0;

    if (m_work.focused) {
        for (auto& stream : m_streams) {
            const HRESULT readingResult = m_input->GetCurrentReading(
                stream.kind,
                stream.device.Get(),
                stream.cursor.ReleaseAndGetAddressOf());

            if (SUCCEEDED(readingResult)) {
                Consume(stream, *stream.cursor.Get(), false);
            }
        }
    }
    
    Publish();
    return S_OK;
}

void GameController::Shutdown() {
    for (auto& stream : m_streams)
    {
        stream.cursor.Reset();
        stream.device.Reset();

        if (m_input && stream.aggregateCreated)
            m_input->DisableAggregateDevice(&stream.id);

        stream = {};
    }

    m_input.Reset();
    m_havePreviousMouse = false;
    m_work = {};

    Publish();
}

void GameController::SetInputFocused(bool focused) noexcept {
    auto previous = m_focusToken.load(std::memory_order_relaxed);

    for (;;) {
        if (((previous & 1) != 0) == focused) return;

        const auto next = ((previous & ~std::uint64_t{1}) + 2) | static_cast<std::uint64_t>(focused);

        if (m_focusToken.compare_exchange_weak(previous, next, std::memory_order_release, std::memory_order_relaxed)) return;
    }
}

void GameController::BeginFrame() {
    if (!m_input)return;

    m_work.ClearTransient();
    ++m_work.frameNumber;

    const auto focusToken = m_focusToken.load(std::memory_order_acquire);

    const bool changed = focusToken != m_seenFocusToken;
    m_work.focused = (focusToken & 1) != 0;

    if (changed)
    {
        m_work.ReleaseAll();

        for (auto& stream : m_streams)
            stream.cursor.Reset();

        m_havePreviousMouse = false;
        m_seenFocusToken = focusToken;
    }

    if (m_work.focused) {
        for (auto& stream : m_streams)
            Drain(stream, changed);
    }
    else {
        m_work.ReleaseAll();
    }

    const auto finalFocusToken = m_focusToken.load(std::memory_order_acquire);

    if (finalFocusToken != focusToken)
    {
        m_work.keyboard.Pressed = {};
        m_work.mouse.Pressed = 0;
        m_work.gamepadButtons.Pressed = 0;

        m_work.ReleaseAll();
        m_work.focused = false;
    }

    Publish();
}

void GameController::Drain(Stream& stream, bool rebase)
{
    ComPtr<GI::IGameInputReading> latest;

    const HRESULT hr = m_input->GetCurrentReading(stream.kind, stream.device.Get(), latest.GetAddressOf());

    if (FAILED(hr)) {
        ClearStream(stream.kind);
        stream.cursor.Reset();
        return;
    }

    if (rebase)
    {
        Consume(stream, *latest.Get(), false);
        stream.cursor = latest;
        return;
    }

    if (!stream.cursor)
    {
        Consume(stream, *latest.Get(), true);
        stream.cursor = latest;
        return;
    }
    
    constexpr std::uint32_t MaxReadingsPerFrame = 8192;
    std::uint32_t count = 0;

    while (stream.cursor.Get() != latest.Get())
    {
        ComPtr<GI::IGameInputReading> next;

        const HRESULT nextResult = m_input->GetNextReading(stream.cursor.Get(), stream.kind, stream.device.Get(), next.GetAddressOf());

        if (FAILED(nextResult) || count >= MaxReadingsPerFrame) {
            m_work.historyLost = true;
            Consume(stream, *latest.Get(), true);
            stream.cursor = latest;
            return;
        }

        Consume(stream, *next.Get(), true);
        stream.cursor = next;
        ++count;
    }
}

void GameController::Consume(const Stream& stream, GI::IGameInputReading& reading,bool recordEdges) {
    switch (stream.kind) {
    case GI::GameInputKindKeyboard: {
        std::array<GI::GameInputKeyState, 512> keys{};
        KeyBoard::Bits next{};

        if (reading.GetKeyCount() > keys.size()) {
            m_work.historyLost = true;
            m_work.keyboard.Apply({});
            return;
        }

        const auto count = reading.GetKeyState(static_cast<std::uint32_t>(keys.size()), keys.data());

        for (std::uint32_t i = 0; i < count; ++i) {
            const auto key = keys[i].virtualKey;

            if (key == 0 || key == 255) continue;

            next[key / 64] |= std::uint64_t{1} << (key % 64);
        }

        const auto mergeModifier = [&](UINT left, UINT right, UINT combined){
            const auto test = [&](UINT key){
                return (next[key / 64] & (std::uint64_t{1} << (key % 64))) != 0;
            };

            if (test(left) || test(right)) {
                next[combined / 64] |= std::uint64_t{1} << (combined % 64);
            }
        };

        mergeModifier(VK_LSHIFT, VK_RSHIFT, VK_SHIFT);
        mergeModifier(VK_LCONTROL, VK_RCONTROL, VK_CONTROL);
        mergeModifier(VK_LMENU, VK_RMENU, VK_MENU);

        m_work.keyboard.Apply(next, recordEdges);
        break;
    }

    case GI::GameInputKindMouse:
    {
        GI::GameInputMouseState state{};

        if (!reading.GetMouseState(&state)) return;

        m_work.mouse.Apply(static_cast<std::uint32_t>(state.buttons), recordEdges);

        if (recordEdges && m_havePreviousMouse) {
            constexpr auto relative = GI::GameInputMouseRelativePosition;

            if ((state.positions & relative) != 0 && (m_previousMouse.positions & relative) != 0) {
                m_work.mouseDeltaX += state.positionX - m_previousMouse.positionX;
                m_work.mouseDeltaY += state.positionY - m_previousMouse.positionY;
            }

            m_work.wheelDeltaX += state.wheelX - m_previousMouse.wheelX;
            m_work.wheelDeltaY += state.wheelY - m_previousMouse.wheelY;
        }

        m_previousMouse = state;
        m_havePreviousMouse = true;
        
        break;
    }

    case GI::GameInputKindGamepad: {
        GI::GameInputGamepadState state{};

        if (!reading.GetGamepadState(&state)) return;

        m_work.gamepadButtons.Apply(static_cast<std::uint32_t>(state.buttons), recordEdges);

        ApplyStickDeadZone(state.leftThumbstickX, state.leftThumbstickY, 0.15f);

        ApplyStickDeadZone(state.rightThumbstickX, state.rightThumbstickY, 0.15f);

        const auto filterTrigger = [](float value) {
            constexpr float deadZone = 0.05f;

            return std::clamp((value - deadZone) / (1.f - deadZone), 0.f, 1.f);
        };

        state.leftTrigger = filterTrigger(state.leftTrigger);
        state.rightTrigger = filterTrigger(state.rightTrigger);

        m_work.gamepad = state;
        break;
    }

    default:
        break;
    }
}

void GameController::ClearStream(GI::GameInputKind kind)
{
    switch (kind) {
    case GI::GameInputKindKeyboard:
        m_work.keyboard.Apply({});
        break;

    case GI::GameInputKindMouse:
        m_work.mouse.Apply(0);
        m_havePreviousMouse = false;
        break;

    case GI::GameInputKindGamepad:
        m_work.gamepadButtons.Apply(0);
        m_work.gamepad = {};
        break;

    default:
        break;
    }
}

void GameController::ApplyStickDeadZone(float& x, float& y, float deadZone) noexcept {
    const float lengthSquared = x * x + y * y;

    if (lengthSquared <= deadZone * deadZone) {
        x = 0.f; y = 0.f;
        return;
    }

    const float length = std::sqrt(lengthSquared);
    const float magnitude = std::clamp((length - deadZone) / (1.0f - deadZone), 0.0f, 1.0f);

    const float scale = magnitude / length;

    x *= scale; y *= scale;
}

void GameController::Publish() {
    std::lock_guard lock{m_snapshotMutex};
    m_snapshot = m_work;
}

InputSnapshot GameController::GetSnapshot() const {
    std::lock_guard lock{m_snapshotMutex};
    return m_snapshot;
}