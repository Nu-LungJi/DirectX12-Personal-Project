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
	
	class ENGINE_DLL EngineManager {
		
	};
	
	class ENGINE_DLL EngineResource {
	public:
		virtual ~EngineResource() = default;
		
		static const ENGINE::TypeInfo& StaticType() noexcept {
			static constexpr ENGINE::TypeInfo type { "Interface::EngineResource", nullptr };
			return type;
		}
		
		virtual const ENGINE::TypeInfo& GetType() const noexcept { return StaticType(); }

		template<typename T>
		bool IsA() const noexcept { return GetType().IsA(T::StaticType()); }
	};
}
