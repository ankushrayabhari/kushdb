#include "compile/proxy/column_data.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/khir/khir_program_builder.h"
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
    return "_ZN4kush4data3GetEPNS0_15Int16ColumnDataEi";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data3GetEPNS0_15Int32ColumnDataEi";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data3GetEPNS0_15Int64ColumnDataEi";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data3GetEPNS0_17Float64ColumnDataEi";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data3GetEPNS0_14Int8ColumnDataEi";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data3GetEPNS0_14TextColumnDataEiPNS0_6StringE";
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

template <catalog::SqlType S>
ColumnData<S>::ColumnData(khir::KHIRProgramBuilder& program,
                          std::string_view path)
    : program_(program) {
  auto path_value = program_.GlobalConstCharArray(path);
  value_ = program_.Alloca(program.GetStructType(StructName<S>()));
  program_.Call(program_.GetFunction(OpenFnName<S>()),
                {value_.value(), path_value});

  if constexpr (S == catalog::SqlType::TEXT) {
    result_ = program_.Alloca(program_.GetStructType(String::StringStructName));
  }
}

template <catalog::SqlType S>
ColumnData<S>::~ColumnData() {
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
void ColumnData<S>::ForwardDeclare(khir::KHIRProgramBuilder& program) {
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
                                  {struct_ptr, string_type});
  program.DeclareExternalFunction(CloseFnName<S>(), program.VoidType(),
                                  {struct_ptr});
  program.DeclareExternalFunction(SizeFnName<S>(), program.I32Type(),
                                  {struct_ptr});

  if constexpr (catalog::SqlType::TEXT == S) {
    program.DeclareExternalFunction(
        GetFnName<S>(), program.VoidType(),
        {struct_ptr, program.I32Type(),
         program.PointerType(program.GetStructType(String::StringStructName))});
  } else {
    program.DeclareExternalFunction(GetFnName<S>(), elem_type.value(),
                                    {struct_ptr, program.I32Type()});
  }
}

}  // namespace kush::compile::proxy