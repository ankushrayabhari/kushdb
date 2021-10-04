#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "compile/proxy/struct.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

class ComparisonFunction {
 public:
  ComparisonFunction(
      khir::ProgramBuilder& program, StructBuilder element,
      std::function<void(Struct&, Struct&, std::function<void(Bool)>)> body);
  khir::FunctionRef Get();

 private:
  std::optional<khir::FunctionRef> func;
};

}  // namespace kush::compile::proxy