#include "compile/proxy/column_data.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view OpenFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data4OpenEPNS0_15Int16ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data4OpenEPNS0_15Int32ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data4OpenEPNS0_15Int64ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data4OpenEPNS0_17Float64ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data4OpenEPNS0_14Int8ColumnDataEPKc";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data4OpenEPNS0_14TextColumnDataEPKc";
  }
}

template <catalog::SqlType S>
std::string_view CloseFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data5CloseEPNS0_15Int16ColumnDataE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data5CloseEPNS0_15Int32ColumnDataE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data5CloseEPNS0_15Int64ColumnDataE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data5CloseEPNS0_17Float64ColumnDataE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data5CloseEPNS0_14Int8ColumnDataE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data5CloseEPNS0_14TextColumnDataE";
  }
}

template <catalog::SqlType S>
std::string_view GetFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data3GetEPNS0_15Int16ColumnDataEj";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data3GetEPNS0_15Int32ColumnDataEj";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data3GetEPNS0_15Int64ColumnDataEj";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data3GetEPNS0_17Float64ColumnDataEj";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data3GetEPNS0_14Int8ColumnDataEj";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data3GetEPNS0_14TextColumnDataEjPNS0_6StringE";
  }
}

template <catalog::SqlType S>
std::string_view SizeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data4SizeEPNS0_15Int16ColumnDataE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data4SizeEPNS0_15Int32ColumnDataE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data4SizeEPNS0_15Int64ColumnDataE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data4SizeEPNS0_17Float64ColumnDataE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data4SizeEPNS0_14Int8ColumnDataE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data4SizeEPNS0_14TextColumnDataE";
  }
}

template <catalog::SqlType S>
std::string_view StructName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "kush::data::Int16ColumnData";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "kush::data::Int32ColumnData";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "kush::data::Int64ColumnData";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "kush::data::Float64ColumnData";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "kush::data::Int8ColumnData";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "kush::data::TextColumnData";
  }
}

template <typename T, catalog::SqlType S>
ColumnData<T, S>::ColumnData(ProgramBuilder<T>& program, std::string_view path)
    : program_(program) {
  auto& path_value = program_.GlobalConstString(path);
  value_ = &program_.Alloca(program.GetStructType(StructName<S>()));
  program_.Call(program_.GetFunction(OpenFnName<S>()), {*value_, path_value});

  if constexpr (S == catalog::SqlType::TEXT) {
    result_ =
        &program_.Alloca(program_.GetStructType(String<T>::StringStructName));
  }
}

template <typename T, catalog::SqlType S>
ColumnData<T, S>::~ColumnData() {
  program_.Call(program_.GetFunction(CloseFnName<S>()), {*value_});
}

template <typename T, catalog::SqlType S>
UInt32<T> ColumnData<T, S>::Size() {
  return UInt32<T>(
      program_,
      program_.Call(program_.GetFunction(SizeFnName<S>()), {*value_}));
}

template <typename T, catalog::SqlType S>
std::unique_ptr<Value<T>> ColumnData<T, S>::operator[](UInt32<T>& idx) {
  if constexpr (catalog::SqlType::TEXT == S) {
    program_.Call(program_.GetFunction(GetFnName<S>()),
                  {*value_, idx.Get(), *result_});
    return std::make_unique<String<T>>(program_, *result_);
  }

  auto& elem =
      program_.Call(program_.GetFunction(GetFnName<S>()), {*value_, idx.Get()});

  if constexpr (catalog::SqlType::SMALLINT == S) {
    return std::make_unique<Int16<T>>(program_, elem);
  } else if constexpr (catalog::SqlType::INT == S) {
    return std::make_unique<Int32<T>>(program_, elem);
  } else if constexpr (catalog::SqlType::BIGINT == S) {
    return std::make_unique<Int64<T>>(program_, elem);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return std::make_unique<Float64<T>>(program_, elem);
  } else if constexpr (catalog::SqlType::DATE == S) {
    return std::make_unique<Int64<T>>(program_, elem);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return std::make_unique<Bool<T>>(program_, elem);
  }
}

template <typename T, catalog::SqlType S>
void ColumnData<T, S>::ForwardDeclare(ProgramBuilder<T>& program) {
  typename ProgramBuilder<T>::Type* elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::SqlType::SMALLINT == S) {
    elem_type = &program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    elem_type = &program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    elem_type = &program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    elem_type = &program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    elem_type = &program.I1Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    elem_type = &program.ArrayType(
        program.StructType({program.I32Type(), program.I32Type()}));
  }

  auto& string_type = program.PointerType(program.I8Type());
  auto& struct_type = program.StructType(
      {program.PointerType(*elem_type), program.I32Type()}, StructName<S>());
  auto& struct_ptr = program.PointerType(struct_type);

  program.DeclareExternalFunction(OpenFnName<S>(), program.VoidType(),
                                  {struct_ptr, string_type});
  program.DeclareExternalFunction(CloseFnName<S>(), program.VoidType(),
                                  {struct_ptr});
  program.DeclareExternalFunction(SizeFnName<S>(), program.I32Type(),
                                  {struct_ptr});

  if constexpr (catalog::SqlType::TEXT == S) {
    program.DeclareExternalFunction(GetFnName<S>(), program.VoidType(),
                                    {struct_ptr, program.I32Type(),
                                     program.PointerType(program.GetStructType(
                                         String<T>::StringStructName))});
  } else {
    program.DeclareExternalFunction(GetFnName<S>(), *elem_type,
                                    {struct_ptr, program.I32Type()});
  }
}

INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::SMALLINT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::INT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::BIGINT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::REAL);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::DATE);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::BOOLEAN);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::TEXT);

}  // namespace kush::compile::proxy