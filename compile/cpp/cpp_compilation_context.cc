#include "compile/cpp/cpp_compilation_context.h"

#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "catalog/catalog.h"
#include "compile/program.h"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"

namespace kush::compile {

CppCompilationContext::CppCompilationContext(const catalog::Database& db)
    : db_(db) {}

const catalog::Database& CppCompilationContext::Catalog() { return db_; }

CppProgram& CppCompilationContext::Program() { return program_; }

}  // namespace kush::compile