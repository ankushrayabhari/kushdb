#include "compile/proxy/column_data.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "runtime/column_data.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view OpenFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime10ColumnData9OpenInt16EPNS1_15Int16ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime10ColumnData9OpenInt32EPNS1_15Int32ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime10ColumnData9OpenInt64EPNS1_15Int64ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime10ColumnData11OpenFloat64EPNS1_"
           "17Float64ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime10ColumnData8OpenInt8EPNS1_14Int8ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime10ColumnData8OpenTextEPNS1_14TextColumnDataEPKc";
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
    return "_ZN4kush7runtime10ColumnData10CloseInt16EPNS1_15Int16ColumnDataE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime10ColumnData10CloseInt32EPNS1_15Int32ColumnDataE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime10ColumnData10CloseInt64EPNS1_15Int64ColumnDataE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime10ColumnData12CloseFloat64EPNS1_"
           "17Float64ColumnDataE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime10ColumnData9CloseInt8EPNS1_14Int8ColumnDataE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime10ColumnData9CloseTextEPNS1_14TextColumnDataE";
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
    return "_ZN4kush7runtime10ColumnData8GetInt16EPNS1_15Int16ColumnDataEi";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime10ColumnData8GetInt32EPNS1_15Int32ColumnDataEi";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime10ColumnData8GetInt64EPNS1_15Int64ColumnDataEi";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime10ColumnData10GetFloat64EPNS1_"
           "17Float64ColumnDataEi";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime10ColumnData7GetInt8EPNS1_14Int8ColumnDataEi";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime10ColumnData7GetTextEPNS1_14TextColumnDataEiPNS0_"
           "6String6StringE";
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
    return "_ZN4kush7runtime10ColumnData9SizeInt16EPNS1_15Int16ColumnDataE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime10ColumnData9SizeInt32EPNS1_15Int32ColumnDataE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime10ColumnData9SizeInt64EPNS1_15Int64ColumnDataE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime10ColumnData11SizeFloat64EPNS1_"
           "17Float64ColumnDataE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime10ColumnData8SizeInt8EPNS1_14Int8ColumnDataE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime10ColumnData8SizeTextEPNS1_14TextColumnDataE";
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
ColumnData<S>::ColumnData(khir::ProgramBuilder& program, std::string_view path)
    : program_(program) {
  auto path_value = program_.GlobalConstCharArray(path);
  value_ = program_.Alloca(program.GetStructType(StructName<S>()));
  program_.Call(program_.GetFunction(OpenFnName<S>()),
                {value_.value(), path_value()});

  if constexpr (S == catalog::SqlType::TEXT) {
    result_ = program_.Alloca(program_.GetStructType(String::StringStructName));
  }
}

template <catalog::SqlType S>
void ColumnData<S>::Reset() {
  program_.Call(program_.GetFunction(CloseFnName<S>()), {value_.value()});
}

template <catalog::SqlType S>
Int32 ColumnData<S>::Size() {
  return Int32(program_, program_.Call(program_.GetFunction(SizeFnName<S>()),
                                       {value_.value()}));
}

template <catalog::SqlType S>
std::unique_ptr<Value> ColumnData<S>::operator[](Int32& idx) {
  if constexpr (catalog::SqlType::TEXT == S) {
    program_.Call(program_.GetFunction(GetFnName<S>()),
                  {value_.value(), idx.Get(), result_.value()});
    return std::make_unique<String>(program_, result_.value());
  }

  auto elem = program_.Call(program_.GetFunction(GetFnName<S>()),
                            {value_.value(), idx.Get()});

  if constexpr (catalog::SqlType::SMALLINT == S) {
    return std::make_unique<Int16>(program_, elem);
  } else if constexpr (catalog::SqlType::INT == S) {
    return std::make_unique<Int32>(program_, elem);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
    return std::make_unique<Int64>(program_, elem);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return std::make_unique<Float64>(program_, elem);
  } else if constexpr (catalog::SqlType::DATE == S) {
    return std::make_unique<Int64>(program_, elem);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return std::make_unique<Bool>(program_, elem);
  }
}

template <catalog::SqlType S>
void ColumnData<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  std::optional<typename khir::Type> elem_type;

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

  auto string_type = program.PointerType(program.I8Type());
  auto struct_type = program.StructType(
      {program.PointerType(elem_type.value()), program.I32Type()},
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