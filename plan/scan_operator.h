#pragma once

#include <string>
#include <string_view>

#include "catalog/catalog.h"
#include "nlohmann/json.hpp"
#include "plan/operator.h"
#include "plan/operator_schema.h"
#include "plan/operator_visitor.h"

namespace kush::plan {

class ScanOperator final : public Operator {
 public:
  ScanOperator(OperatorSchema schema, const catalog::Table& relation);

  const catalog::Table& Relation() const;

  void Accept(OperatorVisitor& visitor) override;
  void Accept(ImmutableOperatorVisitor& visitor) const override;

  nlohmann::json ToJson() const override;

 private:
  const catalog::Table& relation_;
};

}  // namespace kush::plan