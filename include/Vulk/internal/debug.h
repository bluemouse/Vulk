#pragma once

#include <stdexcept>

#define _MI_STRINGIZE_IMPL(x) #x
#define _MI_STRINGIZE(x) _MI_STRINGIZE_IMPL(x)

#define MI_FORMAT(...) __helpers_debug__::format(__VA_ARGS__)

#define _MI_AT_THIS_LINE " [" __FILE__ ":" _MI_STRINGIZE(__LINE__) "]"

#define MI_VERIFY(condition)                                                   \
  if (!(condition)) {                                                          \
    throw std::runtime_error("Error: " #condition " failed" _MI_AT_THIS_LINE); \
  }
#define MI_VERIFY_MSG(condition, ...)                                                              \
  if (!(condition)) {                                                                              \
    auto msg = std::string("Error: ") + __helpers_debug__::format(__VA_ARGS__) + _MI_AT_THIS_LINE; \
    throw std::runtime_error(msg);                                                                 \
  }

#if defined(_NDEBUG) || defined(NDEBUG)
#define MI_NOT_TESTED() ((void)0)
#define MI_NOT_IMPLEMENTED() __helpers_debug__::not_implemented(__func__, __FILE__, __LINE__)
#define MI_LOG(...) ((void)0)
#define MI_LOG_ERROR(...) ((void)0)
#define MI_LOG_WARNING(...) ((void)0)
#define MI_LOG_INFO(...) ((void)0)
#define MI_LOG_DEBUG(...) ((void)0)
#else
#define MI_NOT_TESTED() __helpers_debug__::not_tested(__func__, __FILE__, __LINE__)
#define MI_NOT_IMPLEMENTED() __helpers_debug__::not_implemented(__func__, __FILE__, __LINE__)
#define MI_LOG(...) __helpers_debug__::log("", __VA_ARGS__)
#define MI_LOG_ERROR(...) __helpers_debug__::log("ERROR", __VA_ARGS__)
#define MI_LOG_WARNING(...) __helpers_debug__::log("WARNING", __VA_ARGS__)
#define MI_LOG_INFO(...) __helpers_debug__::log("INFO", __VA_ARGS__)
#define MI_LOG_DEBUG(...) __helpers_debug__::log(__FILE__, __LINE__, "DEBUG", __VA_ARGS__)
#define MI_LOG_FUNCTION() __helpers_debug__::log(__FILE__, __LINE__, "", __PRETTY_FUNCTION__)
#define MI_LOG_LINE() __helpers_debug__::log(__FILE__, __LINE__, "", "")
#endif

#if defined(_NO_ASSERTIONS) || defined(_NDEBUG) || defined(NDEBUG)
#define MI_ASSERT(EX) ((void)0)
#define MI_ASSERT_MSG(EX, ...) ((void)0)
#define MI_DEBUG_CODE(CODE)
#else
#define MI_ASSERT(EX) \
  ((EX) ? ((void)0) : __helpers_debug__::assertion_fail(#EX, __FILE__, __LINE__, nullptr))
#define MI_ASSERT_MSG(EX, ...) \
  ((EX) ? ((void)0) : __helpers_debug__::assertion_fail(#EX, __FILE__, __LINE__, __VA_ARGS__))
#define MI_DEBUG_CODE(CODE) CODE
#endif

#if defined(_NO_WARNINGS) || defined(_NDEBUG) || defined(NDEBUG)
#define MI_WARNING(EX) ((void)0)
#define MI_WARNING_MSG(EX, ...) ((void)0)
#else
#define MI_WARNING(EX) \
  ((EX) ? ((void)0) : __helpers_debug__::warning_fail(#EX, __FILE__, __LINE__, 0))
#define MI_WARNING_MSG(EX, ...) \
  ((EX) ? ((void)0) : __helpers_debug__::warning_fail(#EX, __FILE__, __LINE__, __VA_ARGS__))
#endif

// Functions
namespace __helpers_debug__ {
void not_tested(const char* func, const char* file, int line);
void not_implemented(const char* func, const char* file, int line);
void assertion_fail(const char* expr, const char* file, int line, const char* msg, ...);
void warning_fail(const char* expr, const char* file, int line, const char* msg, ...);

void log(const char* tag, const char* msg, ...);
void log(const char* file, int line, const char* tag, const char* msg, ...);

void log_backtraces();

const char* format(const char* fmt, ...);
} // namespace __helpers_debug__

#define MI_VERIFY_VKCMD(cmd)                                             \
  if (cmd != VK_SUCCESS) {                                               \
    throw std::runtime_error("Error: " #cmd " failed" _MI_AT_THIS_LINE); \
  }
#define MI_VERIFY_VKCMD_MSG(cmd, msg)               \
  if (cmd != VK_SUCCESS) {                          \
    throw std::runtime_error(msg _MI_AT_THIS_LINE); \
  }

#define MI_VERIFY_VKHANDLE(handle)                                           \
  if (handle == VK_NULL_HANDLE) {                                            \
    throw std::runtime_error("Error: " #handle " is null" _MI_AT_THIS_LINE); \
  }
#define MI_VERIFY_VKHANDLE_MSG(handle, msg)         \
  if (handle == VK_NULL_HANDLE) {                   \
    throw std::runtime_error(msg _MI_AT_THIS_LINE); \
  }
