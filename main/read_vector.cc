#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "data/column_data.h"

using namespace kush;

int main() {
  // random type for now
  const ColumnData<std::string_view> data("out.skdbcol");

  for (int i = 0; i < data.Size(); i++) {
    std::cout << data[i] << std::endl;
  }
}
