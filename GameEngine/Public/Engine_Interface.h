#pragma once
#include "Engine_Default.h"

namespace Interface {

	class ENGINE_DLL Uncopyable {
	protected:
		constexpr Uncopyable() = default;
		constexpr virtual ~Uncopyable() = default;
		constexpr Uncopyable(const Uncopyable&) = delete;
		constexpr Uncopyable& operator=(const Uncopyable&) = delete;
	};

	template<typename T>
	class ENGINE_DLL Singleton {
	protected:
		constexpr Singleton() = default;
		constexpr virtual ~Singleton() = default;

	public:
		constexpr static T& GetInstance() {
			static T instance;
			return instance;
		}
	};
}
