#pragma once

#include <chrono>
#include <iostream>

#include "compile/query_translator.h"

namespace kush::util {

void ExecuteAndTime(kush::plan::Operator& query) {
  auto start = std::chrono::system_clock::now();
  kush::compile::QueryTranslator translator(query);
  auto [codegen, prog] = translator.Translate();
  prog->Compile();
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(prog->GetFunction("compute"));
  compute();
  auto end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> duration = end - start;
  std::cerr << "Total: " << duration.count() << std::endl;
}

}