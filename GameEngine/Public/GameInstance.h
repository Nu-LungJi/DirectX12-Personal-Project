#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

class ENGINE_DLL GameInstance : public Interface::Uncopyable, public Interface::Singleton<GameInstance> {
private:
	GameInstance()  = default;
	~GameInstance() = default;

	friend class Interface::Singleton<GameInstance>;
};

