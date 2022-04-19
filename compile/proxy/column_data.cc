#include "compile/proxy/column_data.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/value/ir_value.h"
#include "khir/program_builder.h"
#include "runtime/column_data.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view OpenFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnData::OpenInt16";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnData::OpenInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnData::OpenInt64";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnData::OpenFloat64";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnData::OpenInt8";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnData::OpenText";
  }
}

template <catalog::SqlType S>
void* OpenFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt16);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::OpenText);
  }
}

template <catalog::SqlType S>
std::string_view CloseFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnData::CloseInt16";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnData::CloseInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnData::CloseInt64";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnData::CloseFloat64";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnData::CloseInt8";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnData::CloseText";
  }
}

template <catalog::SqlType S>
void* CloseFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt16);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::CloseText);
  }
}

template <catalog::SqlType S>
std::string_view GetFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnData::GetInt16";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnData::GetInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnData::GetInt64";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnData::GetFloat64";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnData::GetInt8";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnData::GetText";
  }
}

template <catalog::SqlType S>
void* GetFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt16);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::GetText);
  }
}

template <catalog::SqlType S>
std::string_view SizeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnData::SizeInt16";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnData::SizeInt32";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnData::SizeInt64";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnData::SizeFloat64";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnData::SizeInt8";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnData::SizeText";
  }
}

template <catalog::SqlType S>
void* SizeFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt16);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt32);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt64);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeFloat64);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeInt8);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnData::SizeText);
  }
}

template <catalog::SqlType S>
std::string_view StructName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::runtime::ColumnData::Int16ColumnData";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::runtime::ColumnData::Int32ColumnData";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::runtime::ColumnData::Int64ColumnData";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::runtime::ColumnData::Float64ColumnData";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::runtime::ColumnData::Int8ColumnData";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::runtime::ColumnData::TextColumnData";
  }
}

template <catalog::SqlType S>
khir::Value GetStructInit(khir::ProgramBuilder& program) {
  auto st = program.GetStructType(StructName<S>());

  std::optional<khir::Type> elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::SqlType::SMALLINT == S) {
    elem_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    elem_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    elem_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    elem_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    elem_type = program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    elem_type = program.ArrayType(
        program.StructType({program.I32Type(), program.I32Type()}));
  }

  return program.ConstantStruct(
      st, {program.NullPtr(program.PointerType(elem_type.value())),
           program.ConstI32(0)});
}

template <catalog::SqlType S>
ColumnData<S>::ColumnData(khir::ProgramBuilder& program, std::string_view path)
    : program_(program),
      path_(path),
      path_value_(program_.GlobalConstCharArray(path)),
      value_(program_.Global(program.GetStructType(StructName<S>()),
                             GetStructInit<S>(program))) {
  if constexpr (S == catalog::SqlType::TEXT) {
    result_ = String::Global(program_, "").Get();
  }
}

template <catalog::SqlType S>
ColumnData<S>::ColumnData(khir::ProgramBuilder& program, std::string_view path,
                          khir::Value value)
    : program_(program),
      path_(path),
      path_value_(program_.GlobalConstCharArray(path)),
      value_(value) {
  if constexpr (S == catalog::SqlType::TEXT) {
    result_ = String::Global(program_, "").Get();
  }
}

template <catalog::SqlType S>
void ColumnData<S>::Reset() {
  program_.Call(program_.GetFunction(CloseFnName<S>()), {value_});
}

template <catalog::SqlType S>
void ColumnData<S>::Init() {
  program_.Call(program_.GetFunction(OpenFnName<S>()), {value_, path_value_});
}

template <catalog::SqlType S>
Int32 ColumnData<S>::Size() {
  return Int32(program_,
               program_.Call(program_.GetFunction(SizeFnName<S>()), {value_}));
}

template <catalog::SqlType S>
khir::Value ColumnData<S>::Get() {
  return value_;
}

template <catalog::SqlType S>
std::unique_ptr<Iterable> ColumnData<S>::Regenerate(
    khir::ProgramBuilder& program, khir::Value value) {
  return std::make_unique<ColumnData<S>>(
      program, path_,
      program.PointerCast(
          value, program.PointerType(program.GetStructType(StructName<S>()))));
}

template <catalog::SqlType S>
std::unique_ptr<IRValue> ColumnData<S>::operator[](Int32& idx) {
  if constexpr (catalog::SqlType::TEXT == S) {
    program_.Call(program_.GetFunction(GetFnName<S>()),
                  {value_, idx.Get(), result_.value()});
    return std::make_unique<String>(program_, result_.value());
  }

  auto elem =
      program_.Call(program_.GetFunction(GetFnName<S>()), {value_, idx.Get()});

  if constexpr (catalog::SqlType::SMALLINT == S) {
    return std::make_unique<Int16>(program_, elem);
  } else if constexpr (catalog::SqlType::INT == S) {
    return std::make_unique<Int32>(program_, elem);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
    return std::make_unique<Int64>(program_, elem);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return std::make_unique<Float64>(program_, elem);
  } else if constexpr (catalog::SqlType::DATE == S) {
    return std::make_unique<Date>(program_, elem);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return std::make_unique<Bool>(program_, elem);
  }
}

template <catalog::SqlType S>
catalog::SqlType ColumnData<S>::Type() {
  return S;
}

template <catalog::SqlType S>
void ColumnData<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<khir::Type> elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::SqlType::SMALLINT == S) {
    elem_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    elem_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    elem_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    elem_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    elem_type = program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    elem_type = program.ArrayType(
        program.StructType({program.I32Type(), program.I64Type()}));
  }

  auto string_type = program.PointerType(program.I8Type());
  auto struct_type = program.StructType(
      {program.PointerType(elem_type.value()), program.I64Type()},
      StructName<S>());
  auto struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(OpenFnName<S>(), program.VoidType(),
                                  {struct_ptr, string_type}, OpenFn<S>());
  program.DeclareExternalFunction(CloseFnName<S>(), program.VoidType(),
                                  {struct_ptr}, CloseFn<S>());
  program.DeclareExternalFunction(SizeFnName<S>(), program.I32Type(),
                                  {struct_ptr}, SizeFn<S>());

  if constexpr (catalog::SqlType::TEXT == S) {
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

template class ColumnData<catalog::SqlType::SMALLINT>;
template class ColumnData<catalog::SqlType::INT>;
template class ColumnData<catalog::SqlType::BIGINT>;
template class ColumnData<catalog::SqlType::REAL>;
template class ColumnData<catalog::SqlType::DATE>;
template class ColumnData<catalog::SqlType::BOOLEAN>;
template class ColumnData<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy