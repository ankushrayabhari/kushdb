#pragma once

#include <chrono>
#include <iostream>

#include "compile/query_translator.h"

namespace kush::util {

void ExecuteAndTime(kush::plan::Operator& query) {
  auto start = std::chrono::system_clock::now();
  kush::compile::QueryTranslator translator(query);
  auto [codegen, prog] = translator.Translate();
  auto gen = std::chrono::system_clock::now();
  prog->Compile();
  auto comp = std::chrono::system_clock::now();
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(prog->GetFunction("compute"));
  compute();
  auto end = std::chrono::system_clock::now();

  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> duration = gen - start;
  std::cerr << "Generation: " << duration.count() << std::endl;
  duration = comp - gen;
  std::cerr << "Compilation: " << duration.count() << std::endl;
  duration = end - comp;
  std::cerr << "Execution: " << duration.count() << std::endl;
  duration = end - start;
  std::cerr << "Total: " << duration.count() << std::endl;
}

}