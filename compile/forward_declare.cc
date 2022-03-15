#include "compile/forward_declare.h"

#include "catalog/sql_type.h"
#include "compile/proxy/column_data.h"
#include "compile/proxy/column_index.h"
#include "compile/proxy/hash_table.h"
#include "compile/proxy/skinner_join_executor.h"
#include "compile/proxy/skinner_scan_select_executor.h"
#include "compile/proxy/tuple_idx_table.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/vector.h"
#include "compile/translators/expression_translator.h"
#include "khir/program.h"
#include "khir/program_builder.h"

namespace kush::compile {

void ForwardDeclare(khir::ProgramBuilder& program) {
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
  proxy::ColumnIndexBucket::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::SMALLINT>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::INT>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::BIGINT>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::BOOLEAN>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::DATE>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::REAL>::ForwardDeclare(program);
  proxy::DiskColumnIndex<catalog::SqlType::TEXT>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::SMALLINT>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::INT>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::BIGINT>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::BOOLEAN>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::DATE>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::REAL>::ForwardDeclare(program);
  proxy::MemoryColumnIndex<catalog::SqlType::TEXT>::ForwardDeclare(program);

  proxy::TupleIdxTable::ForwardDeclare(program);
  proxy::SkinnerJoinExecutor::ForwardDeclare(program);
  proxy::SkinnerScanSelectExecutor::ForwardDeclare(program);
}

}  // namespace kush::compile