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

proxy::Bool Ternary(khir::ProgramBuilder& program, const Bool& cond,
                    std::function<proxy::Bool()> then_fn,
                    std::function<proxy::Bool()> else_fn);

proxy::Int8 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                    std::function<proxy::Int8()> then_fn,
                    std::function<proxy::Int8()> else_fn);

proxy::Int16 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int16()> then_fn,
                     std::function<proxy::Int16()> else_fn);

proxy::Int32 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int32()> then_fn,
                     std::function<proxy::Int32()> else_fn);

proxy::Int64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                     std::function<proxy::Int64()> then_fn,
                     std::function<proxy::Int64()> else_fn);

proxy::Float64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                       std::function<proxy::Float64()> then_fn,
                       std::function<proxy::Float64()> else_fn);

proxy::String Ternary(khir::ProgramBuilder& program, const Bool& cond,
                      std::function<proxy::String()> then_fn,
                      std::function<proxy::String()> else_fn);

}  // namespace kush::compile::proxy