#pragma once

#include <fstream>
#include <string>

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

class CppProgram {
 public:
  CppProgram();

  void CodegenInitialize();
  void CodegenFinalize();
  void Compile();
  void Execute();

  If& GenerateIf();
  For& GenerateFor();

  std::ofstream fout;

 private:
  std::string file_name_;
  std::string dylib_;

  friend class If;
  friend class For;
};

}  // namespace compile
}  // namespace kush