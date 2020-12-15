#pragma once

#include <fstream>
#include <string>

#include "compilation/program.h"

namespace kush {
namespace compile {

class CppProgram;

class If {
 public:
  If(CppProgram& program);

  void Begin();
  void Body();
  void End();

 private:
  CppProgram& program_;
};

class For {
 public:
  For(CppProgram& program);

  void Begin();
  void Condition();
  void Update();
  void Body();
  void End();

 private:
  CppProgram& program_;
};

class CppProgram : public Program {
 public:
  CppProgram();

  void CodegenInitialize();
  void CodegenFinalize();
  void Compile() override;
  void Execute() override;

  If GenerateIf();
  For GenerateFor();
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