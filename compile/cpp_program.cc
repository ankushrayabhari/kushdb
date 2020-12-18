#include "compile/cpp_program.h"

#include <dlfcn.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>

namespace kush {
namespace compile {

CppProgram::CppProgram()
    : file_name_("/tmp/query.cpp"), dylib_("/tmp/query.so") {}

void CppProgram::CodegenInitialize() {
  start = std::chrono::system_clock::now();
  fout.open(file_name_);
  fout << "#include \"data/column_data.h\"\n";
  fout << "#include \"util/hash_util.h\"\n";
  fout << "#include <iostream>\n";
  fout << "#include <iomanip>\n";
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
  gen = std::chrono::system_clock::now();
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
  std::string command = "clang++ -w -O3 --std=c++17 -I. -shared -fpic " +
                        file_name_ + " -o " + dylib_;
  if (system(command.c_str()) != 0) {
    throw std::runtime_error("Failed to compile file.");
  }
  comp = std::chrono::system_clock::now();
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
  link = std::chrono::system_clock::now();

  process_query();
  dlclose(handle);

  end = std::chrono::system_clock::now();
  std::cerr << "Performance stats (ms):" << std::endl;
  std::chrono::duration<double, std::milli> elapsed_seconds = gen - start;
  std::cerr << "Code generation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = comp - gen;
  std::cerr << "Compilation: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = link - comp;
  std::cerr << "Linking: " << elapsed_seconds.count() << std::endl;
  elapsed_seconds = end - link;
  std::cerr << "Execution: " << elapsed_seconds.count() << std::endl;
}

}  // namespace compile
}  // namespace kush