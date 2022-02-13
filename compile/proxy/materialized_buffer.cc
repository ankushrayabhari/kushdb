#include "compile/proxy/materialized_buffer.h"

#include <memory>
#include <vector>

#include "compile/proxy/column_data.h"
#include "compile/proxy/control_flow/loop.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "khir/program_builder.h"

namespace kush::compile::proxy {

DiskMaterializedBuffer::DiskMaterializedBuffer(
    khir::ProgramBuilder& program,
    std::vector<std::unique_ptr<Iterable>> column_data,
    std::vector<std::unique_ptr<Iterable>> null_data)
    : program_(program),
      column_data_(std::move(column_data)),
      null_data_(std::move(null_data)) {
  assert(column_data_.size() == null_data_.size());
}

void DiskMaterializedBuffer::Init() {
  for (auto& column : column_data_) {
    column->Init();
  }

  for (auto& null_column : null_data_) {
    if (null_column != nullptr) {
      null_column->Init();
    }
  }
}

void DiskMaterializedBuffer::Reset() {
  for (auto& column : column_data_) {
    column->Reset();
  }

  for (auto& null_column : null_data_) {
    if (null_column != nullptr) {
      null_column->Reset();
    }
  }
}

Int32 DiskMaterializedBuffer::Size() { return column_data_[0]->Size(); }

std::vector<SQLValue> DiskMaterializedBuffer::operator[](Int32 i) {
  std::vector<SQLValue> output;
  for (int col_idx = 0; col_idx < column_data_.size(); col_idx++) {
    auto& column = column_data_[col_idx];
    auto& null_column = null_data_[col_idx];

    Bool null_value(program_, false);
    if (null_column != nullptr) {
      null_value = static_cast<Bool&>(*(*null_column)[i]);
    }

    output.emplace_back((*column)[i], column->Type(), null_value);
  }
  return output;
}

SQLValue DiskMaterializedBuffer::Get(Int32 i, int col_idx) {
  auto& column = column_data_[col_idx];
  auto& null_column = null_data_[col_idx];

  Bool null_value(program_, false);
  if (null_column != nullptr) {
    null_value = static_cast<Bool&>(*(*null_column)[i]);
  }

  return SQLValue((*column)[i], column->Type(), null_value);
}

void DiskMaterializedBuffer::Scan(
    int col_idx, std::function<void(Int32, SQLValue)> handler) {
  auto cardinality = Size();
  auto& column = *column_data_[col_idx];
  auto& null_column = null_data_[col_idx];
  Loop loop(
      program_,
      [&](auto& loop) { loop.AddLoopVariable(proxy::Int32(program_, 0)); },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);
        return i < cardinality;
      },
      [&](auto& loop) {
        auto i = loop.template GetLoopVariable<proxy::Int32>(0);

        Bool null_value(program_, false);
        if (null_column != nullptr) {
          null_value = static_cast<Bool&>(*(*null_column)[i]);
        }

        handler(i, SQLValue(column[i], column.Type(), null_value));

        return loop.Continue(i + 1);
      });
}

khir::Value DiskMaterializedBuffer::Serialize() {
  auto i8_ptr_ty = program_.PointerType(program_.I8Type());

  auto value = program_.Alloca(i8_ptr_ty, 2 * column_data_.size());

  for (int i = 0; i < column_data_.size(); i++) {
    program_.StorePtr(program_.ConstGEP(i8_ptr_ty, value, {2 * i}),
                      program_.PointerCast(column_data_[i]->Get(), i8_ptr_ty));

    if (null_data_[i] != nullptr) {
      program_.StorePtr(program_.ConstGEP(i8_ptr_ty, value, {2 * i + 1}),
                        program_.PointerCast(null_data_[i]->Get(), i8_ptr_ty));
    }
  }

  return program_.PointerCast(value, i8_ptr_ty);
}

std::unique_ptr<MaterializedBuffer> DiskMaterializedBuffer::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  std::vector<std::unique_ptr<Iterable>> column_data;
  std::vector<std::unique_ptr<Iterable>> null_data;

  void** values = static_cast<void**>(value);
  for (int i = 0; i < column_data_.size(); i++) {
    void* col_data_val = values[2 * i];

    column_data.push_back(
        column_data_[i]->Regenerate(program, program.ConstPtr(col_data_val)));
    if (null_data_[i] == nullptr) {
      null_data.push_back(nullptr);
    } else {
      void* null_data_val = values[2 * i + 1];
      null_data.push_back(
          null_data_[i]->Regenerate(program, program.ConstPtr(null_data_val)));
    }
  }

  return std::make_unique<DiskMaterializedBuffer>(
      program, std::move(column_data), std::move(null_data));
}

MemoryMaterializedBuffer::MemoryMaterializedBuffer(
    khir::ProgramBuilder& program,
    std::unique_ptr<StructBuilder> vector_content, Vector vector)
    : program_(program),
      vector_content_(std::move(vector_content)),
      vector_(std::move(vector)) {}

void MemoryMaterializedBuffer::Init() {
  // vector already built so do nothing
}

void MemoryMaterializedBuffer::Reset() { vector_.Reset(); }

Int32 MemoryMaterializedBuffer::Size() { return vector_.Size(); }

std::vector<SQLValue> MemoryMaterializedBuffer::operator[](Int32 i) {
  return vector_[i].Unpack();
}

SQLValue MemoryMaterializedBuffer::Get(Int32 i, int col) {
  return vector_[i].Unpack()[col];
}

khir::Value MemoryMaterializedBuffer::Serialize() {
  return program_.PointerCast(vector_.Get(),
                              program_.PointerType(program_.I8Type()));
}

std::unique_ptr<MaterializedBuffer> MemoryMaterializedBuffer::Regenerate(
    khir::ProgramBuilder& program, void* value) {
  // regenerate the struct type
  auto types = vector_content_->Types();
  const auto& nullables = vector_content_->Nullable();
  auto struct_builder = std::make_unique<proxy::StructBuilder>(program);
  for (int i = 0; i < types.size(); i++) {
    struct_builder->Add(types[i], nullables[i]);
  }
  struct_builder->Build();

  proxy::Vector vector(
      program, *struct_builder,
      program.PointerCast(program.ConstPtr(value),
                          program.PointerType(program.GetStructType(
                              proxy::Vector::VectorStructName))));

  return std::make_unique<MemoryMaterializedBuffer>(
      program, std::move(struct_builder), std::move(vector));
}

}  // namespace kush::compile::proxy