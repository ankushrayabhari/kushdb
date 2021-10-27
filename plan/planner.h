#pragma once

#include "parse/statement/select_statement.h"
#include "parse/table/table.h"
#include "plan/operator/operator.h"

namespace kush::plan {

std::unique_ptr<Operator> Plan(const parse::SelectStatement& stmt);

}  // namespace kush::plan