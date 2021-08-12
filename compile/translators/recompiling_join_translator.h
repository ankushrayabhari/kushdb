#pragma once

#include <cstdint>
#include <vector>

namespace kush::compile {

class RecompilingJoinTranslator {
 public:
  typedef int32_t (*ExecuteJoinFn)(int32_t initial_budget, bool resume_progress,
                                   int32_t* progress_arr,
                                   int32_t* table_ctr_ptr,
                                   int32_t* num_result_tuples,
                                   int32_t* idx_arr);
  virtual ExecuteJoinFn CompileJoinOrder(const std::vector<int>& order,
                                         void** materialized_buffers,
                                         void** materialized_indexes,
                                         void* tuple_idx_table) = 0;
};

}  // namespace kush::compile