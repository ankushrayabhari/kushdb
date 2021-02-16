#include <cstdint>
#include <iostream>

#include "data/column_data.h"

int main() {
  kush::data::Float64ColumnData col;
  kush::data::Open(&col, "tpch/data/l_quantity.kdb");

  auto size = kush::data::Size(&col);
  for (uint32_t i = 0; i < size; i++) {
    double val = kush::data::Get(&col, i);
    std::cout << val << "|\n";
  }
}