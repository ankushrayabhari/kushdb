#pragma once

#include <functional>
#include <utility>

#include "compile/khir/program_builder.h"
#include "compile/proxy/bool.h"

namespace kush::compile::proxy {

std::vector<khir::Value> If(khir::ProgramBuilder& program, const Bool& cond,
                            std::function<std::vector<khir::Value>()> then_fn,
                            std::function<std::vector<khir::Value>()> else_fn);

}  // namespace kush::compile::proxy