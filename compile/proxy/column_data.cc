#include "compile/proxy/column_data.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "execution/query_state.h"
#include "khir/program_builder.h"
#include "runtime/column_data.h"

namespace kush::compile::proxy {

template <catalog::TypeId S>
std::string_view OpenFnName() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return "kush::runtime::ColumnData::OpenInt16";
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return "kush::runtime::ColumnData::OpenInt32";
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return "kush::runtime::ColumnData::OpenInt64";
  } else if constexpr (catalog::TypeId::REAL == S) {
    return "kush::runtime::ColumnData::OpenFloat64";
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return "kush::runtime::ColumnData::OpenInt8";
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return "kush::runtime::ColumnData::OpenText";
  }
}

template <catalog::TypeId S>
void* OpenFn() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt16);
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt32);
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt64);
  } else if constexpr (catalog::TypeId::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenFloat64);
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt8);
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenText);
  }
}

template <catalog::TypeId S>
std::string_view CloseFnName() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return "kush::runtime::ColumnData::CloseInt16";
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return "kush::runtime::ColumnData::CloseInt32";
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return "kush::runtime::ColumnData::CloseInt64";
  } else if constexpr (catalog::TypeId::REAL == S) {
    return "kush::runtime::ColumnData::CloseFloat64";
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return "kush::runtime::ColumnData::CloseInt8";
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return "kush::runtime::ColumnData::CloseText";
  }
}

template <catalog::TypeId S>
void* CloseFn() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt16);
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt32);
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt64);
  } else if constexpr (catalog::TypeId::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseFloat64);
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt8);
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseText);
  }
}

template <catalog::TypeId S>
std::string_view GetFnName() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return "kush::runtime::ColumnData::GetInt16";
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return "kush::runtime::ColumnData::GetInt32";
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return "kush::runtime::ColumnData::GetInt64";
  } else if constexpr (catalog::TypeId::REAL == S) {
    return "kush::runtime::ColumnData::GetFloat64";
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return "kush::runtime::ColumnData::GetInt8";
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return "kush::runtime::ColumnData::GetText";
  }
}

template <catalog::TypeId S>
void* GetFn() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt16);
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt32);
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt64);
  } else if constexpr (catalog::TypeId::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetFloat64);
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt8);
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetText);
  }
}

template <catalog::TypeId S>
std::string_view SizeFnName() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return "kush::runtime::ColumnData::SizeInt16";
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return "kush::runtime::ColumnData::SizeInt32";
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return "kush::runtime::ColumnData::SizeInt64";
  } else if constexpr (catalog::TypeId::REAL == S) {
    return "kush::runtime::ColumnData::SizeFloat64";
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return "kush::runtime::ColumnData::SizeInt8";
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return "kush::runtime::ColumnData::SizeText";
  }
}

template <catalog::TypeId S>
void* SizeFn() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt16);
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt32);
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt64);
  } else if constexpr (catalog::TypeId::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeFloat64);
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt8);
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeText);
  }
}

template <catalog::TypeId S>
std::string_view StructName() {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return "kush::runtime::ColumnData::Int16ColumnData";
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return "kush::runtime::ColumnData::Int32ColumnData";
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return "kush::runtime::ColumnData::Int64ColumnData";
  } else if constexpr (catalog::TypeId::REAL == S) {
    return "kush::runtime::ColumnData::Float64ColumnData";
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return "kush::runtime::ColumnData::Int8ColumnData";
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return "kush::runtime::ColumnData::TextColumnData";
  }
}

template <catalog::TypeId S>
khir::Value GetStructInit(khir::ProgramBuilder& program) {
  auto st = program.GetStructType(StructName<S>());

  std::optional<khir::Type> elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::TypeId::SMALLINT == S) {
    elem_type = program.I16Type();
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    elem_type = program.I32Type();
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    elem_type = program.I64Type();
  } else if constexpr (catalog::TypeId::REAL == S) {
    elem_type = program.F64Type();
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    elem_type = program.I1Type();
  } else if constexpr (catalog::TypeId::TEXT == S) {
    elem_type = program.ArrayType(
        program.StructType({program.I32Type(), program.I32Type()}));
  }

  return program.ConstantStruct(
      st, {program.NullPtr(program.PointerType(elem_type.value())),
           program.ConstI32(0)});
}

template <catalog::TypeId S>
void* Allocate(execution::QueryState& state) {
  if constexpr (catalog::TypeId::SMALLINT == S) {
    return state.Allocate<runtime::ColumnData::Int16ColumnData>();
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    return state.Allocate<runtime::ColumnData::Int32ColumnData>();
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    return state.Allocate<runtime::ColumnData::Int64ColumnData>();
  } else if constexpr (catalog::TypeId::REAL == S) {
    return state.Allocate<runtime::ColumnData::Float64ColumnData>();
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    return state.Allocate<runtime::ColumnData::Int8ColumnData>();
  } else if constexpr (catalog::TypeId::TEXT == S) {
    return state.Allocate<runtime::ColumnData::TextColumnData>();
  }
}

template <catalog::TypeId S>
ColumnData<S>::ColumnData(khir::ProgramBuilder& program,
                          execution::QueryState& state, std::string_view path,
                          const catalog::Type& type)
    : program_(program),
      type_(type),
      path_(path),
      path_value_(program_.GlobalConstCharArray(path)),
      value_(program_.PointerCast(
          program_.ConstPtr(Allocate<S>(state)),
          program_.PointerType(program.GetStructType(StructName<S>())))) {
  if constexpr (S == catalog::TypeId::TEXT) {
    result_ = String::Global(program_, "").Get();
  }
}

template <catalog::TypeId S>
ColumnData<S>::ColumnData(khir::ProgramBuilder& program, std::string_view path,
                          const catalog::Type& type, khir::Value value)
    : program_(program),
      type_(type),
      path_(path),
      path_value_(program_.GlobalConstCharArray(path)),
      value_(value) {
  if constexpr (S == catalog::TypeId::TEXT) {
    result_ = String::Global(program_, "").Get();
  }
}

template <catalog::TypeId S>
void ColumnData<S>::Reset() {
  program_.Call(program_.GetFunction(CloseFnName<S>()), {value_});
}

template <catalog::TypeId S>
void ColumnData<S>::Init() {
  program_.Call(program_.GetFunction(OpenFnName<S>()), {value_, path_value_});
}

template <catalog::TypeId S>
Int32 ColumnData<S>::Size() {
  return Int32(program_,
               program_.Call(program_.GetFunction(SizeFnName<S>()), {value_}));
}

template <catalog::TypeId S>
khir::Value ColumnData<S>::Get() {
  return value_;
}

template <catalog::TypeId S>
std::unique_ptr<Iterable> ColumnData<S>::Regenerate(
    khir::ProgramBuilder& program, khir::Value value) {
  return std::make_unique<ColumnData<S>>(
      program, path_, type_,
      program.PointerCast(
          value, program.PointerType(program.GetStructType(StructName<S>()))));
}

template <catalog::TypeId S>
std::unique_ptr<IRValue> ColumnData<S>::operator[](Int32& idx) {
  if constexpr (catalog::TypeId::TEXT == S) {
    program_.Call(program_.GetFunction(GetFnName<S>()),
                  {value_, idx.Get(), result_.value()});
    return std::make_unique<String>(program_, result_.value());
  }

  auto data = program_.LoadPtr(program_.StaticGEP(
      program_.GetStructType(StructName<S>()), value_, {0, 0}));

  if constexpr (catalog::TypeId::SMALLINT == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.I16Type(), data, idx.Get(), {});
    return std::make_unique<Int16>(program_, program_.LoadI16(elem_ptr));
  } else if constexpr (catalog::TypeId::INT == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.I32Type(), data, idx.Get(), {});
    return std::make_unique<Int32>(program_, program_.LoadI32(elem_ptr));
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.I64Type(), data, idx.Get(), {});
    return std::make_unique<Int64>(program_, program_.LoadI64(elem_ptr));
  } else if constexpr (catalog::TypeId::REAL == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.F64Type(), data, idx.Get(), {});
    return std::make_unique<Float64>(program_, program_.LoadF64(elem_ptr));
  } else if constexpr (catalog::TypeId::DATE == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.I32Type(), data, idx.Get(), {});
    return std::make_unique<Date>(program_, program_.LoadI32(elem_ptr));
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    auto elem_ptr = program_.DynamicGEP(program_.I1Type(), data, idx.Get(), {});
    return std::make_unique<Bool>(program_, program_.LoadI1(elem_ptr));
  } else if constexpr (catalog::TypeId::ENUM == S) {
    auto elem_ptr =
        program_.DynamicGEP(program_.I32Type(), data, idx.Get(), {});
    return std::make_unique<Enum>(program_, type_.enum_id,
                                  program_.LoadI32(elem_ptr));
  }
}

template <catalog::TypeId S>
khir::Value ColumnData<S>::SimdLoad(Int32& idx) {
  if constexpr (catalog::TypeId::TEXT == S || catalog::TypeId::SMALLINT == S ||
                catalog::TypeId::BIGINT == S || catalog::TypeId::REAL == S ||
                catalog::TypeId::BOOLEAN == S) {
    throw std::runtime_error("Unsupported");
  }

  auto data = program_.LoadPtr(program_.StaticGEP(
      program_.GetStructType(StructName<S>()), value_, {0, 0}));
  auto elem_ptr = program_.DynamicGEP(program_.I32Type(), data, idx.Get(), {});
  auto casted = program_.PointerCast(
      elem_ptr, program_.PointerType(program_.I32Vec8Type()));
  return program_.LoadI32Vec8(casted);
}

template <catalog::TypeId S>
const catalog::Type& ColumnData<S>::Type() {
  return type_;
}

template <catalog::TypeId S>
void ColumnData<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<khir::Type> elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::TypeId::SMALLINT == S) {
    elem_type = program.I16Type();
  } else if constexpr (catalog::TypeId::INT == S ||
                       catalog::TypeId::DATE == S ||
                       catalog::TypeId::ENUM == S) {
    elem_type = program.I32Type();
  } else if constexpr (catalog::TypeId::BIGINT == S) {
    elem_type = program.I64Type();
  } else if constexpr (catalog::TypeId::REAL == S) {
    elem_type = program.F64Type();
  } else if constexpr (catalog::TypeId::BOOLEAN == S) {
    elem_type = program.I1Type();
  } else if constexpr (catalog::TypeId::TEXT == S) {
    elem_type = program.ArrayType(
        program.StructType({program.I32Type(), program.I64Type()}));
  }

  auto struct_type = program.StructType(
      {program.PointerType(elem_type.value()), program.I64Type()},
      StructName<S>());

  auto string_type = program.PointerType(program.I8Type());
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(OpenFnName<S>(), program.VoidType(),
                                  {struct_ptr, string_type}, OpenFn<S>());
  program.DeclareExternalFunction(CloseFnName<S>(), program.VoidType(),
                                  {struct_ptr}, CloseFn<S>());
  program.DeclareExternalFunction(SizeFnName<S>(), program.I32Type(),
                                  {struct_ptr}, SizeFn<S>());

  if constexpr (catalog::TypeId::TEXT == S) {
    program.DeclareExternalFunction(
        GetFnName<S>(), program.VoidType(),
        {struct_ptr, program.I32Type(),
         program.PointerType(program.GetStructType(String::StringStructName))},
        GetFn<S>());
  } else {
    program.DeclareExternalFunction(GetFnName<S>(), elem_type.value(),
                                    {struct_ptr, program.I32Type()},
                                    GetFn<S>());
  }
}

template class ColumnData<catalog::TypeId::SMALLINT>;
template class ColumnData<catalog::TypeId::INT>;
template class ColumnData<catalog::TypeId::BIGINT>;
template class ColumnData<catalog::TypeId::REAL>;
template class ColumnData<catalog::TypeId::DATE>;
template class ColumnData<catalog::TypeId::BOOLEAN>;
template class ColumnData<catalog::TypeId::TEXT>;
template class ColumnData<catalog::TypeId::ENUM>;

}  // namespace kush::compile::proxy