#pragma once

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

#include "compile/query_translator.h"
#include "execution/executable_query.h"
#include "plan/operator/output_operator.h"
#include "util/test_util.h"

namespace kush::util {

void TimeExecute(kush::plan::OutputOperator& query) {
  {
    kush::compile::QueryTranslator translator(query);
    auto executable_query = translator.Translate();
    executable_query.Compile();
    executable_query.Execute();
  }

  for (int i = 0; i < 5; i++) {
    auto start = std::chrono::system_clock::now();
    kush::compile::QueryTranslator translator(query);
    auto executable_query = translator.Translate();
    executable_query.Compile();
    executable_query.Execute();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cerr << duration.count() << std::endl;
  }
}

}  // namespace kush::util