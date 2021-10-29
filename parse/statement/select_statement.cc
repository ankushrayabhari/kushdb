#include "parse/statement/select_statement.h"

#include <memory>
#include <vector>

#include "parse/expression/expression.h"
#include "parse/statement/statement.h"
#include "parse/table/table.h"
#include "util/vector_util.h"

namespace kush::parse {

SelectStatement::SelectStatement(
    std::vector<std::unique_ptr<Expression>> selects,
    std::unique_ptr<Table> from, std::unique_ptr<Expression> where)
    : selects_(std::move(selects)),
      from_(std::move(from)),
      where_(std::move(where)) {}

const Table& SelectStatement::From() const { return *from_; }

const Expression* SelectStatement::Where() const { return where_.get(); }

std::vector<std::reference_wrapper<const Expression>> SelectStatement::Selects()
    const {
  return util::ImmutableReferenceVector(selects_);
}

}  // namespace kush::parse