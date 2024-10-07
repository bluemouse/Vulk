#include <Vulk/internal/debug.h>

#include <execinfo.h>
#include <unistd.h>

#include <array>
#include <cassert>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#include <spirv_reflect.h>

#include <fmt/core.h>
using fmt::format;

using std::cerr;
using std::endl;
using std::strlen;
using std::vector;
using std::vsnprintf;

#if defined(ARCH_OS_LINUX)
#define MI_COLORIZE_LOG 1

#define SET_RED(text) "\033[31m" text "\033[0m"
#define SET_GREEN(text) "\033[32m" text "\033[0m"
#define SET_YELLOW(text) "\033[33m" text "\033[0m"
#define SET_BLUE(text) "\033[34m" text "\033[0m"
#define SET_MAGENTA(text) "\033[35m" text "\033[0m"
#define SET_CYAN(text) "\033[36m" text "\033[0m"

#define SET_RED_BOLD(text) "\033[1;31m" text "\033[0m"
#define SET_GREEN_BOLD(text) "\033[1;32m" text "\033[0m"
#define SET_YELLOW_BOLD(text) "\033[1;33m" text "\033[0m"
#define SET_BLUE_BOLD(text) "\033[1;34m" text "\033[0m"
#define SET_MAGENTA_BOLD(text) "\033[1;35m" text "\033[0m"
#define SET_CYAN_BOLD(text) "\033[1;36m" text "\033[0m"

#endif // ARCH_OS_LINUX

#if defined(MI_COLORIZE_LOG)
constexpr const char* TAG_ERROR   = SET_RED_BOLD("Error");
constexpr const char* TAG_WARNING = SET_YELLOW_BOLD("Warning");
constexpr const char* TAG_INFO    = SET_CYAN_BOLD("Info");
#else
constexpr const char* TAG_ERROR   = "Error";
constexpr const char* TAG_WARNING = "Warning";
constexpr const char* TAG_INFO    = "Info";
#endif // MI_COLORIZE_LOG

// #define MI_THROW_ON_VERIFY_FAILURE 1

namespace {

static inline std::string vformat(const char* fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(nullptr, 0, fmt, args) + 1; // Extra space for '\0'
  std::vector<char> buffer(size);
  vsnprintf(buffer.data(), size, fmt, args_copy);
  va_end(args_copy);
  return std::string(buffer.data(), buffer.size() - 1); // Exclude the '\0'
}

static inline std::string where(const char* file, int line) {
#if defined(MI_COLORIZE_LOG)
  return format(SET_GREEN("{}:{}"), file, line);
#else
  return format("{}:{}", file, line);
#endif // MI_COLORIZE_LOG
}

static inline std::string backtraces() {
  std::ostringstream oss;

  const int maxBacktraceDepth = 8;
  std::array<void*, maxBacktraceDepth> array{};
  auto numTraces = static_cast<size_t>(backtrace(array.data(), maxBacktraceDepth));
  char** traces  = backtrace_symbols(array.data(), numTraces);
  for (size_t i = 0; i < numTraces; ++i) {
    oss << "        " << traces[i] << "\n";
  }
  free(traces);

  return oss.str();
}

static inline const char* string_SpvReflectResult(SpvReflectResult result) {
  // Convert SpvReflectResult into the string (similar to string_VkResult())
  switch (result) {
    case SPV_REFLECT_RESULT_NOT_READY: return "SPV_REFLECT_RESULT_NOT_READY";
    case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED: return "SPV_REFLECT_RESULT_ERROR_PARSE_FAILED";
    case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED: return "SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED";
    case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED: return "SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED";
    case SPV_REFLECT_RESULT_ERROR_NULL_POINTER: return "SPV_REFLECT_RESULT_ERROR_NULL_POINTER";
    case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR: return "SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR";
    case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH: return "SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH";
    case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND:
      return "SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE";
    case SPV_REFLECT_RESULT_ERROR_SPIRV_MAX_RECURSIVE_EXCEEDED:
      return "SPV_REFLECT_RESULT_ERROR_SPIRV_MAX_RECURSIVE_EXCEEDED";
    case SPV_REFLECT_RESULT_SUCCESS: return "SPV_REFLECT_RESULT_SUCCESS";
    default: return "Unhandled SpvReflectResult";
  };
}

} // namespace

namespace __helpers_debug__ {
void not_tested(const char* func, const char* file, int line) {
  log_warning(file, line, format("{} is not tested", func).c_str());
}

void not_implemented(const char* func, const char* file, int line) {
  [[maybe_unused]] auto msg = format("{} is not implemented", func);
  log_error(file, line, msg.c_str());
#ifndef _NDEBUG
  throw std::runtime_error(msg);
#endif
}

#define FORMAT_MSG(msg)                          \
  va_list args;                                  \
  va_start(args, msg);                           \
  std::string formattedMsg = vformat(msg, args); \
  va_end(args);

void assertion_fail(const char* expr, const char* file, int line) {
  assertion_fail(expr, file, line, nullptr);
}

void assertion_fail(const char* expr, const char* file, int line, const char* msg, ...) {
  std::string logMsg = format("Assertion violation! [{}]", where(file, line));
  logMsg += format("\n    Condition: {}", expr);
  if (msg != nullptr) {
    FORMAT_MSG(msg);
    logMsg += "\n    Message: " + formattedMsg;
  }
  logMsg += "\n    Backtrace:\n" + backtraces();
  log(TAG_ERROR, logMsg.c_str());
  throw std::runtime_error(format("Assertion violation: {}", expr));
}

void warning_fail(const char* expr, const char* file, int line) {
  warning_fail(expr, file, line, nullptr);
}

void warning_fail(const char* expr, const char* file, int line, const char* msg, ...) {
  std::string logMsg = format("Assertion violation! [{}]", where(file, line));
  logMsg += format("\n    Condition: {}", expr);
  if (msg != nullptr) {
    FORMAT_MSG(msg);
    logMsg += "\n    Message: " + formattedMsg;
  }
  log(TAG_WARNING, logMsg.c_str());
}

void log(const char* msg, ...) {
  FORMAT_MSG(msg);
  fmt::print(stderr, "{}\n", formattedMsg);
}

void log(const char* tag, const char* msg, ...) {
  FORMAT_MSG(msg);
  fmt::print(stderr, "{}\n", tag ? format("{}: {}", tag, formattedMsg) : formattedMsg);
}

void log(const char* file, int line, const char* tag, const char* msg, ...) {
  FORMAT_MSG(msg);
  fmt::print(stderr,
             "{} [{}]\n",
             tag ? format("{}: {}", tag, formattedMsg) : formattedMsg,
             where(file, line));
}

void log_error(const char* file, int line, const char* msg, ...) {
  FORMAT_MSG(msg);
  log(file, line, TAG_ERROR, formattedMsg.c_str());
}

void log_warning(const char* file, int line, const char* msg, ...) {
  FORMAT_MSG(msg);
  log(file, line, TAG_WARNING, formattedMsg.c_str());
}

void log_info(const char* file, int line, const char* msg, ...) {
  FORMAT_MSG(msg);
  log(file, line, TAG_INFO, formattedMsg.c_str());
}

void verification_fail(const char* file, int line, const char* cond) {
  auto msg = format("Condition '{}' failed!", cond);
  log_error(file, line, msg.c_str());
#if defined(MI_THROW_ON_VERIFY_FAILURE)
  throw std::runtime_error(msg);
#endif // MI_THROW_ON_VERIFY_FAILURE
}

void verification_fail(const char* file, int line, const char* cond, const char* msg, ...) {
  FORMAT_MSG(msg);
  auto logMsg = format("Condition '{}' failed! Message: {}", cond, formattedMsg);
  log_error(file, line, logMsg.c_str());
#if defined(MI_THROW_ON_VERIFY_FAILURE)
  throw std::runtime_error(logMsg);
#endif // MI_THROW_ON_VERIFY_FAILURE
}

void vulkan_call_fail(const char* file, int line, const char* vkCall, int vkResult) {
#if defined(MI_COLORIZE_LOG)
  auto result = format(SET_YELLOW("{}"), string_VkResult(static_cast<VkResult>(vkResult)));
#else
  const char* result = string_VkResult(static_cast<VkResult>(vkResult));
#endif // MI_COLORIZE_LOG
  auto msg = format("{}: {}", vkCall, result);
  log_error(file, line, msg.c_str());
#if defined(MI_THROW_ON_VERIFY_FAILURE)
  throw std::runtime_error(msg);
#endif // MI_THROW_ON_VERIFY_FAILURE
}

void spvreflect_call_fail(const char* file, int line, const char* reflectCall, int reflectResult) {
#if defined(MI_COLORIZE_LOG)
  auto result = format(SET_YELLOW("{}"),
                       string_SpvReflectResult(static_cast<SpvReflectResult>(reflectResult)));
#else
  const char* result = string_SpvReflectResult(static_cast<SpvReflectResult>(reflectResult));
#endif // MI_COLORIZE_LOG
  auto msg = format("{}: {}", reflectCall, result);
  log_error(file, line, msg.c_str());
#if defined(MI_THROW_ON_VERIFY_FAILURE)
  throw std::runtime_error(msg);
#endif // MI_THROW_ON_VERIFY_FAILURE
}

} // namespace __helpers_debug__
