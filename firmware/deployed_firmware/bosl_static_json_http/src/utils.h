#include <Arduino.h>

#ifdef DEBUG
namespace {
template <typename T>
static void DBG_PLAIN(T last) {
  DEBUG.println(last);
}

template <typename T, typename... Args>
static void DBG_PLAIN(T head, Args... tail) {
  DEBUG.print(head);
  DEBUG.print(' ');
  DBG_PLAIN(tail...);
}

template <typename... Args>
static void DBG(Args... args) {
  DEBUG.print("[");
  DEBUG.print(millis());
  DEBUG.print("] ");
  DBG_PLAIN(args...);
}
}  // namespace
#else
#define DBG_PLAIN(...)
#define DBG(...)
#endif

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}