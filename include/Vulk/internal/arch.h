#pragma once

//
// OS
//
#if defined(__linux__)
#define ARCH_OS_LINUX
#elif defined(__APPLE__)
#define ARCH_OS_OSX
#elif defined(_WIN32) || defined(_WIN64)
#define ARCH_OS_WINDOWS
#elif defined(__ANDROID__)
#define ARCH_OS_ANDROID
#endif

//
// Processor
//
#if defined(i386) || defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#define ARCH_CPU_X86
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
#define ARCH_CPU_ARM
#endif

//
// Compiler
//
#if defined(__clang__)
#define ARCH_COMPILER_CLANG
#define ARCH_COMPILER_VERSION __clang_major__
#elif defined(__GNUC__)
#define ARCH_COMPILER_GCC
#define ARCH_COMPILER_VERSION __GNUC__
#elif defined(_MSC_VER)
#define ARCH_COMPILER_MSVC
#define ARCH_COMPILER_VERSION _MSC_VER
#endif

// Likely and unlikely branch prediction
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
template <typename T>
constexpr bool ARCH_LIKELY(T x) {
	return __builtin_expect(static_cast<bool>(x), true);
}
template <typename T>
constexpr bool ARCH_UNLIKELY(T x) {
	return __builtin_expect(static_cast<bool>(x), false);
}
#else
#define ARCH_LIKELY(x) (x)
#define ARCH_UNLIKELY(x) (x)
#endif
