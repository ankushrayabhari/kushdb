#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"
#include "compile/proxy/struct.h"

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