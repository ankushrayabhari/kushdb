#include "compile/proxy/tuple_idx_table.h"

#include <functional>

#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/tuple_idx_table/tuple_idx_table.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

namespace {
constexpr std::string_view tuple_idx_table_iterator_name(
    "kush::runtime::TupleIdxTable::Iterator");
constexpr std::string_view create_it_fn_name(
    "kush::runtime::TupleIdxTable::CreateIt");
constexpr std::string_view free_it_fn_name(
    "kush::runtime::TupleIdxTable::FreeIt");
constexpr std::string_view get_fn_name(
    "kush::runtime::TupleIdxTable::Iterator::Get");
constexpr std::string_view create_fn_name(
    "kush::runtime::TupleIdxTable::Create");
constexpr std::string_view free_fn_name("kush::runtime::TupleIdxTable::Free");
constexpr std::string_view insert_fn_name(
    "kush::runtime::TupleIdxTable::TupleIdxTable::Insert");
constexpr std::string_view begin_fn_name(
    "kush::runtime::TupleIdxTable::TupleIdxTable::Begin");
constexpr std::string_view next_fn_name(
    "kush::runtime::TupleIdxTable::TupleIdxTable::IteratorNext");
}  // namespace

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program)
    : program_(program),
      value_(program.Global(
          program.PointerType(program.GetOpaqueType(TypeName)),
          program.NullPtr(
              program.PointerType(program.GetOpaqueType(TypeName))))) {
  auto tuple_idx_table = program_.Call(program_.GetFunction(create_fn_name));
  program_.StorePtr(value_, tuple_idx_table);
}

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program, khir::Value value)
    : program_(program),
      value_(program.Global(
          program.PointerType(program.GetOpaqueType(TypeName)), value)) {}

void TupleIdxTable::Reset() {
  auto tuple_idx_table = program_.LoadPtr(value_);
  program_.Call(program_.GetFunction(free_fn_name), {tuple_idx_table});
}

Bool TupleIdxTable::Insert(const khir::Value& idx_arr,
                           const Int32& num_tables) {
  auto tuple_idx_table = program_.LoadPtr(value_);
  return Bool(program_,
              program_.Call(program_.GetFunction(insert_fn_name),
                            {tuple_idx_table, idx_arr, num_tables.Get()}));
}

khir::Value TupleIdxTable::Get() { return program_.LoadPtr(value_); }

void TupleIdxTable::ForEach(std::function<void(const khir::Value&)> handler) {
  auto tuple_idx_table = program_.LoadPtr(value_);

  auto tuple_it = program_.Call(program_.GetFunction(create_it_fn_name));
  Bool has_next(program_, program_.Call(program_.GetFunction(begin_fn_name),
                                        {tuple_idx_table, tuple_it}));

  Loop(
      program_, [&](auto& loop) { loop.AddLoopVariable(has_next); },
      [&](auto& loop) { return loop.template GetLoopVariable<Bool>(0); },
      [&](auto& loop) {
        auto tuple_idx_arr =
            program_.Call(program_.GetFunction(get_fn_name), {tuple_it});

        handler(tuple_idx_arr);

        Bool has_next(program_,
                      program_.Call(program_.GetFunction(next_fn_name),
                                    {tuple_idx_table, tuple_it}));
        return loop.Continue(has_next);
      });

  program_.Call(program_.GetFunction(free_it_fn_name), {tuple_it});
}

void TupleIdxTable::ForwardDeclare(khir::ProgramBuilder& program) {
  auto tuple_idx_table_type = program.OpaqueType(TypeName);
  auto tuple_idx_table_ptr_type = program.PointerType(tuple_idx_table_type);
  program.DeclareExternalFunction(
      create_fn_name, tuple_idx_table_ptr_type, {},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Create));
  program.DeclareExternalFunction(
      free_fn_name, program.VoidType(), {tuple_idx_table_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Free));
  program.DeclareExternalFunction(
      insert_fn_name, program.I1Type(),
      {tuple_idx_table_ptr_type, program.PointerType(program.I32Type()),
       program.I32Type()},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Insert));

  auto tuple_idx_iterator_type =
      program.OpaqueType(tuple_idx_table_iterator_name);
  auto tuple_idx_iterator_ptr_type =
      program.PointerType(tuple_idx_iterator_type);
  program.DeclareExternalFunction(
      create_it_fn_name, tuple_idx_iterator_ptr_type, {},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::CreateIt));
  program.DeclareExternalFunction(
      free_it_fn_name, program.VoidType(), {tuple_idx_iterator_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::FreeIt));

  program.DeclareExternalFunction(
      begin_fn_name, program.I1Type(),
      {tuple_idx_table_ptr_type, tuple_idx_iterator_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Begin));
  program.DeclareExternalFunction(
      next_fn_name, program.I1Type(),
      {tuple_idx_table_ptr_type, tuple_idx_iterator_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::IteratorNext));
  program.DeclareExternalFunction(
      get_fn_name, program.PointerType(program.I32Type()),
      {tuple_idx_iterator_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Get));
}

}  // namespace kush::compile::proxy