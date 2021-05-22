#include "compile/khir/type_manager.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"

#include "type_safe/strong_typedef.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

namespace kush::khir {

std::vector<llvm::Type*> GetTypeArray(std::vector<llvm::Type*>& types,
                                      absl::Span<const Type> field_type_id) {
  std::vector<llvm::Type*> result;
  for (auto x : field_type_id) {
    result.push_back(types[x.GetID()]);
  }
  return result;
}

uint16_t Type::GetID() const { return static_cast<uint16_t>(*this); }

TypeManager::TypeManager()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("type_manager", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  type_id_to_impl_.push_back(builder_->getVoidTy());
  type_id_to_impl_.push_back(builder_->getInt1Ty());
  type_id_to_impl_.push_back(builder_->getInt8Ty());
  type_id_to_impl_.push_back(builder_->getInt16Ty());
  type_id_to_impl_.push_back(builder_->getInt32Ty());
  type_id_to_impl_.push_back(builder_->getInt64Ty());
  type_id_to_impl_.push_back(builder_->getDoubleTy());
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
  type_id_to_impl_.push_back(llvm::StructType::create(
      *context_, GetTypeArray(type_id_to_impl_, field_type_id)));
  return static_cast<Type>(size);
}

Type TypeManager::PointerType(Type type) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::PointerType::get(type_id_to_impl_[type.GetID()], 0));
  return static_cast<Type>(size);
}

Type TypeManager::ArrayType(Type type, int len) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::ArrayType::get(type_id_to_impl_[type.GetID()], len));
  return static_cast<Type>(size);
}

Type TypeManager::FunctionType(Type result, absl::Span<const Type> args) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::FunctionType::get(type_id_to_impl_[result.GetID()],
                              GetTypeArray(type_id_to_impl_, args), false));
  return static_cast<Type>(size);
}

Type TypeManager::GetNamedStructType(std::string_view name) {
  return struct_name_to_type_id_.at(name);
}

Type TypeManager::GetFunctionReturnType(Type func_type) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::dyn_cast<llvm::FunctionType>(type_id_to_impl_[func_type.GetID()])
          ->getReturnType());
  return static_cast<Type>(size);
}

Type TypeManager::GetPointerElementType(Type ptr_type) {
  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::dyn_cast<llvm::PointerType>(type_id_to_impl_[ptr_type.GetID()])
          ->getElementType());
  return static_cast<Type>(size);
}

std::pair<int64_t, Type> TypeManager::GetPointerOffset(
    Type t, absl::Span<const int32_t> idx) {
  std::vector<llvm::Value*> values;
  for (int32_t i : idx) {
    values.push_back(builder_->getInt32(i));
  }
  llvm::Type* target = type_id_to_impl_[t.GetID()];

  int64_t offset =
      module_->getDataLayout().getIndexedOffsetInType(target, values);

  auto size = type_id_to_impl_.size();
  type_id_to_impl_.push_back(
      llvm::GetElementPtrInst::getIndexedType(target, values));
  auto result_type = static_cast<Type>(size);

  return {offset, result_type};
}

}  // namespace kush::khir
