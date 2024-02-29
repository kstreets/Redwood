#pragma once

#ifdef RWD_BUILD_DLL
	#define	RWD_API __declspec(dllexport)
#else
	#define	RWD_API __declspec(dllimport)
#endif

#include <memory>

using u8 = char;
using i32 = int;
using u32 = unsigned int;
using f32 = float;
using f64 = double;

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename... Args>
constexpr Ref<T> MakeRef(Args&&... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename... Args>
constexpr Scope<T> MakeScope(Args&&... args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}
