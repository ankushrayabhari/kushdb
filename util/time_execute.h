#pragma once

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

#include "absl/flags/flag.h"

#include "compile/query_translator.h"
#include "execution/executable_query.h"
#include "plan/operator/output_operator.h"
#include "util/profiler.h"
#include "util/test_util.h"

ABSL_FLAG(int, num_trials, 5, "Number of benchmark trials");

namespace kush::util {

void TimeExecute(kush::plan::Operator& query) {
  {
    auto executable_query = kush::compile::TranslateQuery(query);
    executable_query.Execute();
  }

#if PROFILE_ENABLED
  Profiler::profile([&]() {
    auto start = std::chrono::system_clock::now();
    auto executable_query = kush::compile::TranslateQuery(query);
    executable_query.Execute();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cerr << duration.count() << std::endl;
  });
#else
  for (int i = 0; i < FLAGS_num_trials.Get(); i++) {
    auto start = std::chrono::system_clock::now();
    auto executable_query = kush::compile::TranslateQuery(query);
    executable_query.Execute();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cerr << duration.count() << std::endl;
  }
#endif
}

}  // namespace kush::util