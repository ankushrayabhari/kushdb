#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "catalog/catalog.h"
#include "data/column_data.h"
extern "C" void compute() {
  std::unordered_map<int32_t, std::vector<int32_t>> bufferb;
  kush::ColumnData<int32_t> table2_x3_data("test.skdbcol");
  for (uint32_t c = 0; c < table2_x3_data.size(); c++) {
    int32_t table2_x3 = table2_x3_data[c];
    bufferb[].push_back(table2_x3);
  }
  std::unordered_map<int32_t, std::vector<int32_t>> bufferd;
  kush::ColumnData<int32_t> table_x1_data("test.skdbcol");
  for (uint32_t e = 0; e < table_x1_data.size(); e++) {
    int32_t table_x1 = table_x1_data[e];
    if ((table_x1 < 100000000)) {
      bufferd[table_x1].push_back(table_x1);
    }
  }
  kush::ColumnData<int32_t> table1_x2_data("test.skdbcol");
  for (uint32_t f = 0; f < table1_x2_data.size(); f++) {
    int32_t table1_x2 = table1_x2_data[f];
    for (int h = 0; h < bufferd[table1_x2].size(); h += 1) {
      int32_t table_x1 = bufferd[table1_x2][h + 0];
      for (int j = 0; j < bufferb[].size(); j += 1) {
        int32_t table2_x3 = bufferb[][j + 0];
        std::cout << table2_x3;
        std::cout << " | " << table_x1;
        std::cout << " | " << table1_x2;
        std::cout << '\n';
      }
    }
  }
}
