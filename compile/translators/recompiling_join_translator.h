#pragma once

#include <cstdint>
#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef int32_t (*ExecuteJoinFn)(int32_t initial_budget,
                                   bool resume_progress);
  virtual ExecuteJoinFn CompileJoinOrder(
      const std::vector<int>& order, void** materialized_buffers_raw,
      void** materialized_indexes_raw, void* tuple_idx_table_ptr_raw,
      int32_t* progress_arr_raw, int32_t* table_ctr_raw, int32_t* idx_arr_raw,
      int32_t* offset_arr_raw,
      std::add_pointer<int32_t(int32_t, int8_t)>::type
          valid_tuple_handler_raw) = 0;
};

}  // namespace kush::compile