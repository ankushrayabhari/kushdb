#include "compile/khir/type_manager.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

namespace kush::khir {

TypeManager::TypeManager() {
  type_id_to_impl_.push_back(BaseTypeImpl::VOID);
  type_id_to_impl_.push_back(BaseTypeImpl::I1);
  type_id_to_impl_.push_back(BaseTypeImpl::I8);
  type_id_to_impl_.push_back(BaseTypeImpl::I16);
  type_id_to_impl_.push_back(BaseTypeImpl::I32);
  type_id_to_impl_.push_back(BaseTypeImpl::I64);
  type_id_to_impl_.push_back(BaseTypeImpl::F64);
}

int16_t TypeManager::VoidType() { return 0; }

int16_t TypeManager::I1Type() { return 1; }

int16_t TypeManager::I8Type() { return 2; }

int16_t TypeManager::I16Type() { return 3; }

int16_t TypeManager::I32Type() { return 4; }

int16_t TypeManager::I64Type() { return 5; }

int16_t TypeManager::F64Type() { return 6; }

int16_t TypeManager::NamedStructType(absl::Span<const int16_t> field_type_id,
                                     std::string_view name) {
  auto id = StructType(field_type_id);
  struct_name_to_type_id_[name] = id;
  return id;
}

int16_t TypeManager::StructType(absl::Span<const int16_t> field_type_id) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(StructTypeImpl(field_type_id));
  return size;
}

int16_t TypeManager::PointerType(int16_t type) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(PointerTypeImpl(type));
  return size;
}

int16_t TypeManager::ArrayType(int16_t type, int len) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(ArrayTypeImpl(type, len));
  return size;
}

int16_t TypeManager::FunctionType(int16_t result,
                                  absl::Span<const int16_t> args) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(FunctionTypeImpl(result, args));
  return size;
}

int16_t TypeManager::GetNamedStructType(std::string_view name) {
  return struct_name_to_type_id_.at(name);
}

TypeManager::PointerTypeImpl::PointerTypeImpl(int16_t element_type_id)
    : element_type_(element_type_id) {}

int16_t TypeManager::PointerTypeImpl::ElementType() { return element_type_; }

TypeManager::ArrayTypeImpl::ArrayTypeImpl(int16_t element_type_id, int length)
    : element_type_(element_type_id), length_(length) {}

int16_t TypeManager::ArrayTypeImpl::ElementType() { return element_type_; }

int TypeManager::ArrayTypeImpl::Length() { return length_; }

TypeManager::FunctionTypeImpl::FunctionTypeImpl(
    int16_t result_type_id, absl::Span<const int16_t> arg_type_id)
    : result_type_(result_type_id),
      arg_type_ids_(arg_type_id.begin(), arg_type_id.end()) {}

int16_t TypeManager::FunctionTypeImpl::ResultType() { return result_type_; }

absl::Span<const int16_t> TypeManager::FunctionTypeImpl::ArgTypes() {
  return arg_type_ids_;
}

TypeManager::StructTypeImpl::StructTypeImpl(
    absl::Span<const int16_t> field_type_ids)
    : field_type_ids_(field_type_ids.begin(), field_type_ids.end()) {}

absl::Span<const int16_t> TypeManager::StructTypeImpl::FieldTypes() {
  return field_type_ids_;
}

}  // namespace kush::khir
