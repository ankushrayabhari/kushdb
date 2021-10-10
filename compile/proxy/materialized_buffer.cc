#include "compile/proxy/materialized_buffer.h"

#include <memory>
#include <vector>

#include "compile/proxy/column_data.h"
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
    column->Init();
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

khir::Value DiskMaterializedBuffer::Serialize() {
  auto i8_ptr_ty = program_.PointerType(program_.I8Type());

  std::vector<khir::Type> serialization_fields_;
  for (int i = 0; i < column_data_.size(); i++) {
    serialization_fields_.push_back(i8_ptr_ty);
    serialization_fields_.push_back(i8_ptr_ty);
  }
  auto serialization_type = program_.StructType(serialization_fields_);
  auto value = program_.Alloca(serialization_type);

  for (int i = 0, j = 0; i < serialization_fields_.size(); i += 2, j++) {
    program_.StorePtr(program_.GetElementPtr(serialization_type, value, {0, i}),
                      program_.PointerCast(column_data_[j]->Get(), i8_ptr_ty));

    auto null_val = null_data_[j] == nullptr
                        ? program_.NullPtr(i8_ptr_ty)
                        : program_.PointerCast(null_data_[j]->Get(), i8_ptr_ty);

    program_.StorePtr(
        program_.GetElementPtr(serialization_type, value, {0, i + 1}),
        null_val);
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
    void* null_data_val = values[2 * i + 1];

    column_data.push_back(
        column_data_[i]->Regenerate(program, program.ConstPtr(col_data_val)));
    if (null_data_[i] == nullptr) {
      null_data.push_back(nullptr);
    } else {
      null_data.push_back(
          null_data_[i]->Regenerate(program, program.ConstPtr(null_data_val)));
    }
  }

  return std::make_unique<DiskMaterializedBuffer>(
      program_, std::move(column_data_), std::move(null_data_));
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