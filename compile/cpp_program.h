#pragma once

#include <fstream>
#include <string>

#include "compile/program.h"

namespace kush {
namespace compile {

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

  friend class If;
  friend class For;
};

}  // namespace compile
}  // namespace kush