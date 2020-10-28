#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "plan/expression/expression.h"

namespace kush {
namespace plan {

class Operator {
 public:
  Operator* parent;
  Operator();
  virtual ~Operator() {}
  virtual std::string Id() const = 0;
  virtual void Print(std::ostream& out, int num_indent = 0) const = 0;
};

class Scan final : public Operator {
 public:
  static const std::string ID;
  const std::string relation;

  Scan(const std::string& rel);
  std::string Id() const;
  void Print(std::ostream& out, int num_indent) const;
};

class Select final : public Operator {
 public:
  static const std::string ID;
  std::unique_ptr<Expression> expression;
  std::unique_ptr<Operator> child;
  Select(std::unique_ptr<Operator> c, std::unique_ptr<Expression> e);
  std::string Id() const;
  void Print(std::ostream& out, int num_indent) const;
};

class Output final : public Operator {
 public:
  static const std::string ID;
  std::unique_ptr<Operator> child;
  Output(std::unique_ptr<Operator> c);
  std::string Id() const;
  void Print(std::ostream& out, int num_indent) const;
};

class HashJoin final : public Operator {
 public:
  static const std::string ID;
  std::unique_ptr<Operator> left;
  std::unique_ptr<Operator> right;
  std::unique_ptr<BinaryExpression> expression;

  HashJoin(std::unique_ptr<Operator> l, std::unique_ptr<Operator> r,
           std::unique_ptr<BinaryExpression> e);
  std::string Id() const;
  void Print(std::ostream& out, int num_indent) const;
};

}  // namespace plan
}  // namespace kush