#include "compile/query_translator.h"

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "compile/program.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/printer.h"
#include "compile/proxy/skinner_join_executor.h"
#include "compile/proxy/string.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "compile/translators/translator_factory.h"
#include "khir/asm/asm_backend.h"
#include "khir/llvm/llvm_backend.h"
#include "khir/program_builder.h"
#include "khir/program_printer.h"
#include "plan/operator.h"

ABSL_FLAG(std::string, backend, "asm", "Backend to choose (asm or llvm)");

namespace kush::compile {

QueryTranslator::QueryTranslator(const plan::Operator& op) : op_(op) {}

std::unique_ptr<Program> QueryTranslator::Translate() {
  khir::ProgramBuilder program;

  // Forward declare expression translator helper functions
  ExpressionTranslator::ForwardDeclare(program);

  // Forward declare string functions
  proxy::String::ForwardDeclare(program);

  // Forward declare vector functions
  proxy::Vector::ForwardDeclare(program);

  // Forward declare hash functions
  proxy::HashTable::ForwardDeclare(program);

  // Forward declare print function
  proxy::Printer::ForwardDeclare(program);

  // Forward declare the column data implementations.
  proxy::ColumnData<catalog::SqlType::SMALLINT>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::INT>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::BIGINT>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::BOOLEAN>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::DATE>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::REAL>::ForwardDeclare(program);
  proxy::ColumnData<catalog::SqlType::TEXT>::ForwardDeclare(program);

  // Forward declare the column index implementations
  proxy::ColumnIndexImpl<catalog::SqlType::SMALLINT>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::INT>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::BIGINT>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::BOOLEAN>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::DATE>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::REAL>::ForwardDeclare(program);
  proxy::ColumnIndexImpl<catalog::SqlType::TEXT>::ForwardDeclare(program);

  proxy::SkinnerJoinExecutor::ForwardDeclare(program);
  proxy::TupleIdxTable::ForwardDeclare(program);

  // Create the compute function
  program.CreatePublicFunction(program.VoidType(), {}, "compute");

  // Generate code for operator
  TranslatorFactory factory(program);
  auto translator = factory.Compute(op_);
  translator->Produce();

  // terminate last basic block
  program.Return();

  // khir::ProgramPrinter printer;
  // program.Translate(printer);

  if (FLAGS_backend.CurrentValue() == "asm") {
    auto backend =
        std::make_unique<khir::ASMBackend>(khir::RegAllocImpl::LINEAR_SCAN);
    program.Translate(*backend);
    backend->Compile();
    return std::move(backend);
  }

  if (FLAGS_backend.CurrentValue() == "llvm") {
    auto backend = std::make_unique<khir::LLVMBackend>();
    program.Translate(*backend);
    return std::move(backend);
  }

  throw std::runtime_error("Unknown backend.");
}

}  // namespace kush::compile