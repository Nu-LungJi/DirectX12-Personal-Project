#pragma once
#include "Engine_Struct.h"

namespace Interface {

	class ENGINE_DLL Uncopyable {
	protected:
		constexpr Uncopyable() = default;
		constexpr virtual ~Uncopyable() = default;
		constexpr Uncopyable(const Uncopyable&) = delete;
		constexpr Uncopyable& operator=(const Uncopyable&) = delete;
	};

	template<typename T>
	class Singleton {
	protected:
		constexpr Singleton() = default;
		constexpr virtual ~Singleton() = default;

	public:
		constexpr static T& GetInstance() {
			static T instance;
			return instance;
		}
	};
	
	class ENGINE_DLL EngineSystem abstract {
	public:
		virtual ~EngineSystem() = default;
		
		static const ENGINE::TypeInfo& StaticType() noexcept {
			static constexpr ENGINE::TypeInfo type { "Interface::EngineSystem", nullptr };
			return type;
		}
		
		virtual const ENGINE::TypeInfo& GetType() const noexcept { return StaticType(); }

		template<typename T>
		bool IsA() const noexcept { return GetType().IsA(T::StaticType()); }	
	
	};
	
	class ENGINE_DLL EngineManager abstract : public EngineSystem {
	public:
		virtual ~EngineManager() = default;
	};
	
	class ENGINE_DLL EngineEditor abstract : public EngineSystem {
	public:
		virtual ~EngineEditor() = default;
	};
	
	class ENGINE_DLL EngineResource abstract : public EngineSystem {
	public:
		virtual ~EngineResource() = default;
	};
	
	class ENGINE_DLL EngineObject abstract : public EngineSystem {
	public:
		virtual ~EngineObject() = default;
	};
}
