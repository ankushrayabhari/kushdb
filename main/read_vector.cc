#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "data/column_data.h"

using namespace kush;

int main() {
  // random type for now
  ColumnData<void*> data("out.skdbcol");

  // We now consult the catalog have have determined its an int32 WOW
  const ColumnData<int64_t>& type_data =
      reinterpret_cast<ColumnData<int64_t>&>(data);

  for (int i = 0; i < type_data.size(); i++) {
    std::cout << type_data[i] << " ";
  }
  std::cout << std::endl;
}
