#include "parse/transform/transform_order_by.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/select_statement.h"
#include "parse/transform/transform_expression.h"
#include "third_party/duckdb_libpgquery/parser.h"

namespace kush::parse {

std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> TransformOrderBy(
    duckdb_libpgquery::PGList& order_by) {
  std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> result;
  for (auto node = order_by.head; node != nullptr; node = node->next) {
    auto temp =
        reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);

    if (temp->type != duckdb_libpgquery::T_PGSortBy) {
      throw std::runtime_error("ORDER BY node type expected.");
    }

    OrderType type;
    auto sort = reinterpret_cast<duckdb_libpgquery::PGSortBy*>(temp);
    auto target = sort->node;
    if (sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_DEFAULT) {
      type = OrderType::DEFAULT;
    } else if (sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_ASC) {
      type = OrderType::ASCENDING;
    } else if (sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_DESC) {
      type = OrderType::DESCENDING;
    } else {
      throw std::runtime_error("Unimplemented order by type.");
    }

    if (sort->sortby_nulls != duckdb_libpgquery::PG_SORTBY_NULLS_DEFAULT) {
      throw std::runtime_error("NULL ordering not supported.");
    }

    if (target == nullptr) {
      throw std::runtime_error("Missing target in order by.");
    }

    result.emplace_back(TransformExpression(*target), type);
  }

  return result;
}

}  // namespace kush::parse