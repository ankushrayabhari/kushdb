#include "compile/query_translator.h"

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/llvm/llvm_ir.h"
#include "compile/program.h"
#include "compile/proxy/column_data.h"
#include "compile/translators/translator_factory.h"
#include "plan/operator.h"

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

std::unique_ptr<Program> QueryTranslator::Translate() {
  auto program = std::make_unique<ir::LLVMIr>();

  // Forward declare the column data implementations.
  absl::flat_hash_map<
      catalog::SqlType,
      proxy::ForwardDeclaredColumnDataFunctions<ir::LLVMIrTypes>>
      functions;
  functions.try_emplace(
      catalog::SqlType::SMALLINT,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::SMALLINT>::ForwardDeclare(*program));
  functions.try_emplace(
      catalog::SqlType::INT,
      proxy::ColumnData<ir::LLVMIrTypes, catalog::SqlType::INT>::ForwardDeclare(
          *program));
  functions.try_emplace(
      catalog::SqlType::BIGINT,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::BIGINT>::ForwardDeclare(*program));
  functions.try_emplace(
      catalog::SqlType::REAL,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::REAL>::ForwardDeclare(*program));
  functions.try_emplace(
      catalog::SqlType::DATE,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::REAL>::ForwardDeclare(*program));
  functions.try_emplace(
      catalog::SqlType::TEXT,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::REAL>::ForwardDeclare(*program));
  functions.try_emplace(
      catalog::SqlType::BOOLEAN,
      proxy::ColumnData<ir::LLVMIrTypes,
                        catalog::SqlType::REAL>::ForwardDeclare(*program));

  // Create the compute function
  program->CreateFunction(program->VoidType(), {}, "compute");

  // Generate code for operator
  TranslatorFactory<ir::LLVMIrTypes> factory(*program, functions);
  auto translator = factory.Compute(op_);
  translator->Produce();

  return std::move(program);
}

}  // namespace kush::compile