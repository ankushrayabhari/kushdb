#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>

#include "data/column_data.h"

using namespace kush;

int main() {
  std::random_device rd;
  std::mt19937_64 eng(rd());
  std::uniform_int_distribution<int64_t> distr;

  std::vector<int64_t> data(100);
  for (int i = 0; i < 100; i++) {
    data[i] = distr(eng);
  }

  kush::ColumnData<int64_t>::Serialize("out.skdbcol", data);
  return 0;
}
