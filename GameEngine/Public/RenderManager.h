#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

class GameInstance;

class ENGINE_DLL RenderManager : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit RenderManager() = default;
	~RenderManager() = default;

};
ENGINE_END