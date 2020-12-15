#include "compilation/compilation_context.h"

#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "catalog/catalog.h"
#include "compilation/program.h"
#include "plan/expression/expression.h"
#include "plan/operator.h"
#include "plan/operator_schema.h"

namespace kush::compile {

CompliationContext::CompliationContext(const catalog::Database& db) : db_(db) {}

const catalog::Database& CompliationContext::Catalog() { return db_; }

CppProgram& CompliationContext::Program() { return program_; }

}  // namespace kush::compile