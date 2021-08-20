#pragma once

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include "compile/query_translator.h"

class RedirectStdout {
 public:
  RedirectStdout(int fd) : old_stdout(dup(1)) { dup2(fd, 1); }

  ~RedirectStdout() {
    dup2(old_stdout, 1);
    close(old_stdout);
  }

 private:
  int old_stdout;
};

std::string ExecuteAndCapture(kush::plan::Operator& query) {
  auto unique = std::chrono::system_clock::now().time_since_epoch().count();
  auto test_file = "/tmp/query_output_test" + std::to_string(unique) + ".csv";

  int fd = open(test_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
  RedirectStdout redirect(fd);

  kush::compile::QueryTranslator translator(query);
  auto [codegen, prog] = translator.Translate();
  prog->Compile();
  using compute_fn = std::add_pointer<void()>::type;
  auto compute = reinterpret_cast<compute_fn>(prog->GetFunction("compute"));
  compute();

  std::cout.flush();

  close(fd);

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