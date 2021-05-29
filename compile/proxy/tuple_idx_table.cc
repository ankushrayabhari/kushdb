#include "compile/proxy/tuple_idx_table.h"

#include <functional>

#include "compile/khir/program_builder.h"
#include "compile/proxy/int.h"
#include "compile/proxy/loop.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

constexpr std::string_view create_fn_name(
    "_ZN4kush4data19CreateTupleIdxTableEv");
constexpr std::string_view insert_fn_name(
    "_ZN4kush4data6InsertEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
    "ESt8equal_toIS4_ESaIS4_EEPii");
constexpr std::string_view free_fn_name(
    "_ZN4kush4data4FreeEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
    "ESt8equal_toIS4_ESaIS4_EE");
constexpr std::string_view size_fn_name(
    "_ZN4kush4data4SizeEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
    "ESt8equal_toIS4_ESaIS4_EE");
constexpr std::string_view begin_fn_name(
    "_ZN4kush4data5BeginEPSt13unordered_setISt6vectorIiSaIiEESt4hashIS4_"
    "ESt8equal_toIS4_ESaIS4_EEPPNSt8__detail20_Node_const_iteratorIS4_"
    "Lb1ELb1EEE");
constexpr std::string_view increment_fn_name(
    "_ZN4kush4data9IncrementEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");
constexpr std::string_view get_fn_name(
    "_ZN4kush4data3GetEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");
constexpr std::string_view free_it_fn_name(
    "_ZN4kush4data4FreeEPPNSt8__detail20_Node_const_"
    "iteratorISt6vectorIiSaIiEELb1ELb1EEE");

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program)
    : program_(program),
      value_(program_.GlobalPointer(
          false, program_.PointerType(program_.I8Type()),
          program_.NullPtr(program_.PointerType(program_.I8Type())))()) {
  auto tuple_idx_table = program_.Call(program_.GetFunction(create_fn_name));
  program_.Store(value_, tuple_idx_table);
}

TupleIdxTable::~TupleIdxTable() {
  auto tuple_idx_table = program_.Load(value_);
  program_.Call(program_.GetFunction(free_fn_name), {tuple_idx_table});
}

void TupleIdxTable::Insert(const khir::Value& idx_arr, Int32& num_tables) {
  auto tuple_idx_table = program_.Load(value_);
  program_.Call(program_.GetFunction(insert_fn_name),
                {tuple_idx_table, idx_arr, num_tables.Get()});
}

void TupleIdxTable::ForEach(std::function<void(const khir::Value&)> handler) {
  auto tuple_idx_table = program_.Load(value_);

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
  program.DeclareExternalFunction(create_fn_name, tuple_idx_table_type, {});
  program.DeclareExternalFunction(
      insert_fn_name, program.VoidType(),
      {tuple_idx_table_type, program.PointerType(program.I32Type()),
       program.I32Type()});
  program.DeclareExternalFunction(free_fn_name, program.VoidType(),
                                  {tuple_idx_table_type});
  program.DeclareExternalFunction(size_fn_name, program.I32Type(),
                                  {tuple_idx_table_type});

  auto tuple_idx_iterator_type = program.PointerType(program.I8Type());
  program.DeclareExternalFunction(
      begin_fn_name, program.VoidType(),
      {tuple_idx_table_type, tuple_idx_iterator_type});
  program.DeclareExternalFunction(increment_fn_name, program.VoidType(),
                                  {tuple_idx_iterator_type});
  program.DeclareExternalFunction(get_fn_name,
                                  program.PointerType(program.I32Type()),
                                  {tuple_idx_iterator_type});
  program.DeclareExternalFunction(free_it_fn_name, program.VoidType(),
                                  {tuple_idx_iterator_type});
}

}  // namespace kush::compile::proxy