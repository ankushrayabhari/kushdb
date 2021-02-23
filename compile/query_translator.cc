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
  auto vector_funcs = proxy::Vector<T>::ForwardDeclare(*program);

  // Forward declare hash functions
  auto hash_funcs = proxy::HashTable<T>::ForwardDeclare(*program, vector_funcs);

  // Forward declare print function
  auto print_funcs = proxy::Printer<T>::ForwardDeclare(*program);

  // Forward declare the column data implementations.
  absl::flat_hash_map<catalog::SqlType,
                      proxy::ForwardDeclaredColumnDataFunctions<T>>
      col_data_funcs;
  col_data_funcs.try_emplace(
      catalog::SqlType::SMALLINT,
      proxy::ColumnData<T, catalog::SqlType::SMALLINT>::ForwardDeclare(
          *program));
  col_data_funcs.try_emplace(
      catalog::SqlType::INT,
      proxy::ColumnData<T, catalog::SqlType::INT>::ForwardDeclare(*program));
  col_data_funcs.try_emplace(
      catalog::SqlType::BIGINT,
      proxy::ColumnData<T, catalog::SqlType::BIGINT>::ForwardDeclare(*program));
  col_data_funcs.try_emplace(
      catalog::SqlType::REAL,
      proxy::ColumnData<T, catalog::SqlType::REAL>::ForwardDeclare(*program));
  col_data_funcs.try_emplace(
      catalog::SqlType::DATE,
      proxy::ColumnData<T, catalog::SqlType::REAL>::ForwardDeclare(*program));
  col_data_funcs.try_emplace(
      catalog::SqlType::TEXT,
      proxy::ColumnData<T, catalog::SqlType::REAL>::ForwardDeclare(*program));
  col_data_funcs.try_emplace(
      catalog::SqlType::BOOLEAN,
      proxy::ColumnData<T, catalog::SqlType::REAL>::ForwardDeclare(*program));

  // Create the compute function
  program->CreateExternalFunction(program->VoidType(), {}, "compute");

  // Generate code for operator
  TranslatorFactory<T> factory(*program, col_data_funcs, print_funcs,
                               vector_funcs, hash_funcs);
  auto translator = factory.Compute(op_);
  translator->Produce();

  // terminate last basic block
  program->Return();

  return std::move(program);
}

}  // namespace kush::compile