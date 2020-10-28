#include "plan/operator.h"

#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "util/print_util.h"

namespace kush::plan {

Operator::Operator() : parent(nullptr) {}

Scan::Scan(const std::string& rel) : relation(rel) {}

const std::string Scan::ID = "SCAN";

std::string Scan::Id() const { return ID; }

void Scan::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << ID << ": " << relation << std::endl;
}

Select::Select(std::unique_ptr<Operator> c, std::unique_ptr<Expression> e)
    : expression(std::move(e)), child(std::move(c)) {
  child->parent = this;
}

const std::string Select::ID = "SELECT";

std::string Select::Id() const { return ID; }

void Select::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << ID << std::endl;
  expression->Print(out, num_indent + 1);
  child->Print(out, num_indent + 1);
}

Output::Output(std::unique_ptr<Operator> c) : child(std::move(c)) {
  child->parent = this;
}

const std::string Output::ID = "OUTPUT";

std::string Output::Id() const { return ID; }

void Output::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << ID << std::endl;
  child->Print(out, num_indent + 1);
}

HashJoin::HashJoin(std::unique_ptr<Operator> l, std::unique_ptr<Operator> r,
                   std::unique_ptr<BinaryExpression> e)
    : left(std::move(l)), right(std::move(r)), expression(std::move(e)) {
  left->parent = this;
  right->parent = this;
}

const std::string HashJoin::ID = "HASH_JOIN";

std::string HashJoin::Id() const { return ID; }

void HashJoin::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << ID << std::endl;
  left->Print(out, num_indent + 1);
  right->Print(out, num_indent + 1);
  expression->Print(out, num_indent + 1);
}

}  // namespace kush::plan