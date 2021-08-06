#include "runtime/printer.h"

#include <cstdint>
#include <iostream>
#include <string_view>

#include "runtime/string.h"

namespace kush::runtime::Printer {

void PrintBool(bool v) { std::cout << v << "|"; }

void PrintInt8(int8_t v) { std::cout << v << "|"; }

void PrintInt16(int16_t v) { std::cout << v << "|"; }

void PrintInt32(int32_t v) { std::cout << v << "|"; }

void PrintInt64(int64_t v) { std::cout << v << "|"; }

void PrintFloat64(double v) { std::cout << std::fixed << v << "|"; }

void PrintNewline() { std::cout << "\n"; }

void PrintString(kush::runtime::String::String* str) {
  std::cout << std::string_view(str->data, str->length) << "|";
}

}  // namespace kush::runtime::Printer