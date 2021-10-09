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

MemoryMaterializedBuffer::MemoryMaterializedBuffer(
    khir::ProgramBuilder& program, Vector vector)
    : program_(program), vector_(std::move(vector)) {}

void MemoryMaterializedBuffer::Init() {
  // vector already built so do nothing
}

void MemoryMaterializedBuffer::Reset() { vector_.Reset(); }

Int32 MemoryMaterializedBuffer::Size() { return vector_.Size(); }

std::vector<SQLValue> MemoryMaterializedBuffer::operator[](Int32 i) {
  return vector_[i].Unpack();
}

}  // namespace kush::compile::proxy