#include "compile/proxy/tuple_idx_table.h"

#include <functional>

#include "compile/proxy/int.h"
#include "compile/proxy/loop.h"
#include "khir/program_builder.h"
#include "runtime/tuple_idx_table.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

constexpr std::string_view create_fn_name(
    "_ZN4kush7runtime13TupleIdxTable6CreateEv");
constexpr std::string_view insert_fn_name(
    "_ZN4kush7runtime13TupleIdxTable6InsertEPSt13unordered_"
    "setISt6vectorIiSaIiEESt4hashIS5_ESt8equal_toIS5_ESaIS5_EEPii");
constexpr std::string_view free_fn_name(
    "_ZN4kush7runtime13TupleIdxTable4FreeEPSt13unordered_"
    "setISt6vectorIiSaIiEESt4hashIS5_ESt8equal_toIS5_ESaIS5_EE");
constexpr std::string_view size_fn_name(
    "_ZN4kush7runtime13TupleIdxTable4SizeEPSt13unordered_"
    "setISt6vectorIiSaIiEESt4hashIS5_ESt8equal_toIS5_ESaIS5_EE");
constexpr std::string_view begin_fn_name(
    "_ZN4kush7runtime13TupleIdxTable5BeginEPSt13unordered_"
    "setISt6vectorIiSaIiEESt4hashIS5_ESt8equal_toIS5_ESaIS5_EEPPNSt8__detail20_"
    "Node_const_iteratorIS5_Lb1ELb1EEE");
constexpr std::string_view increment_fn_name(
    "_ZN4kush7runtime13TupleIdxTable9IncrementEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");
constexpr std::string_view get_fn_name(
    "_ZN4kush7runtime13TupleIdxTable3GetEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");
constexpr std::string_view free_it_fn_name(
    "_ZN4kush7runtime13TupleIdxTable6FreeItEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");

GlobalTupleIdxTable::GlobalTupleIdxTable(khir::ProgramBuilder& program)
    : program_(program),
      generator_(program.Global(
          false, true, program.PointerType(program.I8Type()),
          program.NullPtr(program.PointerType(program.I8Type())))) {
  auto tuple_idx_table = program_.Call(program_.GetFunction(create_fn_name));
  program_.StorePtr(generator_(), tuple_idx_table);
}

void GlobalTupleIdxTable::Reset() {
  auto tuple_idx_table = program_.LoadPtr(generator_());
  program_.Call(program_.GetFunction(free_fn_name), {tuple_idx_table});
}

TupleIdxTable GlobalTupleIdxTable::Get() {
  auto value = generator_();
  return TupleIdxTable(program_, value);
}

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program, khir::Value value)
    : program_(program), value_(value) {}

void TupleIdxTable::Insert(const khir::Value& idx_arr, Int32& num_tables) {
  auto tuple_idx_table = program_.LoadPtr(value_);
  program_.Call(program_.GetFunction(insert_fn_name),
                {tuple_idx_table, idx_arr, num_tables.Get()});
}

void TupleIdxTable::ForEach(std::function<void(const khir::Value&)> handler) {
  auto tuple_idx_table = program_.LoadPtr(value_);

  auto size = proxy::Int32(
      program_,
      program_.Call(program_.GetFunction(size_fn_name), {tuple_idx_table}));

  auto tuple_it = program_.Alloca(program_.I8Type());
  program_.Call(program_.GetFunction(begin_fn_name),
                {tuple_idx_table, tuple_it});

  proxy::Loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < size;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        auto tuple_idx_arr =
            program_.Call(program_.GetFunction(get_fn_name), {tuple_it});

        handler(tuple_idx_arr);

        program_.Call(program_.GetFunction(increment_fn_name), {tuple_it});

        return loop.Continue(i + 1);
      });

  program_.Call(program_.GetFunction(free_it_fn_name), {tuple_it});
}

void TupleIdxTable::ForwardDeclare(khir::ProgramBuilder& program) {
  auto tuple_idx_table_type = program.PointerType(program.I8Type());
  program.DeclareExternalFunction(
      create_fn_name, tuple_idx_table_type, {},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Create));
  program.DeclareExternalFunction(
      insert_fn_name, program.VoidType(),
      {tuple_idx_table_type, program.PointerType(program.I32Type()),
       program.I32Type()},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Insert));
  program.DeclareExternalFunction(
      free_fn_name, program.VoidType(), {tuple_idx_table_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Free));
  program.DeclareExternalFunction(
      size_fn_name, program.I32Type(), {tuple_idx_table_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Size));

  auto tuple_idx_iterator_type = program.PointerType(program.I8Type());
  program.DeclareExternalFunction(
      begin_fn_name, program.VoidType(),
      {tuple_idx_table_type, tuple_idx_iterator_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Begin));
  program.DeclareExternalFunction(
      increment_fn_name, program.VoidType(), {tuple_idx_iterator_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Increment));
  program.DeclareExternalFunction(
      get_fn_name, program.PointerType(program.I32Type()),
      {tuple_idx_iterator_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Get));
  program.DeclareExternalFunction(
      free_it_fn_name, program.VoidType(), {tuple_idx_iterator_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::FreeIt));
}

}  // namespace kush::compile::proxy