#pragma once

#include <functional>
#include <utility>

#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn);

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn);

std::vector<khir::Value> Ternary(
    khir::ProgramBuilder& program, const Bool& cond,
    std::function<std::vector<khir::Value>()> then_fn,
    std::function<std::vector<khir::Value>()> else_fn);

}  // namespace kush::compile::proxy