#pragma once
#include <array>

#include "Engine_Default.h"
#include "Engine_Interface.h"

#include <Windows.h>
#include <GameInput.h>
#include <wrl/client.h>

#if GAMEINPUT_API_VERSION != 3
#error This implementation requires GameInput API v3.
#endif

ENGINE_START

namespace GI = GameInput::v3;

enum class VirtualKey : unsigned int
{
    NONE = 0x00,

    MOUSE_L_BUTTON  = 0x01,
    MOUSE_R_BUTTON  = 0x02,
    KEY_CANCEL     = 0x03,
    MOUSE_M_BUTTON = 0x04,
    MOUSE_X1_BUTTON = 0x05,
    MOUSE_X2_BUTTON = 0x06,

    KEY_BACKSPACE = 0x08,
    KEY_TAB       = 0x09,
    KEY_CLEAR     = 0x0C,
    KEY_ENTER     = 0x0D,

    KEY_SHIFT     = 0x10,
    KEY_CTRL      = 0x11,
    KEY_ALT       = 0x12,
    KEY_PAUSE     = 0x13,
    KEY_CAPS_LOCK = 0x14,

    KEY_ESCAPE = 0x1B,

    KEY_SPACE     = 0x20,
    KEY_PAGE_UP   = 0x21,
    KEY_PAGE_DOWN = 0x22,
    KEY_END       = 0x23,
    KEY_HOME      = 0x24,
    KEY_LEFT      = 0x25,
    KEY_UP        = 0x26,
    KEY_RIGHT     = 0x27,
    KEY_DOWN      = 0x28,
    KEY_SELECT    = 0x29,
    KEY_PRINT     = 0x2A,
    KEY_EXE       = 0x2B,
    KEY_SNAPSHOT  = 0x2C,
    KEY_INSERT    = 0x2D,
    KEY_DELETE    = 0x2E,
    KEY_HELP      = 0x2F,

    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39,

    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4A,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_M = 0x4D,
    KEY_N = 0x4E,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5A,

    KEY_L_WIN = 0x5B,
    KEY_R_WIN = 0x5C,
    KEY_APPS  = 0x5D,

    KEY_NUMPAD_0        = 0x60,
    KEY_NUMPAD_1        = 0x61,
    KEY_NUMPAD_2        = 0x62,
    KEY_NUMPAD_3        = 0x63,
    KEY_NUMPAD_4        = 0x64,
    KEY_NUMPAD_5        = 0x65,
    KEY_NUMPAD_6        = 0x66,
    KEY_NUMPAD_7        = 0x67,
    KEY_NUMPAD_8        = 0x68,
    KEY_NUMPAD_9        = 0x69,
    KEY_NUMPAD_MULTIPLY = 0x6A,
    KEY_NUMPAD_ADD      = 0x6B,
    KEY_NUMPAD_SEPARA   = 0x6C,
    KEY_NUMPAD_SUBTRACT = 0x6D,
    KEY_NUMPAD_DECIMAL  = 0x6E,
    KEY_NUMPAD_DIVIDE   = 0x6F,

    KEY_F1  = 0x70,
    KEY_F2  = 0x71,
    KEY_F3  = 0x72,
    KEY_F4  = 0x73,
    KEY_F5  = 0x74,
    KEY_F6  = 0x75,
    KEY_F7  = 0x76,
    KEY_F8  = 0x77,
    KEY_F9  = 0x78,
    KEY_F10 = 0x79,
    KEY_F11 = 0x7A,
    KEY_F12 = 0x7B,
    KEY_F13 = 0x7C,
    KEY_F14 = 0x7D,
    KEY_F15 = 0x7E,
    KEY_F16 = 0x7F,
    KEY_F17 = 0x80,
    KEY_F18 = 0x81,
    KEY_F19 = 0x82,
    KEY_F20 = 0x83,
    KEY_F21 = 0x84,
    KEY_F22 = 0x85,
    KEY_F23 = 0x86,
    KEY_F24 = 0x87,

    KEY_NUM_LOCK    = 0x90,
    KEY_SCROLL_LOCK = 0x91,

    KEY_L_SHIFT = 0xA0,
    KEY_R_SHIFT = 0xA1,
    KEY_L_CTRL  = 0xA2,
    KEY_R_CTRL  = 0xA3,
    KEY_L_ALT   = 0xA4,
    KEY_R_ALT   = 0xA5,

    KEY_SEMICOLON   = 0xBA,
    KEY_EQUAL       = 0xBB,
    KEY_COMMA       = 0xBC,
    KEY_MINUS       = 0xBD,
    KEY_PERIOD      = 0xBE,
    KEY_SLASH       = 0xBF,
    KEY_GRAVEACCENT = 0xC0,

    KEY_LEFTBRACKET  = 0xDB,
    KEY_BACKSLASH    = 0xDC,
    KEY_RIGHTBRACKET = 0xDD,
    KEY_APOSTROPHE   = 0xDE,

    NUM = 256
};

struct KeyBoard final {
	using Bits = std::array<std::uint64_t, 4>;
	Bits Down{};
	Bits Pressed{};
	Bits Released{};
	
	[[nodiscard]]
	static constexpr bool Test(const Bits& bits, VirtualKey key) noexcept {
		const auto index = static_cast<unsigned int>(key);
		return index < 256 && (bits[index / 64] & (std::uint64_t{1} << (index % 64))) != 0;
	}
	
	[[nodiscard]]
	constexpr bool IsDown(VirtualKey key)		const noexcept { return Test(Down, key);	 }
	[[nodiscard]]
	constexpr bool IsPressed(VirtualKey key)	const noexcept { return Test(Pressed, key);  }
	[[nodiscard]]
	constexpr bool IsReleased(VirtualKey key)	const noexcept { return Test(Released, key); }
	
	constexpr void Apply(const Bits& next, bool recordEdges = true) noexcept {
		for (std::size_t i = 0; i < Down.size(); ++i) {
			if (recordEdges) {
				Pressed[i] |= next[i] & ~Down[i];
				Released[i] |= Down[i] & ~next[i];
			}

			Down[i] = next[i];
		}
	}
	
	constexpr void ClearEdges() noexcept {
		Pressed = {};
		Released = {};
	}
};

struct ButtonState final {
	std::uint32_t Down{};
	std::uint32_t Pressed{};
	std::uint32_t Released{};

	[[nodiscard]]
	constexpr bool IsDown(std::uint32_t mask) const noexcept	 { return (Down & mask) != 0;	  }
	[[nodiscard]]
	constexpr bool IsPressed(std::uint32_t mask) const noexcept  { return (Pressed & mask) != 0;  }
	[[nodiscard]]
	constexpr bool IsReleased(std::uint32_t mask) const noexcept { return (Released & mask) != 0; }
	
	constexpr void Apply(std::uint32_t next, bool recordEdges = true) noexcept {
		if (recordEdges) {
			Pressed |= next & ~Down;
			Released |= Down & ~next;
		}

		Down = next;
	}

	constexpr void ClearEdges() noexcept {
		Pressed = 0;
		Released = 0;
	}
};

struct InputSnapshot final {
	KeyBoard	keyboard;
	ButtonState mouse;
	ButtonState gamepadButtons;

	std::int64_t mouseDeltaX{};
	std::int64_t mouseDeltaY{};
	std::int64_t wheelDeltaX{};
	std::int64_t wheelDeltaY{};

	GI::GameInputGamepadState gamepad{};

	std::uint64_t frameNumber{};
	bool focused{};

	// Set when intermediate input events could not be recovered
	// due to a long stall or the processing limit.
	bool historyLost{};

	void ClearTransient() noexcept {
		keyboard.ClearEdges();
		mouse.ClearEdges();
		gamepadButtons.ClearEdges();

		mouseDeltaX = 0;
		mouseDeltaY = 0;
		wheelDeltaX = 0;
		wheelDeltaY = 0;
		historyLost = false;
	}

	void ReleaseAll() noexcept {
		keyboard.Apply({});
		mouse.Apply(0);
		gamepadButtons.Apply(0);

		gamepad = {};

		mouseDeltaX = 0;
		mouseDeltaY = 0;
		wheelDeltaX = 0;
		wheelDeltaY = 0;
	}
};

class GameInstance;

class ENGINE_DLL GameController : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit GameController() = default;
	~GameController() = default;
	
	// Call before starting update/read tasks or after all tasks have stopped.
	[[nodiscard]]
	HRESULT Initialize();

public:
	void Shutdown();
	void SetInputFocused(bool focused) noexcept;
	void BeginFrame();

public:
	[[nodiscard]]
	InputSnapshot GetSnapshot() const;

private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	struct Stream final {
		GI::GameInputKind kind{};
		APP_LOCAL_DEVICE_ID id{};
		bool aggregateCreated{};

		ComPtr<GI::IGameInputDevice> device;
		ComPtr<GI::IGameInputReading> cursor;
	};

	void Drain(Stream& stream, bool rebase);
	void Consume(const Stream& stream, GI::IGameInputReading& reading, bool recordEdges);

	void ClearStream(GI::GameInputKind kind);
	void Publish();

	static void ApplyStickDeadZone(float& x, float& y, float deadZone) noexcept;

private:
	ComPtr<GI::IGameInput> m_input;

	std::array<Stream, 3> m_streams{};
	InputSnapshot m_work{};

	GI::GameInputMouseState m_previousMouse{};
	bool m_havePreviousMouse{};

	std::atomic<std::uint64_t> m_focusToken{};
	std::uint64_t m_seenFocusToken{};

	mutable std::mutex m_snapshotMutex;
	InputSnapshot m_snapshot{};
};
ENGINE_END