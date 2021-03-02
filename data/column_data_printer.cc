#include <cstdint>
#include <iostream>

#include "data/column_data.h"
#include "data/vector.h"

using namespace kush::data;

bool comp(int8_t* v1, int8_t* v2) {
  double* l = reinterpret_cast<double*>(v1);
  double* r = reinterpret_cast<double*>(v2);
  if (*l < *r) {
    return true;
  }
  return false;
}

int main() {
  /*
  Float64ColumnData col;
  Open(&col, "tpch/data/l_quantity.kdb");

  auto size = Size(&col);
  for (uint32_t i = 0; i < size; i++) {
    double val = Get(&col, i);
    std::cout << val << "|\n";
  } */

  Vector vec;
  Create(&vec, sizeof(double), 5);
  *reinterpret_cast<double*>(PushBack(&vec)) = 1.5;
  *reinterpret_cast<double*>(PushBack(&vec)) = 3.2;
  *reinterpret_cast<double*>(PushBack(&vec)) = 6;
  *reinterpret_cast<double*>(PushBack(&vec)) = 2.7;
  *reinterpret_cast<double*>(PushBack(&vec)) = 5;

  for (int i = 0; i < Size(&vec); i++) {
    std::cout << *reinterpret_cast<double*>(Get(&vec, i)) << std::endl;
  }

  Sort(&vec, comp);

  for (int i = 0; i < Size(&vec); i++) {
    std::cout << *reinterpret_cast<double*>(Get(&vec, i)) << std::endl;
  }
}