#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

class GameInstance;
class ENGINE_DLL GraphicDevice : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit GraphicDevice() = default;
	~GraphicDevice() = default;

};
ENGINE_END