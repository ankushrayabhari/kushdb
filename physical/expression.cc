#include "physical/expression.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "third_party/magic_enum.h"
#include "util/print_util.h"

namespace kush {
namespace physical {

const std::string IntLiteral::ID = "INT_LITERAL";

IntLiteral::IntLiteral(int64_t v) : value(v) {}

void IntLiteral::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << value << std::endl;
}

std::string IntLiteral::Id() const { return ID; }

const std::string ColumnRef::ID = "COLUMN_REF";

ColumnRef::ColumnRef(const std::string& tab, const std::string& col)
    : table(tab), column(col) {}

void ColumnRef::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << column << std::endl;
}

std::string ColumnRef::Id() const { return ID; }

const std::string BinaryExpression::ID = "BINOP";

BinaryExpression::BinaryExpression(const BinaryOperatorType& t,
                                   std::unique_ptr<Expression> l,
                                   std::unique_ptr<Expression> r)
    : type(t), left(std::move(l)), right(std::move(r)) {}

void BinaryExpression::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << magic_enum::enum_name(type) << std::endl;
  left->Print(out, num_indent + 1);
  right->Print(out, num_indent + 1);
}

std::string BinaryExpression::Id() const { return ID; }

}  // namespace physical
}  // namespace kush