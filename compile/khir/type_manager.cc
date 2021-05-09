#include "compile/khir/type_manager.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

namespace kush::khir {

uint16_t Type::GetID() { return static_cast<uint16_t>(*this); }

TypeManager::TypeManager() {
  type_id_to_impl_.push_back(BaseTypeImpl::VOID);
  type_id_to_impl_.push_back(BaseTypeImpl::I1);
  type_id_to_impl_.push_back(BaseTypeImpl::I8);
  type_id_to_impl_.push_back(BaseTypeImpl::I16);
  type_id_to_impl_.push_back(BaseTypeImpl::I32);
  type_id_to_impl_.push_back(BaseTypeImpl::I64);
  type_id_to_impl_.push_back(BaseTypeImpl::F64);
}

Type TypeManager::VoidType() { return static_cast<Type>(0); }

Type TypeManager::I1Type() { return static_cast<Type>(1); }

Type TypeManager::I8Type() { return static_cast<Type>(2); }

Type TypeManager::I16Type() { return static_cast<Type>(3); }

Type TypeManager::I32Type() { return static_cast<Type>(4); }

Type TypeManager::I64Type() { return static_cast<Type>(5); }

Type TypeManager::F64Type() { return static_cast<Type>(6); }

Type TypeManager::NamedStructType(absl::Span<const Type> field_type_id,
                                  std::string_view name) {
  auto id = StructType(field_type_id);
  struct_name_to_type_id_[name] = id;
  return id;
}

Type TypeManager::StructType(absl::Span<const Type> field_type_id) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(StructTypeImpl(field_type_id));
  return static_cast<Type>(size);
}

Type TypeManager::PointerType(Type type) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(PointerTypeImpl(type));
  return static_cast<Type>(size);
}

Type TypeManager::ArrayType(Type type, int len) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(ArrayTypeImpl(type, len));
  return static_cast<Type>(size);
}

Type TypeManager::FunctionType(Type result, absl::Span<const Type> args) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(FunctionTypeImpl(result, args));
  return static_cast<Type>(size);
}

Type TypeManager::GetNamedStructType(std::string_view name) {
  return struct_name_to_type_id_.at(name);
}

TypeManager::PointerTypeImpl::PointerTypeImpl(Type element_type_id)
    : element_type_(element_type_id) {}

Type TypeManager::PointerTypeImpl::ElementType() { return element_type_; }

TypeManager::ArrayTypeImpl::ArrayTypeImpl(Type element_type_id, int length)
    : element_type_(element_type_id), length_(length) {}

Type TypeManager::ArrayTypeImpl::ElementType() { return element_type_; }

int TypeManager::ArrayTypeImpl::Length() { return length_; }

TypeManager::FunctionTypeImpl::FunctionTypeImpl(
    Type result_type_id, absl::Span<const Type> arg_type_id)
    : result_type_(result_type_id),
      arg_type_ids_(arg_type_id.begin(), arg_type_id.end()) {}

Type TypeManager::FunctionTypeImpl::ResultType() { return result_type_; }

absl::Span<const Type> TypeManager::FunctionTypeImpl::ArgTypes() {
  return arg_type_ids_;
}

TypeManager::StructTypeImpl::StructTypeImpl(
    absl::Span<const Type> field_type_ids)
    : field_type_ids_(field_type_ids.begin(), field_type_ids.end()) {}

absl::Span<const Type> TypeManager::StructTypeImpl::FieldTypes() {
  return field_type_ids_;
}

}  // namespace kush::khir
