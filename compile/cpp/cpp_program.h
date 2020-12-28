#pragma once

#include <chrono>
#include <fstream>
#include <string>

#include "compile/program.h"

namespace kush::compile::cpp {

class CppProgram : public Program {
 public:
  CppProgram();

  void CodegenInitialize();
  void CodegenFinalize();
  void Compile() override;
  void Execute() override;

  std::string GenerateVariable();

  std::ofstream fout;

 private:
  std::string file_name_;
  std::string dylib_;
  std::string last_variable_;

  std::chrono::time_point<std::chrono::system_clock> start, gen, comp, link,
      end;
};

}  // namespace kush::compile::cpp