#pragma once

#include <cstdint>
#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef int32_t (*ExecuteJoinFn)(int32_t initial_budget,
                                   bool resume_progress);
  virtual ExecuteJoinFn CompileJoinOrder(
      const std::vector<int>& order, void** materialized_buffers,
      void** materialized_indexes, void* tuple_idx_table, int32_t* progress_arr,
      int32_t* table_ctr, int32_t* idx_arr, int32_t* offset_arr,
      std::add_pointer<int32_t(int32_t, int8_t)>::type valid_tuple_handler) = 0;
};

}  // namespace kush::compile