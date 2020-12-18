#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "data/column_data.h"

int main() {
  // random type for now
  const kush::data::ColumnData<int32_t> data("sample/int2.kdb");

  for (int i = 0; i < data.Size(); i++) {
    std::cout << data[i] << std::endl;
  }
}
