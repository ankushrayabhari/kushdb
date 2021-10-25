#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/select_statement.h"
#include "parse/transform/transform_expression.h"
#include "parse/transform/transform_order_by.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::vector<std::unique_ptr<Expression>> TransformGroupBy(
    duckdb_libpgquery::PGList& group_by) {
  std::vector<std::unique_ptr<Expression>> result;
  for (auto node = group_by.head; node != nullptr; node = node->next) {
    auto n = reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
    if (n->type == duckdb_libpgquery::T_PGGroupingSet) {
      throw std::runtime_error("GROUPING SET not supported.");
    }

    result.push_back(TransformExpression(*n));
  }
  return result;
}

}  // namespace kush::parse