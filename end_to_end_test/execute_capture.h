#pragma once

#include <chrono>
#include <fstream>
#include <iostream>

#include "compile/query_translator.h"

std::string ExecuteAndCapture(kush::plan::Operator& query) {
  auto unique = std::chrono::system_clock::now().time_since_epoch().count();
  auto test_file = "/tmp/query_output_test" + std::to_string(unique) + ".csv";

  std::fstream buffer(test_file, std::fstream::out | std::fstream::trunc);
  std::streambuf* sbuf = std::cout.rdbuf();
  std::cout.rdbuf(buffer.rdbuf());

  kush::compile::QueryTranslator translator(query);
  auto [codegen, prog] = translator.Translate();
  prog->Compile();
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(prog->GetFunction("compute"));
  compute();

  std::cout.rdbuf(sbuf);

  buffer.close();

  return test_file;
}

std::string GetFileContents(const std::string& filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in) {
    std::ostringstream contents;
    contents << in.rdbuf();
    in.close();
    return (contents.str());
  }
  throw(errno);
}