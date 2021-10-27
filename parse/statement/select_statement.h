#pragma once
#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/statement.h"
#include "parse/table/table.h"

namespace kush::parse {

enum class OrderType { DEFAULT, ASCENDING, DESCENDING };

class SelectStatement : public Statement {
 public:
  SelectStatement(
      std::vector<std::unique_ptr<Expression>> selects,
      std::unique_ptr<Table> from, std::unique_ptr<Expression> where,
      std::vector<std::unique_ptr<Expression>> group_by,
      std::unique_ptr<Expression> having,
      std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> order_by);

  const Table& From() const;
  const Expression* Where() const;

 private:
  std::vector<std::unique_ptr<Expression>> selects_;
  std::unique_ptr<Table> from_;
  std::unique_ptr<Expression> where_;
  std::vector<std::unique_ptr<Expression>> group_by_;
  std::unique_ptr<Expression> having_;
  std::vector<std::pair<std::unique_ptr<Expression>, OrderType>> order_by_;
};

}  // namespace kush::parse