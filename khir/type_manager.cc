#include "khir/type_manager.h"

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
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

namespace kush::khir {

bool TypeManager::initialized_ = false;

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

TypeManager::OpaqueTypeImpl::OpaqueTypeImpl(Type type, std::string_view name,
                                            llvm::Type* type_impl)
    : type_(type), name_(name), type_impl_(type_impl) {}

llvm::Type* TypeManager::OpaqueTypeImpl::GetLLVM() { return type_impl_; }

Type TypeManager::OpaqueTypeImpl::Get() { return type_; }

std::string_view TypeManager::OpaqueTypeImpl::Name() { return name_; }

TypeManager::TypeManager()
    : context_(std::make_unique<llvm::LLVMContext>()),
      module_(std::make_unique<llvm::Module>("type_manager", *context_)),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context_)) {
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module_->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!target) {
    throw std::runtime_error("Target not found: " + error);
  }

  auto cpu = "x86-64";
  auto features = "";

  llvm::TargetOptions opt;
  auto reloc_model =
      llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::PIC_);
  auto target_machine = target->createTargetMachine(target_triple, cpu,
                                                    features, opt, reloc_model);

  module_->setDataLayout(target_machine->createDataLayout());

  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::VOID, VoidType(),
                                         builder_->getVoidTy()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::I1, I1Type(),
                                         builder_->getInt1Ty()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::I8, I8Type(),
                                         builder_->getInt8Ty()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::I16, I16Type(),
                                         builder_->getInt16Ty()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::I32, I32Type(),
                                         builder_->getInt32Ty()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::I64, I64Type(),
                                         builder_->getInt64Ty()));
  AddType(std::make_unique<BaseTypeImpl>(BaseTypeId::F64, F64Type(),
                                         builder_->getDoubleTy()));
  AddType(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I32_VEC_8, I32Vec8Type(),
      llvm::FixedVectorType::get(builder_->getInt32Ty(), 8)));
  AddType(std::make_unique<BaseTypeImpl>(
      BaseTypeId::I1_VEC_8, I1Vec8Type(),
      llvm::FixedVectorType::get(builder_->getInt1Ty(), 8)));
  AddType(std::make_unique<PointerTypeImpl>(I8Type(), I8PtrType(),
                                            builder_->getInt8PtrTy()));
  AddType(std::make_unique<PointerTypeImpl>(
      I32Vec8Type(), I32Vec8PtrType(),
      llvm::PointerType::get(
          llvm::FixedVectorType::get(builder_->getInt1Ty(), 8), 0)));
}

Type TypeManager::GetOutputType() {
  return static_cast<Type>(type_id_to_impl_.size());
}

Type TypeManager::AddType(std::unique_ptr<TypeImpl> mp) {
  auto llvm_ty = mp->GetLLVM();
  if (impl_to_type_id_.contains(llvm_ty)) {
    throw std::runtime_error("Check the map first!");
  }

  auto output = GetOutputType();
  impl_to_type_id_[llvm_ty] = output;
  type_id_to_impl_.push_back(std::move(mp));
  return output;
}

Type TypeManager::VoidType() const { return static_cast<Type>(0); }
bool TypeManager::IsVoid(Type t) const { return t.GetID() == 0; }

Type TypeManager::I1Type() const { return static_cast<Type>(1); }
bool TypeManager::IsI1Type(Type t) const { return t.GetID() == 1; }

Type TypeManager::I8Type() const { return static_cast<Type>(2); }
bool TypeManager::IsI8Type(Type t) const { return t.GetID() == 2; }

Type TypeManager::I16Type() const { return static_cast<Type>(3); }
bool TypeManager::IsI16Type(Type t) const { return t.GetID() == 3; }

Type TypeManager::I32Type() const { return static_cast<Type>(4); }
bool TypeManager::IsI32Type(Type t) const { return t.GetID() == 4; }

Type TypeManager::I64Type() const { return static_cast<Type>(5); }
bool TypeManager::IsI64Type(Type t) const { return t.GetID() == 5; }

Type TypeManager::F64Type() const { return static_cast<Type>(6); }
bool TypeManager::IsF64Type(Type t) const { return t.GetID() == 6; }

Type TypeManager::I32Vec8Type() const { return static_cast<Type>(7); }
bool TypeManager::IsI32Vec8Type(Type t) const { return t.GetID() == 7; }

Type TypeManager::I1Vec8Type() const { return static_cast<Type>(8); }
bool TypeManager::IsI1Vec8Type(Type t) const { return t.GetID() == 8; }

Type TypeManager::I8PtrType() const { return static_cast<Type>(9); }

Type TypeManager::I32Vec8PtrType() const { return static_cast<Type>(10); }

Type TypeManager::OpaqueType(std::string_view name) {
  if (opaque_name_to_type_id_.contains(name)) {
    throw std::runtime_error(std::string(name) + ": name exists!");
  }

  auto impl = llvm::StructType::create(*context_, name);
  if (impl_to_type_id_.contains(impl)) {
    return impl_to_type_id_[impl];
  }

  auto output =
      AddType(std::make_unique<OpaqueTypeImpl>(GetOutputType(), name, impl));
  opaque_name_to_type_id_[name] = output;
  return output;
}

Type TypeManager::NamedStructType(absl::Span<const Type> field_type_id,
                                  std::string_view name) {
  if (struct_name_to_type_id_.contains(name)) {
    throw std::runtime_error(std::string(name) + ": name exists!");
  }

  auto id = StructType(field_type_id);
  struct_name_to_type_id_[name] = id;
  return id;
}

Type TypeManager::StructType(absl::Span<const Type> field_type_id) {
  auto impl = llvm::StructType::create(
      *context_, GetTypeArray(type_id_to_impl_, field_type_id));
  if (impl_to_type_id_.contains(impl)) {
    return impl_to_type_id_[impl];
  }
  return AddType(
      std::make_unique<StructTypeImpl>(field_type_id, GetOutputType(), impl));
}

Type TypeManager::PointerType(Type elem) {
  auto impl =
      llvm::PointerType::get(type_id_to_impl_[elem.GetID()]->GetLLVM(), 0);
  if (impl_to_type_id_.contains(impl)) {
    return impl_to_type_id_[impl];
  }
  return AddType(
      std::make_unique<PointerTypeImpl>(elem, GetOutputType(), impl));
}

Type TypeManager::ArrayType(Type type, int len) {
  auto impl =
      llvm::ArrayType::get(type_id_to_impl_[type.GetID()]->GetLLVM(), len);
  if (impl_to_type_id_.contains(impl)) {
    return impl_to_type_id_[impl];
  }
  return AddType(
      std::make_unique<ArrayTypeImpl>(type, len, GetOutputType(), impl));
}

Type TypeManager::FunctionType(Type result, absl::Span<const Type> args) {
  auto impl =
      llvm::FunctionType::get(type_id_to_impl_[result.GetID()]->GetLLVM(),
                              GetTypeArray(type_id_to_impl_, args), false);
  if (impl_to_type_id_.contains(impl)) {
    return impl_to_type_id_[impl];
  }
  return AddType(
      std::make_unique<FunctionTypeImpl>(result, args, GetOutputType(), impl));
}

Type TypeManager::GetOpaqueType(std::string_view name) {
  return opaque_name_to_type_id_.at(name);
}

Type TypeManager::GetNamedStructType(std::string_view name) {
  return struct_name_to_type_id_.at(name);
}

Type TypeManager::GetFunctionReturnType(Type func_type) const {
  return dynamic_cast<FunctionTypeImpl&>(*type_id_to_impl_[func_type.GetID()])
      .ResultType();
}

Type TypeManager::GetPointerElementType(Type ptr_type) const {
  return dynamic_cast<PointerTypeImpl&>(*type_id_to_impl_[ptr_type.GetID()])
      .ElementType();
}

Type TypeManager::GetArrayElementType(Type ptr_type) const {
  return dynamic_cast<ArrayTypeImpl&>(*type_id_to_impl_[ptr_type.GetID()])
      .ElementType();
}

absl::Span<const Type> TypeManager::GetStructElementTypes(Type ptr_type) const {
  return dynamic_cast<StructTypeImpl&>(*type_id_to_impl_[ptr_type.GetID()])
      .ElementTypes();
}

std::vector<int32_t> TypeManager::GetStructFieldOffsets(Type t) const {
  auto st =
      llvm::dyn_cast<llvm::StructType>(type_id_to_impl_[t.GetID()]->GetLLVM());
  auto layout = module_->getDataLayout().getStructLayout(st);

  std::vector<int32_t> offsets;
  for (int i = 0; i < st->getNumElements(); i++) {
    offsets.push_back(layout->getElementOffset(i));
  }
  return offsets;
}

int32_t TypeManager::GetTypeSize(Type t) const {
  return module_->getDataLayout().getTypeAllocSize(
      type_id_to_impl_[t.GetID()]->GetLLVM());
}

std::pair<int32_t, Type> TypeManager::GetPointerOffset(
    Type t, absl::Span<const int32_t> idx, bool dynamic) {
  int i;
  int32_t offset;

  if (dynamic) {
    i = 0;
    offset = 0;
  } else {
    i = 1;
    offset = idx[0] * GetTypeSize(t);
  }

  TypeImpl* result_type = type_id_to_impl_[t.GetID()].get();
  for (; i < idx.size(); i++) {
    if (auto ptr_type = dynamic_cast<PointerTypeImpl*>(result_type)) {
      // nothing changes
      result_type = type_id_to_impl_[ptr_type->ElementType().GetID()].get();
      offset += idx[i] * GetTypeSize(ptr_type->ElementType());
    } else if (auto array_type = dynamic_cast<ArrayTypeImpl*>(result_type)) {
      result_type = type_id_to_impl_[array_type->ElementType().GetID()].get();
      offset += idx[i] * GetTypeSize(array_type->ElementType());
    } else if (auto struct_type = dynamic_cast<StructTypeImpl*>(result_type)) {
      result_type =
          type_id_to_impl_[struct_type->ElementTypes()[idx[i]].GetID()].get();
      auto field_offsets = GetStructFieldOffsets(struct_type->Get());
      offset += field_offsets[idx[i]];
    } else {
      throw std::runtime_error("Cannot index into type.");
    }
  }

  return {offset, PointerType(result_type->Get())};
}

void TypeManager::Translate(TypeTranslator& translator) const {
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
        case I32_VEC_8:
          translator.TranslateI32Vec8Type();
          break;
        case I1_VEC_8:
          translator.TranslateI1Vec8Type();
          break;
      }
    } else if (auto opaque_type = dynamic_cast<OpaqueTypeImpl*>(type_impl)) {
      translator.TranslateOpaqueType(opaque_type->Name());
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

bool TypeManager::IsPtrType(Type t) const {
  return dynamic_cast<PointerTypeImpl*>(type_id_to_impl_[t.GetID()].get()) !=
         nullptr;
}

bool TypeManager::IsArrayType(Type t) const {
  return dynamic_cast<ArrayTypeImpl*>(type_id_to_impl_[t.GetID()].get()) !=
         nullptr;
}

bool TypeManager::IsStructType(Type t) const {
  return dynamic_cast<StructTypeImpl*>(type_id_to_impl_[t.GetID()].get()) !=
         nullptr;
}

bool TypeManager::IsFuncType(Type t) const {
  return dynamic_cast<FunctionTypeImpl*>(type_id_to_impl_[t.GetID()].get()) !=
         nullptr;
}

}  // namespace kush::khir
