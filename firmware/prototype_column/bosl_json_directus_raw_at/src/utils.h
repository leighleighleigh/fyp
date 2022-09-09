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