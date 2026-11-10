#pragma once

namespace Interface {

	class Uncopyable {
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
	
	class EngineManager {
		
	};
}
