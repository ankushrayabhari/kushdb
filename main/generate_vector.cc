#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>

#include "data/column_data.h"

using namespace kush;

int main() {
  const std::string charset =
      "0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::random_device rd;
  std::mt19937_64 eng(rd());
  std::uniform_int_distribution<int> char_distr(0, charset.size() - 1);
  std::uniform_int_distribution<int> length_distr(0, 100);

  auto randchar = [&]() -> char { return charset[char_distr(eng)]; };

  std::vector<std::string> data(100);
  for (int i = 0; i < 100; i++) {
    int len = length_distr(eng);
    data[i] = std::string(len, 0);
    std::generate_n(data[i].begin(), len, randchar);
  }

  kush::ColumnData<std::string_view>::Serialize("out.skdbcol", data);
  return 0;
}
