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
void PrintDate(int32_t v);
void PrintFloat64(double v);
void PrintString(kush::runtime::String::String* str);
void PrintNewline();

void PrintBoolDebug(bool v);
void PrintInt8Debug(int8_t v);
void PrintInt16Debug(int16_t v);
void PrintInt32Debug(int32_t v);
void PrintInt64Debug(int64_t v);
void PrintDateDebug(int32_t v);
void PrintFloat64Debug(double v);
void PrintStringDebug(kush::runtime::String::String* str);
void PrintNewlineDebug();

}  // namespace kush::runtime::Printer