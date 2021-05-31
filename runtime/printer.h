#pragma once

#include <cstdint>
#include <iostream>
#include <string_view>

#include "runtime/string.h"

namespace kush::runtime::Printer {

void PrintBool(bool v);
void PrintInt8(int8_t v);
void PrintInt16(int16_t v);
void PrintInt32(int32_t v);
void PrintInt64(int64_t v);
void PrintFloat64(double v);
void PrintString(kush::runtime::String::String* str);
void PrintNewline();

}  // namespace kush::runtime::Printer