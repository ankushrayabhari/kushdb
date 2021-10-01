#pragma once

#include <chrono>
#include <iostream>

#include "compile/query_translator.h"
#include "execution/executable_query.h"

namespace kush::util {

void ExecuteAndTime(kush::plan::Operator& query) {
  auto start = std::chrono::system_clock::now();
  kush::compile::QueryTranslator translator(query);
  auto executable_query = translator.Translate();
  executable_query.Compile();
  executable_query.Execute();
  auto end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> duration = end - start;
  std::cerr << "Total: " << duration.count() << std::endl;
}

}  // namespace kush::util