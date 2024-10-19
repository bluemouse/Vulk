#pragma once

#include <Vulk/internal/arch.h>

#include <stdexcept>

// Helper macro to convert an integer to a string
#define CONCATENATE(prefix, line) prefix##line
#define MAKE_UNIQUE(prefix, line) CONCATENATE(prefix, line)

#if defined(_NDEBUG) || defined(NDEBUG)

#define MI_NOT_TESTED() ((void)0)
#define MI_NOT_IMPLEMENTED() __helpers_debug__::not_implemented(__func__, __FILE__, __LINE__)
#define MI_TRACE() __helpers_debug__::log(__FILE__, __LINE__, "Trace", __PRETTY_FUNCTION__)

#define MI_LOG(...) ((void)0)
#define MI_LOG_ERROR(...) ((void)0)
#define MI_LOG_WARNING(...) ((void)0)
#define MI_LOG_INFO(...) ((void)0)

#else

#define MI_NOT_TESTED() __helpers_debug__::not_tested(__func__, __FILE__, __LINE__)
#define MI_NOT_IMPLEMENTED() __helpers_debug__::not_implemented(__func__, __FILE__, __LINE__)
#define MI_TRACE() __helpers_debug__::log(__FILE__, __LINE__, "Trace", __PRETTY_FUNCTION__)

#define MI_LOG(...) __helpers_debug__::log(__VA_ARGS__)
#define MI_LOG_ERROR(...) __helpers_debug__::log_error(__FILE__, __LINE__, __VA_ARGS__)
#define MI_LOG_WARNING(...) __helpers_debug__::log_warning(__FILE__, __LINE__, __VA_ARGS__)
#define MI_LOG_INFO(...) __helpers_debug__::log_info(__FILE__, __LINE__, __VA_ARGS__)

#endif // defined(_NDEBUG) || defined(NDEBUG)

#if defined(_NO_ASSERTIONS) || defined(_NDEBUG) || defined(NDEBUG)

#define MI_ASSERT(COND) ((void)0)
#define MI_ASSERT_MSG(COND, ...) ((void)0)
#define MI_DEBUG_CODE(CODE)

#else

#define MI_ASSERT(COND) \
  (ARCH_LIKELY(COND) ? ((void)0) : __helpers_debug__::assertion_fail(#COND, __FILE__, __LINE__))
#define MI_ASSERT_MSG(COND, ...) \
  (ARCH_LIKELY(COND) ? ((void)0) \
                     : __helpers_debug__::assertion_fail(#COND, __FILE__, __LINE__, __VA_ARGS__))
#define MI_DEBUG_CODE(CODE) CODE

#endif // defined(_NO_ASSERTIONS) || defined(_NDEBUG) || defined(NDEBUG)

#if defined(_NO_WARNINGS) || defined(_NDEBUG) || defined(NDEBUG)

#define MI_WARNING(COND) ((void)0)
#define MI_WARNING_MSG(COND, ...) ((void)0)

#else

#define MI_WARNING(COND) \
  (ARCH_LIKELY(COND) ? ((void)0) : __helpers_debug__::warning_fail(#COND, __FILE__, __LINE__))
#define MI_WARNING_MSG(COND, ...) \
  (ARCH_LIKELY(COND) ? ((void)0)  \
                     : __helpers_debug__::warning_fail(#COND, __FILE__, __LINE__, __VA_ARGS__))

#endif // defined(_NO_WARNINGS) || defined(_NDEBUG) || defined(NDEBUG)

//
// Verification macros
//
#define MI_VERIFY(condition)                                              \
  if (ARCH_UNLIKELY(!(condition))) {                                      \
    __helpers_debug__::verification_fail(__FILE__, __LINE__, #condition); \
  }
#define MI_VERIFY_MSG(condition, ...)                                                  \
  if (ARCH_UNLIKELY(!(condition))) {                                                   \
    __helpers_debug__::verification_fail(__FILE__, __LINE__, #condition, __VA_ARGS__); \
  }

//
// Verification macros (Vulkan)
//
#define MI_VERIFY_VK_RESULT(call)                                             \
  do {                                                                        \
    const auto result = call;                                                 \
    if (ARCH_UNLIKELY(result != VK_SUCCESS)) {                                \
      __helpers_debug__::vulkan_call_fail(__FILE__, __LINE__, #call, result); \
    }                                                                         \
  } while (0)

#define MI_VERIFY_VK_HANDLE(handle)                                                            \
  do {                                                                                         \
    if (ARCH_UNLIKELY(handle == VK_NULL_HANDLE)) {                                             \
      __helpers_debug__::verification_fail(__FILE__, __LINE__, #handle " != VK_NULL_HANDLE "); \
    }                                                                                          \
  } while (0)

#define MI_VERIFY_SPVREFLECT_RESULT(call)                                         \
  do {                                                                            \
    const auto result = call;                                                     \
    if (ARCH_UNLIKELY(result != SPV_REFLECT_RESULT_SUCCESS)) {                    \
      __helpers_debug__::spvreflect_call_fail(__FILE__, __LINE__, #call, result); \
    }                                                                             \
  } while (0)

//
// Label and Name macros (Vulkan)
//
#define MI_BEGIN_LABEL(queue, label, color) queue.beginLabel(label, color)
#define MI_END_LABEL(queue, label, color) queue.endLabel(label, color)
#define MI_INSERT_LABEL(queue, label, color) queue.insertLabel(label, color)
#define MI_SCOPED_LABEL(queue, label, color) auto __##__LINE__ = queue.scopedLabel(label, color)
#define MI_SET_NAME(device, type, object, name) device.setDebugObjectName(type, object, name)

//
// Helper functions
//
namespace __helpers_debug__ {

void not_tested(const char* func, const char* file, int line);
void not_implemented(const char* func, const char* file, int line);

void assertion_fail(const char* expr, const char* file, int line);
void assertion_fail(const char* expr, const char* file, int line, const char* msg, ...);

void warning_fail(const char* expr, const char* file, int line);
void warning_fail(const char* expr, const char* file, int line, const char* msg, ...);

void log(const char* msg, ...);
void log(const char* tag, const char* msg, ...);
void log(const char* file, int line, const char* tag, const char* msg, ...);

void log_error(const char* file, int line, const char* msg, ...);
void log_warning(const char* file, int line, const char* msg, ...);
void log_info(const char* file, int line, const char* msg, ...);

void verification_fail(const char* file, int line, const char* cond);
void verification_fail(const char* file, int line, const char* cond, const char* msg, ...);

void vulkan_call_fail(const char* file, int line, const char* vkCall, int vkResult);
void spvreflect_call_fail(const char* file, int line, const char* reflectCall, int reflectResult);

} // namespace __helpers_debug__
