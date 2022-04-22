#include "runtime/printer.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "runtime/date.h"
#include "runtime/string.h"

namespace kush::runtime::Printer {

void PrintBool(bool v) { std::cout << (v ? 't' : 'f') << "|"; }

void PrintInt8(int8_t v) { std::cout << v << "|"; }

void PrintInt16(int16_t v) { std::cout << v << "|"; }

void PrintInt32(int32_t v) { std::cout << v << "|"; }

void PrintInt64(int64_t v) { std::cout << v << "|"; }

void PrintDate(int32_t v) {
  int32_t y(0), m(0), d(0);
  runtime::Date::SplitDate(v, &y, &m, &d);
  std::cout << y << '-' << std::setw(2) << std::setfill('0') << m << '-'
            << std::setw(2) << std::setfill('0') << d << "|";
}

void PrintFloat64(double v) { std::cout << std::fixed << v << "|"; }

void PrintNewline() { std::cout << "\n"; }

void PrintString(kush::runtime::String::String* str) {
  std::cout << std::string_view(str->data, str->length) << "|";
}

void PrintBoolDebug(bool v) { std::cerr << (v ? 't' : 'f') << "|"; }

void PrintInt8Debug(int8_t v) { std::cerr << v << "|"; }

void PrintInt16Debug(int16_t v) { std::cerr << v << "|"; }

void PrintInt32Debug(int32_t v) { std::cerr << v << "|"; }

void PrintInt64Debug(int64_t v) { std::cerr << v << "|"; }

void PrintDateDebug(int32_t v) {
  int32_t y, m, d;
  runtime::Date::SplitDate(v, &y, &m, &d);
  std::cerr << y << '-' << std::setw(2) << std::setfill('0') << m << '-'
            << std::setw(2) << std::setfill('0') << d << "|";
}

void PrintFloat64Debug(double v) { std::cerr << std::fixed << v << "|"; }

void PrintNewlineDebug() { std::cerr << std::endl; }

void PrintStringDebug(kush::runtime::String::String* str) {
  std::cerr << std::string_view(str->data, str->length) << "|";
}

}  // namespace kush::runtime::Printer