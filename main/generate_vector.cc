#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "data/column_data.h"

using namespace kush;

int main() {
  std::srand(std::time(nullptr));
  ColumnData<int32_t> data;
  data.reset(100);

  for (int i = 0; i < 100; i++) {
    data.push_back(std::rand());
  }

  data.serialize("test1.skdbcol");
  return 0;
}
