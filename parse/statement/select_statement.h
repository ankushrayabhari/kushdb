#pragma once
#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/statement.h"
#include "parse/table/table.h"

namespace kush::parse {

class SelectStatement : public Statement {
 public:
  SelectStatement(std::vector<std::unique_ptr<Expression>> selects,
                  std::unique_ptr<Table> from,
                  std::unique_ptr<Expression> where);

  const Table& From() const;
  const Expression* Where() const;
  std::vector<std::reference_wrapper<const Expression>> Selects() const;

 private:
  std::vector<std::unique_ptr<Expression>> selects_;
  std::unique_ptr<Table> from_;
  std::unique_ptr<Expression> where_;
};

}  // namespace kush::parse