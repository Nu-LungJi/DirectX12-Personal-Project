#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

class ENGINE_DLL TimeManager : public Interface::Uncopyable, public Interface::Singleton<TimeManager> {
private:
	TimeManager() = default;
	~TimeManager() = default;

	friend class Interface::Singleton<TimeManager>;
};
