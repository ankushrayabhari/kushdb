#pragma once

#include <cassert>
#include <chrono>
#include <iostream>

#include "compile/query_translator.h"
#include "execution/executable_query.h"
#include "plan/output_operator.h"
#include "util/test_util.h"

namespace kush::util {

void BenchVerify(kush::plan::OutputOperator& query,
                 const std::string& expected_file) {
  {
    // Verify (counts as a warmup)
    auto output_file = ExecuteAndCapture(query);
    auto expected = GetFileContents(expected_file);
    auto output = GetFileContents(output_file);
    assert(CHECK_EQ_TBL(expected, output, query.Child().Schema().Columns()));
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