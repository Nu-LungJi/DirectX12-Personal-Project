#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

class ENGINE_DLL RenderManager : public Interface::Uncopyable, public Interface::Singleton<RenderManager> {
private:
	RenderManager() = default;
	~RenderManager() = default;

	friend class Interface::Singleton<RenderManager>;
};
