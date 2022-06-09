#include <iostream>
#ifndef NDEBUG
constexpr bool debug_ = true;
#else
constexpr bool debug_ = false;
#endif

void debug(const std::string &str) {
  if (debug_)
    std::cerr << str << "\n";
}