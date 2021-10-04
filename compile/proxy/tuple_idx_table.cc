#include "compile/proxy/tuple_idx_table.h"

#include <functional>

#include "compile/proxy/loop.h"
#include "compile/proxy/value/value.h"
#include "khir/program_builder.h"
#include "runtime/tuple_idx_table.h"
#include "util/vector_util.h"

namespace kush::compile::proxy {

constexpr std::string_view tuple_idx_table_iterator_name(
    "kush::runtime::TupleIdxTable::Iterator");
constexpr std::string_view create_fn_name(
    "kush::runtime::TupleIdxTable::Create");
constexpr std::string_view insert_fn_name(
    "kush::runtime::TupleIdxTable::Insert");
constexpr std::string_view free_fn_name("kush::runtime::TupleIdxTable::Free");
constexpr std::string_view size_fn_name("kush::runtime::TupleIdxTable::Size");
constexpr std::string_view begin_fn_name("kush::runtime::TupleIdxTable::Begin");
constexpr std::string_view increment_fn_name(
    "kush::runtime::TupleIdxTable::Increment");
constexpr std::string_view get_fn_name("kush::runtime::TupleIdxTable::Get");
constexpr std::string_view free_it_fn_name(
    "kush::runtime::TupleIdxTable::FreeIt");

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program, bool global)
    : program_(program),
      value_(global ? program.Global(
                          false, true,
                          program.PointerType(program.GetOpaqueType(TypeName)),
                          program.NullPtr(program.PointerType(
                              program.GetOpaqueType(TypeName))))
                    : program.Alloca(program.PointerType(
                          program.GetOpaqueType(TypeName)))) {
  auto tuple_idx_table = program_.Call(program_.GetFunction(create_fn_name));
  program_.StorePtr(value_, tuple_idx_table);
}

TupleIdxTable::TupleIdxTable(khir::ProgramBuilder& program, khir::Value value)
    : program_(program),
      value_(program.Alloca(
          program.PointerType(program.GetOpaqueType(TypeName)))) {
  program_.StorePtr(value_, value);
}

void TupleIdxTable::Reset() {
  auto tuple_idx_table = program_.LoadPtr(value_);
  program_.Call(program_.GetFunction(free_fn_name), {tuple_idx_table});
}

void TupleIdxTable::Insert(const khir::Value& idx_arr,
                           const Int32& num_tables) {
  auto tuple_idx_table = program_.LoadPtr(value_);
  program_.Call(program_.GetFunction(insert_fn_name),
                {tuple_idx_table, idx_arr, num_tables.Get()});
}

khir::Value TupleIdxTable::Get() { return program_.LoadPtr(value_); }

proxy::Int32 TupleIdxTable::Size() {
  auto tuple_idx_table = program_.LoadPtr(value_);
  return proxy::Int32(
      program_,
      program_.Call(program_.GetFunction(size_fn_name), {tuple_idx_table}));
}

void TupleIdxTable::ForEach(std::function<void(const khir::Value&)> handler) {
  auto tuple_idx_table = program_.LoadPtr(value_);

  auto size = proxy::Int32(
      program_,
      program_.Call(program_.GetFunction(size_fn_name), {tuple_idx_table}));

  auto tuple_it = program_.Alloca(program_.PointerType(
      program_.GetOpaqueType(tuple_idx_table_iterator_name)));
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
  auto tuple_idx_table_type = program.OpaqueType(TypeName);
  auto tuple_idx_table_ptr_type = program.PointerType(tuple_idx_table_type);
  program.DeclareExternalFunction(
      create_fn_name, tuple_idx_table_ptr_type, {},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Create));
  program.DeclareExternalFunction(
      insert_fn_name, program.VoidType(),
      {tuple_idx_table_ptr_type, program.PointerType(program.I32Type()),
       program.I32Type()},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Insert));
  program.DeclareExternalFunction(
      free_fn_name, program.VoidType(), {tuple_idx_table_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Free));
  program.DeclareExternalFunction(
      size_fn_name, program.I32Type(), {tuple_idx_table_ptr_type},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Size));

  auto tuple_idx_iterator_type =
      program.OpaqueType(tuple_idx_table_iterator_name);
  auto tuple_idx_iterator_ptr_type =
      program.PointerType(tuple_idx_iterator_type);
  program.DeclareExternalFunction(
      begin_fn_name, program.VoidType(),
      {tuple_idx_table_ptr_type,
       program.PointerType(tuple_idx_iterator_ptr_type)},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Begin));
  program.DeclareExternalFunction(
      increment_fn_name, program.VoidType(),
      {program.PointerType(tuple_idx_iterator_ptr_type)},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Increment));
  program.DeclareExternalFunction(
      get_fn_name, program.PointerType(program.I32Type()),
      {program.PointerType(tuple_idx_iterator_ptr_type)},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::Get));
  program.DeclareExternalFunction(
      free_it_fn_name, program.VoidType(),
      {program.PointerType(tuple_idx_iterator_ptr_type)},
      reinterpret_cast<void*>(&runtime::TupleIdxTable::FreeIt));
}

}  // namespace kush::compile::proxy