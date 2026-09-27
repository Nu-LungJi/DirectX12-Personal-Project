#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

class ENGINE_DLL GraphicDevice : public Interface::Uncopyable, public Interface::Singleton<GraphicDevice> {
private:
	GraphicDevice() = default;
	~GraphicDevice() = default;

	friend class Interface::Singleton<GraphicDevice>;
};
