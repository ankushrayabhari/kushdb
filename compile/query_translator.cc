#include "compile/query_translator.h"

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/llvm/llvm_ir.h"
#include "compile/program.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/vector.h"
#include "compile/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

std::unique_ptr<Program> QueryTranslator::Translate() {
  auto program = std::make_unique<ir::LLVMIr>();

  using T = ir::LLVMIrTypes;

  // Forward declare vector functions
  proxy::Vector<T>::ForwardDeclare(*program);

  // Forward declare hash functions
  proxy::BucketList<T>::ForwardDeclare(*program);
  proxy::HashTable<T>::ForwardDeclare(*program);

  // Forward declare print function
  proxy::Printer<T>::ForwardDeclare(*program);

  // Forward declare the column data implementations.
  proxy::ColumnData<T, catalog::SqlType::SMALLINT>::ForwardDeclare(*program);
  proxy::ColumnData<T, catalog::SqlType::INT>::ForwardDeclare(*program);
  proxy::ColumnData<T, catalog::SqlType::BIGINT>::ForwardDeclare(*program);
  proxy::ColumnData<T, catalog::SqlType::BOOLEAN>::ForwardDeclare(*program);
  proxy::ColumnData<T, catalog::SqlType::DATE>::ForwardDeclare(*program);
  proxy::ColumnData<T, catalog::SqlType::REAL>::ForwardDeclare(*program);

  // Create the compute function
  program->CreatePublicFunction(program->VoidType(), {}, "compute");

  // Generate code for operator
  TranslatorFactory<T> factory(*program);
  auto translator = factory.Compute(op_);
  translator->Produce();

  // terminate last basic block
  program->Return();

  return std::move(program);
}

}  // namespace kush::compile