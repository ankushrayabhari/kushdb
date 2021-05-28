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

uint16_t Type::GetID() const { return static_cast<uint16_t>(*this); }

std::vector<llvm::Type*> TypeManager::GetTypeArray(
    std::vector<std::unique_ptr<TypeManager::TypeImpl>>& types,
    absl::Span<const Type> field_type_id) {
  std::vector<llvm::Type*> result;
  for (auto x : field_type_id) {
    result.push_back(types[x.GetID()]->GetLLVM());
  }
  return result;
}

TypeManager::BaseTypeImpl::BaseTypeImpl(BaseTypeId id, Type type,
                                        llvm::Type* type_impl)
    : id_(id), type_(type), type_impl_(type_impl) {}

llvm::Type* TypeManager::BaseTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::BaseTypeImpl::Get() { return type_; }

TypeManager::BaseTypeId TypeManager::BaseTypeImpl::TypeId() { return id_; }

TypeManager::PointerTypeImpl::PointerTypeImpl(Type elem, Type type,
                                              llvm::Type* type_impl)
    : elem_(elem), type_(type), type_impl_(type_impl) {}

llvm::Type* TypeManager::PointerTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::PointerTypeImpl::Get() { return type_; }

Type TypeManager::PointerTypeImpl::ElementType() { return elem_; }

TypeManager::ArrayTypeImpl::ArrayTypeImpl(Type elem, int len, Type type,
                                          llvm::Type* type_impl)
    : elem_(elem), len_(len), type_(type), type_impl_(type_impl) {}

llvm::Type* TypeManager::ArrayTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::ArrayTypeImpl::Get() { return type_; }

Type TypeManager::ArrayTypeImpl::ElementType() { return elem_; }

int TypeManager::ArrayTypeImpl::Length() { return len_; }

TypeManager::FunctionTypeImpl::FunctionTypeImpl(
    Type result, absl::Span<const Type> arg_types, Type type,
    llvm::Type* type_impl)
    : result_type_(result),
      arg_types_(arg_types.begin(), arg_types.end()),
      type_(type),
      type_impl_(type_impl) {}

llvm::Type* TypeManager::FunctionTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::FunctionTypeImpl::Get() { return type_; }

Type TypeManager::FunctionTypeImpl::ResultType() { return result_type_; }

absl::Span<const Type> TypeManager::FunctionTypeImpl::ArgTypes() {
  return arg_types_;
}

TypeManager::StructTypeImpl::StructTypeImpl(
    absl::Span<const Type> element_types, Type type, llvm::Type* type_impl)
    : elem_types_(element_types.begin(), element_types.end()),
      type_(type),
      type_impl_(type_impl) {}

llvm::Type* TypeManager::StructTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::StructTypeImpl::Get() { return type_; }

absl::Span<const Type> TypeManager::StructTypeImpl::ElementTypes() {
  return elem_types_;
}

TypeManager::TypeManager()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("type_manager", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::VOID, static_cast<Type>(0), builder_->getVoidTy()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I1, static_cast<Type>(1), builder_->getInt1Ty()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I8, static_cast<Type>(2), builder_->getInt8Ty()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I16, static_cast<Type>(3), builder_->getInt16Ty()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I32, static_cast<Type>(4), builder_->getInt32Ty()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I64, static_cast<Type>(5), builder_->getInt64Ty()));
  type_id_to_impl_.push_back(std::make_unique<BaseTypeImpl>(
      BaseTypeId::F64, static_cast<Type>(6), builder_->getDoubleTy()));
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
  auto output = static_cast<Type>(type_id_to_impl_.size());

  auto impl = llvm::StructType::create(
      *context_, GetTypeArray(type_id_to_impl_, field_type_id));
  type_id_to_impl_.push_back(
      std::make_unique<StructTypeImpl>(field_type_id, output, impl));
  return output;
}

Type TypeManager::PointerType(Type elem) {
  auto output = static_cast<Type>(type_id_to_impl_.size());
  auto impl =
      llvm::PointerType::get(type_id_to_impl_[elem.GetID()]->GetLLVM(), 0);
  type_id_to_impl_.push_back(
      std::make_unique<PointerTypeImpl>(elem, output, impl));
  return output;
}

Type TypeManager::ArrayType(Type type, int len) {
  auto output = static_cast<Type>(type_id_to_impl_.size());
  auto impl =
      llvm::ArrayType::get(type_id_to_impl_[type.GetID()]->GetLLVM(), len);
  type_id_to_impl_.push_back(
      std::make_unique<ArrayTypeImpl>(type, len, output, impl));
  return output;
}

Type TypeManager::FunctionType(Type result, absl::Span<const Type> args) {
  auto output = static_cast<Type>(type_id_to_impl_.size());
  auto impl =
      llvm::FunctionType::get(type_id_to_impl_[result.GetID()]->GetLLVM(),
                              GetTypeArray(type_id_to_impl_, args), false);
  type_id_to_impl_.push_back(
      std::make_unique<FunctionTypeImpl>(result, args, output, impl));
  return output;
}

Type TypeManager::GetNamedStructType(std::string_view name) {
  return struct_name_to_type_id_.at(name);
}

Type TypeManager::GetFunctionReturnType(Type func_type) {
  return dynamic_cast<FunctionTypeImpl&>(*type_id_to_impl_[func_type.GetID()])
      .ResultType();
}

Type TypeManager::GetPointerElementType(Type ptr_type) {
  return dynamic_cast<PointerTypeImpl&>(*type_id_to_impl_[ptr_type.GetID()])
      .ElementType();
}

std::pair<int64_t, Type> TypeManager::GetPointerOffset(
    Type t, absl::Span<const int32_t> idx) {
  std::vector<llvm::Value*> values;
  for (int32_t i : idx) {
    values.push_back(builder_->getInt32(i));
  }
  llvm::Type* target = type_id_to_impl_[t.GetID()]->GetLLVM();
  int64_t offset =
      module_->getDataLayout().getIndexedOffsetInType(target, values);

  TypeImpl* result_type = type_id_to_impl_[t.GetID()].get();
  for (int i = 1; i < idx.size(); i++) {
    if (auto ptr_type = dynamic_cast<PointerTypeImpl*>(result_type)) {
      // nothing changes
      result_type = type_id_to_impl_[ptr_type->ElementType().GetID()].get();
    } else if (auto array_type = dynamic_cast<ArrayTypeImpl*>(result_type)) {
      result_type = type_id_to_impl_[array_type->ElementType().GetID()].get();
    } else if (auto struct_type = dynamic_cast<StructTypeImpl*>(result_type)) {
      result_type =
          type_id_to_impl_[struct_type->ElementTypes()[idx[i]].GetID()].get();
    } else {
      throw std::runtime_error("Cannot index into type.");
    }
  }

  return {offset, PointerType(result_type->Get())};
}

void TypeManager::Translate(TypeTranslator& translator) {
  for (int i = 0; i < type_id_to_impl_.size(); i++) {
    auto type_impl = type_id_to_impl_[i].get();
    if (auto base_type = dynamic_cast<BaseTypeImpl*>(type_impl)) {
      switch (base_type->TypeId()) {
        case VOID:
          translator.TranslateVoidType();
          break;
        case I1:
          translator.TranslateI1Type();
          break;
        case I8:
          translator.TranslateI8Type();
          break;
        case I16:
          translator.TranslateI16Type();
          break;
        case I32:
          translator.TranslateI32Type();
          break;
        case I64:
          translator.TranslateI64Type();
          break;
        case F64:
          translator.TranslateF64Type();
          break;
      }
    } else if (auto ptr_type = dynamic_cast<PointerTypeImpl*>(type_impl)) {
      translator.TranslatePointerType(ptr_type->ElementType());
    } else if (auto array_type = dynamic_cast<ArrayTypeImpl*>(type_impl)) {
      translator.TranslateArrayType(array_type->ElementType(),
                                    array_type->Length());
    } else if (auto struct_type = dynamic_cast<StructTypeImpl*>(type_impl)) {
      translator.TranslateStructType(struct_type->ElementTypes());
    } else if (auto function_type =
                   dynamic_cast<FunctionTypeImpl*>(type_impl)) {
      translator.TranslateFunctionType(function_type->ResultType(),
                                       function_type->ArgTypes());
    } else {
      throw std::runtime_error("Unreachable.");
    }
  }
}

}  // namespace kush::khir
