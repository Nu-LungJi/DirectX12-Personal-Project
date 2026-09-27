#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

class ENGINE_DLL GameController : public Interface::Uncopyable, public Interface::Singleton<GameController> {
private:
	GameController() = default;
	~GameController() = default;

	friend class Interface::Singleton<GameController>;
};
