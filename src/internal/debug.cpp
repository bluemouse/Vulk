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
#include <vector>

using std::cerr;
using std::endl;
using std::strlen;
using std::vector;
using std::vsnprintf;

namespace __helpers_debug__ {
void not_tested(const char* func, const char* file, int line) {
#ifdef _NDEBUG
  log("ERROR", "%s [file(ln): %s(%d)] is not tested", func, file, line);
#else
  log("WARNING", "%s [file(ln): %s(%d)] is not tested", func, file, line);
#endif
}

void not_implemented(const char* func, const char* file, int line) {
  cerr << "Error: " << func << " is not implemented!" << '\n'
       << "  File(Line): " << file << " (" << line << ")" << '\n';
#ifndef _NDEBUG
  abort();
#endif
}

void assertion_fail(const char* expr, const char* file, int line, const char* msg, ...) {
  cerr << "Error: assertion violation!" << '\n'
       << "  Condition: " << expr << '\n'
       << "  File(Line): " << file << " (" << line << ")" << '\n';

  if (msg != nullptr) {
    size_t msgLen = strlen(msg) + 160;
    vector<char> buffer(msgLen);
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer.data(), msgLen, msg, args);
    va_end(args);
    cerr << "Message:" << buffer.data() << '\n';
  }
  log_backtraces();
  abort();
}

void warning_fail(const char* expr, const char* file, int line, const char* msg, ...) {
  cerr << "Warning: check violation!" << '\n'
       << "  Condition: " << expr << '\n'
       << "  File(Line): " << file << " (" << line << ")" << '\n';

  if (msg != 0) {
    size_t msgLen = strlen(msg) + 160;
    vector<char> buffer(msgLen);
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer.data(), msgLen, msg, args);
    va_end(args);
    cerr << "Message:" << buffer.data() << '\n';
  }
}

void log(const char* tag, const char* msg, ...) {
  size_t msgLen = strlen(msg) + 160;
  vector<char> buffer(msgLen);
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer.data(), msgLen, msg, args);
  va_end(args);
  if (tag != nullptr) {
    cerr << tag << ": ";
  }
  cerr << buffer.data() << '\n';
}

void log(const char* file, int line, const char* tag, const char* msg, ...) {
  size_t msgLen = strlen(msg) + 160;
  vector<char> buffer(msgLen);
  va_list args;
  va_start(args, msg);
  vsnprintf(buffer.data(), msgLen, msg, args);
  va_end(args);

  cerr << tag << "[" << file << ":" << line << "]: " << buffer.data() << '\n';
}

void log_backtraces() {
  const int maxBacktraceDepth = 10;
  std::array<void*, maxBacktraceDepth> array{};
  auto numTraces = static_cast<size_t>(backtrace(array.data(), maxBacktraceDepth));
  char** traces  = backtrace_symbols(array.data(), numTraces);
  for (size_t i = 0; i < numTraces; ++i) {
    cerr << traces[i] << '\n';
  }
  free(traces);
}

const char* format(const char* fmt, ...) {
  static char buffer[1024];
  va_list args;
  va_start(args, fmt);
  vsprintf(buffer, fmt, args);
  va_end(args);
  return buffer;
}

} // namespace __helpers_debug__
