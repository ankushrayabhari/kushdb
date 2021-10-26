#pragma once

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "compile/query_translator.h"
#include "execution/executable_query.h"

namespace kush::util {

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

std::vector<std::string> Split(const std::string& s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(std::move(item));
  }
  return elems;
}

bool CHECK_EQ_TBL(
    const std::vector<std::string>& expected,
    const std::vector<std::string>& output,
    const std::vector<kush::plan::OperatorSchema::Column>& columns,
    double threshold = 1e-5) {
  if (expected.size() != output.size()) {
    std::cerr << "Differing table sizes: " << expected.size() << " vs "
              << output.size() << std::endl;
    return false;
  }

  for (int i = 0; i < expected.size(); i++) {
    auto split_e = Split(expected[i], '|');
    auto split_o = Split(output[i], '|');

    if (split_e.size() != columns.size()) {
      return false;
    }
    if (split_o.size() != columns.size()) {
      return false;
    }

    for (int j = 0; j < columns.size(); j++) {
      const auto& e_value = split_e[j];
      const auto& o_value = split_o[j];

      switch (columns[j].Expr().Type()) {
        case catalog::SqlType::REAL: {
          double e_value_as_d = std::stod(e_value);
          double o_value_as_d = std::stod(o_value);
          if (fabs(e_value_as_d - o_value_as_d) >= threshold) {
            std::cerr << "Expected column equal (" << i << "," << j
                      << ") : " << e_value << " vs " << o_value << std::endl;
            return false;
          }
          break;
        }

        case catalog::SqlType::SMALLINT:
        case catalog::SqlType::INT:
        case catalog::SqlType::BIGINT:
        case catalog::SqlType::DATE:
        case catalog::SqlType::TEXT:
        case catalog::SqlType::BOOLEAN:
          if (e_value != o_value) {
            std::cerr << "Expected column equal (" << i << "," << j
                      << ") : " << e_value << " vs " << o_value << std::endl;
            return false;
          }
          break;
      }
    }
  }

  return true;
}

}  // namespace kush::util