#include "compile/cpp_program.h"

#include <dlfcn.h>

#include <fstream>
#include <functional>
#include <string>

namespace kush {
namespace compile {

CppProgram::CppProgram()
    : file_name_("/tmp/query.cpp"), dylib_("/tmp/query.so") {}

void CppProgram::CodegenInitialize() {
  fout.open(file_name_);
  fout << "#include \"data/column_data.h\"\n";
  fout << "#include \"util/hash_util.h\"\n";
  fout << "#include <iostream>\n";
  fout << "#include <cstdint>\n";
  fout << "#include <vector>\n";
  fout << "#include <string>\n";
  fout << "#include <string_view>\n";
  fout << "#include <unordered_map>\n";
  fout << "extern \"C\" void compute() {\n";
}

void CppProgram::CodegenFinalize() {
  fout << "}\n";
  fout.close();
}

std::string CppProgram::GenerateVariable() {
  if (last_variable_.empty()) {
    last_variable_ = "a";
  } else {
    last_variable_.back()++;
    if (last_variable_.back() > 'z') {
      last_variable_.back() = 'z';
      last_variable_.push_back('a');
    }
  }

  return last_variable_;
}

void CppProgram::Compile() {
  std::string command = "clang++ -O3 --std=c++17 -I. -shared -fpic " +
                        file_name_ + " -o " + dylib_;
  if (system(command.c_str()) != 0) {
    throw std::runtime_error("Failed to compile file.");
  }
}

void CppProgram::Execute() {
  void* handle = dlopen((dylib_).c_str(), RTLD_LAZY);

  if (!handle) {
    throw std::runtime_error("Failed to open");
  }

  using compute_fn = std::add_pointer<void()>::type;
  auto process_query = (compute_fn)(dlsym(handle, "compute"));
  if (!process_query) {
    dlclose(handle);
    throw std::runtime_error("Failed to get compute fn");
  }

  process_query();
  dlclose(handle);
}

}  // namespace compile
}  // namespace kush