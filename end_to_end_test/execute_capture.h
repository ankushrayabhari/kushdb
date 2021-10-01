#pragma once

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "compile/query_translator.h"
#include "execution/executable_query.h"

class RedirectStdout {
 public:
  RedirectStdout(int fd) : old_stdout_(dup(1)) { dup2(fd, 1); }

  ~RedirectStdout() {
    dup2(old_stdout_, 1);
    close(old_stdout_);
  }

 private:
  int old_stdout_;
};

std::string ExecuteAndCapture(kush::plan::Operator& query) {
  auto unique = std::chrono::system_clock::now().time_since_epoch().count();
  auto test_file = "/tmp/query_output_test" + std::to_string(unique) + ".csv";

  int fd = open(test_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0660);
  RedirectStdout redirect(fd);

  kush::compile::QueryTranslator translator(query);
  auto executable_query = translator.Translate();
  executable_query.Compile();
  executable_query.Execute();

  std::cout.flush();

  close(fd);

  return test_file;
}

std::vector<std::string> GetFileContents(const std::string& filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);

  std::vector<std::string> output;
  std::string str;
  while (std::getline(in, str)) {
    output.push_back(str);
  }

  return output;
}