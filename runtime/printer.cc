#include "runtime/printer.h"

#include <cstdint>
#include <iostream>
#include <string_view>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "runtime/string.h"

namespace kush::runtime::Printer {

void PrintBool(bool v) { std::cout << (v ? 't' : 'f') << "|"; }

void PrintInt8(int8_t v) { std::cout << v << "|"; }

void PrintInt16(int16_t v) { std::cout << v << "|"; }

void PrintInt32(int32_t v) { std::cout << v << "|"; }

void PrintInt64(int64_t v) { std::cout << v << "|"; }

void PrintDate(int64_t v) {
  auto time = absl::FromUnixMillis(v);
  auto utc = absl::UTCTimeZone();
  auto day = absl::ToCivilDay(time, utc);
  std::cout << absl::FormatCivilTime(day) << "|";
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

void PrintDateDebug(int64_t v) {
  auto time = absl::FromUnixMillis(v);
  auto utc = absl::UTCTimeZone();
  auto day = absl::ToCivilDay(time, utc);
  std::cerr << absl::FormatCivilTime(day) << "|";
}

void PrintFloat64Debug(double v) { std::cerr << std::fixed << v << "|"; }

void PrintNewlineDebug() { std::cerr << std::endl; }

void PrintStringDebug(kush::runtime::String::String* str) {
  std::cerr << std::string_view(str->data, str->length) << "|";
}

}  // namespace kush::runtime::Printer