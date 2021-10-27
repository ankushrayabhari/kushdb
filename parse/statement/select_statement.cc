#include "parse/statement/select_statement.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/statement.h"
#include "parse/table/table.h"

namespace kush::parse {

SelectStatement::SelectStatement(
    std::vector<std::unique_ptr<Expression>> selects,
    std::unique_ptr<Table> from, std::unique_ptr<Expression> where,
    std::vector<std::unique_ptr<Expression>> group_by,
    std::unique_ptr<Expression> having,
    std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> order_by)
    : selects_(std::move(selects)),
      from_(std::move(from)),
      where_(std::move(where)),
      group_by_(std::move(group_by)),
      having_(std::move(having)),
      order_by_(std::move(order_by)) {}

const Table& SelectStatement::From() const { return *from_; }

const Expression* SelectStatement::Where() const { return where_.get(); }

}  // namespace kush::parse