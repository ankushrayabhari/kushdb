#include "algebra/operator.h"

#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "util/print_util.h"

namespace skinner {
namespace algebra {

TableScan::TableScan(absl::string_view relation)
    : name_("TABLE SCAN"), relation_(relation) {}

void TableScan::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << name_ << ": " << relation_ << std::endl;
}

Select::Select(std::unique_ptr<Operator> child, absl::string_view expression)
    : name_("SELECT"), expression_(expression), child_(std::move(child)) {}

void Select::Print(std::ostream& out, int num_indent) const {
  util::Indent(out, num_indent) << name_ << std::endl;
  util::Indent(out, num_indent + 1) << expression_ << std::endl;
  child_->Print(out, num_indent + 1);
}

}  // namespace algebra
}  // namespace skinner