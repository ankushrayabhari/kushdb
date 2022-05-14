#pragma once

#include <functional>
#include <utility>

#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile {

enum Modifier { NOT };

}  // namespace kush::compile

namespace kush::compile::proxy {

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn);

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn);

void If(khir::ProgramBuilder& program, Modifier m, const Bool& cond,
        std::function<void()> then_fn);

void If(khir::ProgramBuilder& program, Modifier m, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn);

Bool Ternary(khir::ProgramBuilder& program, const Bool& cond,
             std::function<Bool()> then_fn, std::function<Bool()> else_fn);

Int8 Ternary(khir::ProgramBuilder& program, const Bool& cond,
             std::function<Int8()> then_fn, std::function<Int8()> else_fn);

Int16 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int16()> then_fn, std::function<Int16()> else_fn);

Int32 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int32()> then_fn, std::function<Int32()> else_fn);

Int64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int64()> then_fn, std::function<Int64()> else_fn);

Float64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                std::function<Float64()> then_fn,
                std::function<Float64()> else_fn);

String Ternary(khir::ProgramBuilder& program, const Bool& cond,
               std::function<String()> then_fn,
               std::function<String()> else_fn);

template <typename T, std::size_t N>
std::array<T, N> Ternary(khir::ProgramBuilder& program, const Bool& cond,
                         std::function<std::array<T, N>()> then_fn,
                         std::function<std::array<T, N>()> else_fn);

template <typename S>
SQLValue NullableTernary(khir::ProgramBuilder& program, const Bool& cond,
                         std::function<SQLValue()> then_fn,
                         std::function<SQLValue()> else_fn);

}  // namespace kush::compile::proxy