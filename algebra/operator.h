#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace skinner {

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
  std::string expression;
  std::unique_ptr<Operator> child;
  Select(std::unique_ptr<Operator> c, const std::string& e);
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

}  // namespace skinner