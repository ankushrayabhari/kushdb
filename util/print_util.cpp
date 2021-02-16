#include <cstdint>
#include <iostream>

namespace kush::util {

void Print(int8_t v) { std::cout << v << "|"; }

void Print(int16_t v) { std::cout << v << "|"; }

void Print(int32_t v) { std::cout << v << "|"; }

void Print(int64_t v) { std::cout << v << "|"; }

void Print(double v) { std::cout << v << "|"; }

void PrintNewline() { std::cout << "\n"; }

}  // namespace kush::util