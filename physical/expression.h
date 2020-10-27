#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace kush {
namespace physical {

// Basic expressions:
// - Operators like +,-,*
// - Functions like SUM,MIN,MAX,CAST
// - Column References
// - Constants
class Expression {
 public:
  virtual ~Expression() {}
  virtual std::string Id() const = 0;
  virtual void Print(std::ostream& out, int num_indent) const = 0;
};

class IntLiteral final : public Expression {
 public:
  const static std::string ID;
  const int64_t value;
  explicit IntLiteral(int64_t v);
  void Print(std::ostream& out, int num_indent) const;
  std::string Id() const;
};

class ColumnRef final : public Expression {
 public:
  const static std::string ID;
  const std::string table;
  const std::string column;
  explicit ColumnRef(const std::string& tab, const std::string& col);
  void Print(std::ostream& out, int num_indent) const;
  std::string Id() const;
};

enum class BinaryOperatorType {
  ADD,
  AND,
  DIV,
  EQ,
  GT,
  GTE,
  LT,
  LTE,
  MUL,
  NEQ,
  OR,
  SUB,
  MOD,
  XOR
};

class BinaryExpression final : public Expression {
 public:
  const static std::string ID;
  BinaryOperatorType type;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;

  BinaryExpression(const BinaryOperatorType& op_type,
                   std::unique_ptr<Expression> left,
                   std::unique_ptr<Expression> right);
  void Print(std::ostream& out, int num_indent) const;
  std::string Id() const;
};

}  // namespace physical
}  // namespace kush