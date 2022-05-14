#include "compile/proxy/control_flow/if.h"

#include <functional>
#include <utility>

#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn) {
  if (cond.Get() == program.ConstI1(true)) {
    then_fn();
    return;
  }

  if (cond.Get() == program.ConstI1(false)) {
    return;
  }

  auto first_block = program.GenerateBlock();
  auto dest_block = program.GenerateBlock();
  program.Branch(cond.Get(), first_block, dest_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

void If(khir::ProgramBuilder& program, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn) {
  if (cond.Get() == program.ConstI1(true)) {
    then_fn();
    return;
  }

  if (cond.Get() == program.ConstI1(false)) {
    else_fn();
    return;
  }

  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();
  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(second_block);
  else_fn();

  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F&& f) {
  if constexpr (Start < End) {
    f(std::integral_constant<decltype(Start), Start>());
    constexpr_for<Start + Inc, End, Inc>(f);
  }
}

std::array<Int32, 2> Ternary(khir::ProgramBuilder& program, const Bool& cond,
                             std::function<std::array<Int32, 2>()> then_fn,
                             std::function<std::array<Int32, 2>()> else_fn) {
  if (cond.Get() == program.ConstI1(true)) {
    return then_fn();
  }

  if (cond.Get() == program.ConstI1(false)) {
    return else_fn();
  }

  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  const auto then_branch_value = then_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }

  std::array<khir::Type, 2> types;
  std::array<khir::Value, 2> then_branch_phi_members;
  for (int i = 0; i < 2; i++) {
    auto v = then_branch_value[i];
    then_branch_phi_members[i] = program.PhiMember(v.Get());
    types[i] = program.TypeOf(v.Get());
  }

  program.Branch(dest_block);

  program.SetCurrentBlock(second_block);
  const auto else_branch_value = else_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  std::array<khir::Value, 2> else_branch_phi_members;
  for (int i = 0; i < 2; i++) {
    auto v = else_branch_value[i];
    else_branch_phi_members[i] = program.PhiMember(v.Get());
  }
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
  std::array<khir::Value, 2> values;
  for (int i = 0; i < 2; i++) {
    auto then_branch_phi_member = then_branch_phi_members[i];
    auto else_branch_phi_member = else_branch_phi_members[i];
    values[i] = program.Phi(types[i]);
    program.UpdatePhiMember(values[i], then_branch_phi_member);
    program.UpdatePhiMember(values[i], else_branch_phi_member);
  }
  return std::array<Int32, 2>{Int32(program, values[0]),
                              Int32(program, values[1])};
}

template <typename T>
T TernaryImpl(khir::ProgramBuilder& program, const Bool& cond,
              std::function<T()> then_fn, std::function<T()> else_fn) {
  if (cond.Get() == program.ConstI1(true)) {
    return then_fn();
  }

  if (cond.Get() == program.ConstI1(false)) {
    return else_fn();
  }

  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  const auto then_branch_value = then_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  auto then_branch_phi_member = program.PhiMember(then_branch_value.Get());
  khir::Type phi_type = program.TypeOf(then_branch_value.Get());
  program.Branch(dest_block);

  program.SetCurrentBlock(second_block);
  const auto else_branch_value = else_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  auto else_branch_phi_member = program.PhiMember(else_branch_value.Get());
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
  auto phi_value = program.Phi(phi_type);
  program.UpdatePhiMember(phi_value, then_branch_phi_member);
  program.UpdatePhiMember(phi_value, else_branch_phi_member);
  return T(program, phi_value);
}

void If(khir::ProgramBuilder& program, Modifier m, const Bool& cond,
        std::function<void()> then_fn) {
  if (cond.Get() == program.ConstI1(false)) {
    then_fn();
    return;
  }

  if (cond.Get() == program.ConstI1(true)) {
    return;
  }

  auto first_block = program.GenerateBlock();
  auto dest_block = program.GenerateBlock();
  program.Branch(cond.Get(), dest_block, first_block);

  program.SetCurrentBlock(first_block);
  then_fn();
  if (!program.IsTerminated(program.CurrentBlock())) {
    program.Branch(dest_block);
  }

  program.SetCurrentBlock(dest_block);
}

void If(khir::ProgramBuilder& program, Modifier m, const Bool& cond,
        std::function<void()> then_fn, std::function<void()> else_fn) {
  If(program, cond, else_fn, then_fn);
}

Bool Ternary(khir::ProgramBuilder& program, const Bool& cond,
             std::function<Bool()> then_fn, std::function<Bool()> else_fn) {
  return TernaryImpl<Bool>(program, cond, then_fn, else_fn);
}

Int8 Ternary(khir::ProgramBuilder& program, const Bool& cond,
             std::function<Int8()> then_fn, std::function<Int8()> else_fn) {
  return TernaryImpl<Int8>(program, cond, then_fn, else_fn);
}

Int16 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int16()> then_fn, std::function<Int16()> else_fn) {
  return TernaryImpl<Int16>(program, cond, then_fn, else_fn);
}

Int32 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int32()> then_fn, std::function<Int32()> else_fn) {
  return TernaryImpl<Int32>(program, cond, then_fn, else_fn);
}

Int64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
              std::function<Int64()> then_fn, std::function<Int64()> else_fn) {
  return TernaryImpl<Int64>(program, cond, then_fn, else_fn);
}

Float64 Ternary(khir::ProgramBuilder& program, const Bool& cond,
                std::function<Float64()> then_fn,
                std::function<Float64()> else_fn) {
  return TernaryImpl<Float64>(program, cond, then_fn, else_fn);
}

String Ternary(khir::ProgramBuilder& program, const Bool& cond,
               std::function<String()> then_fn,
               std::function<String()> else_fn) {
  return TernaryImpl<String>(program, cond, then_fn, else_fn);
}

template <typename S>
SQLValue NullableTernary(khir::ProgramBuilder& program, const Bool& cond,
                         std::function<SQLValue()> then_fn,
                         std::function<SQLValue()> else_fn) {
  if (cond.Get() == program.ConstI1(true)) {
    return then_fn();
  }

  if (cond.Get() == program.ConstI1(false)) {
    return else_fn();
  }

  auto dest_block = program.GenerateBlock();

  auto first_block = program.GenerateBlock();
  auto second_block = program.GenerateBlock();

  program.Branch(cond.Get(), first_block, second_block);

  program.SetCurrentBlock(first_block);
  const auto then_branch_value = then_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  auto then_branch_phi_member =
      program.PhiMember(then_branch_value.Get().Get());
  auto then_branch_null_phi_member =
      program.PhiMember(then_branch_value.IsNull().Get());
  khir::Type phi_type = program.TypeOf(then_branch_value.Get().Get());
  khir::Type null_type = program.I1Type();
  program.Branch(dest_block);

  program.SetCurrentBlock(second_block);
  const auto else_branch_value = else_fn();
  if (program.IsTerminated(program.CurrentBlock())) {
    throw std::runtime_error("Current block cannot be terminated.");
  }
  if (else_branch_value.Type() != then_branch_value.Type()) {
    throw std::runtime_error("Must be same type returned from each branch.");
  }

  auto else_branch_phi_member =
      program.PhiMember(else_branch_value.Get().Get());
  auto else_branch_null_phi_member =
      program.PhiMember(else_branch_value.IsNull().Get());
  program.Branch(dest_block);

  program.SetCurrentBlock(dest_block);
  auto phi_value = program.Phi(phi_type);
  program.UpdatePhiMember(phi_value, then_branch_phi_member);
  program.UpdatePhiMember(phi_value, else_branch_phi_member);

  auto null_phi_value = program.Phi(null_type);
  program.UpdatePhiMember(null_phi_value, then_branch_null_phi_member);
  program.UpdatePhiMember(null_phi_value, else_branch_null_phi_member);

  if constexpr (std::is_same_v<S, Enum>) {
    return SQLValue(Enum(program, then_branch_value.Type().enum_id, phi_value),
                    Bool(program, null_phi_value));
  } else {
    return SQLValue(S(program, phi_value), Bool(program, null_phi_value));
  }
}

template SQLValue NullableTernary<Bool>(khir::ProgramBuilder& program,
                                        const Bool& cond,
                                        std::function<SQLValue()> then_fn,
                                        std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Int16>(khir::ProgramBuilder& program,
                                         const Bool& cond,
                                         std::function<SQLValue()> then_fn,
                                         std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Int32>(khir::ProgramBuilder& program,
                                         const Bool& cond,
                                         std::function<SQLValue()> then_fn,
                                         std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Int64>(khir::ProgramBuilder& program,
                                         const Bool& cond,
                                         std::function<SQLValue()> then_fn,
                                         std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Date>(khir::ProgramBuilder& program,
                                        const Bool& cond,
                                        std::function<SQLValue()> then_fn,
                                        std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Float64>(khir::ProgramBuilder& program,
                                           const Bool& cond,
                                           std::function<SQLValue()> then_fn,
                                           std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<String>(khir::ProgramBuilder& program,
                                          const Bool& cond,
                                          std::function<SQLValue()> then_fn,
                                          std::function<SQLValue()> else_fn);

template SQLValue NullableTernary<Enum>(khir::ProgramBuilder& program,
                                        const Bool& cond,
                                        std::function<SQLValue()> then_fn,
                                        std::function<SQLValue()> else_fn);

}  // namespace kush::compile::proxy