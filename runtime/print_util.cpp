#include <cstdint>
#include <iostream>
#include <string_view>

#include "runtime/string.h"

namespace kush::util {

void Print(bool v) { std::cout << v << "|"; }

void Print(int8_t v) { std::cout << v << "|"; }

void Print(int16_t v) { std::cout << v << "|"; }

void Print(int32_t v) { std::cout << v << "|"; }

void Print(int64_t v) { std::cout << v << "|"; }

void Print(double v) { std::cout << v << "|"; }

void PrintNewline() { std::cout << "\n"; }

void PrintString(kush::data::String* str) {
  std::cout << std::string_view(str->data, str->length) << "|";
}

}  // namespace kush::util