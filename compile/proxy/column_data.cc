#include "compile/proxy/column_data.h"

#include <memory>

#include "catalog/sql_type.h"
#include "compile/ir_registry.h"
#include "compile/program_builder.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <typename T>
ForwardDeclaredColumnDataFunctions<T>::ForwardDeclaredColumnDataFunctions(
    typename ProgramBuilder<T>::Type& struct_type,
    typename ProgramBuilder<T>::Function& open,
    typename ProgramBuilder<T>::Function& close,
    typename ProgramBuilder<T>::Function& get,
    typename ProgramBuilder<T>::Function& size)
    : struct_type_(struct_type),
      open_(open),
      close_(close),
      get_(get),
      size_(size) {}

template <typename T>
typename ProgramBuilder<T>::Type&
ForwardDeclaredColumnDataFunctions<T>::StructType() {
  return struct_type_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredColumnDataFunctions<T>::Open() {
  return open_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredColumnDataFunctions<T>::Close() {
  return close_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredColumnDataFunctions<T>::Get() {
  return get_;
}

template <typename T>
typename ProgramBuilder<T>::Function&
ForwardDeclaredColumnDataFunctions<T>::Size() {
  return size_;
}

INSTANTIATE_ON_IR(ForwardDeclaredColumnDataFunctions);

template <typename T, catalog::SqlType S>
ColumnData<T, S>::ColumnData(ProgramBuilder<T>& program,
                             ForwardDeclaredColumnDataFunctions<T>& funcs,
                             std::string_view path)
    : program_(program), funcs_(funcs) {
  auto& path_value = program_.CreateGlobal(path);
  auto& size = program_.SizeOf(funcs_.StructType());
  value_ = &program.PointerCast(program_.Malloc(size),
                                program_.PointerType(funcs_.StructType()));
  program_.Call(funcs_.Open(), {*value_, path_value});
}

template <typename T, catalog::SqlType S>
ColumnData<T, S>::~ColumnData() {
  program_.Call(funcs_.Close(), {*value_});
}

template <typename T, catalog::SqlType S>
UInt32<T> ColumnData<T, S>::Size() {
  return UInt32<T>(program_, program_.Call(funcs_.Size(), {*value_}));
}

template <typename T, catalog::SqlType S>
std::unique_ptr<Value<T>> ColumnData<T, S>::operator[](UInt32<T>& idx) {
  auto& elem = program_.Call(funcs_.Get(), {*value_, idx.Get()});

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
ForwardDeclaredColumnDataFunctions<T> ColumnData<T, S>::ForwardDeclare(
    ProgramBuilder<T>& program) {
  std::string_view open_name, close_name, get_name, size_name;
  typename ProgramBuilder<T>::Type* elem_type;

  // Initialize all the mangled names and the corresponding data type
  if constexpr (catalog::SqlType::SMALLINT == S) {
    open_name = "_ZN4kush4data4OpenEPNS0_15Int16ColumnDataEPKc";
    close_name = "_ZN4kush4data5CloseEPNS0_15Int16ColumnDataE";
    get_name = "_ZN4kush4data3GetEPNS0_15Int16ColumnDataEj";
    size_name = "_ZN4kush4data4SizeEPNS0_15Int16ColumnDataE";
    elem_type = &program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    open_name = "_ZN4kush4data4OpenEPNS0_15Int32ColumnDataEPKc";
    close_name = "_ZN4kush4data5CloseEPNS0_15Int32ColumnDataE";
    get_name = "_ZN4kush4data3GetEPNS0_15Int32ColumnDataEj";
    size_name = "_ZN4kush4data4SizeEPNS0_15Int32ColumnDataE";
    elem_type = &program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    open_name = "_ZN4kush4data4OpenEPNS0_15Int64ColumnDataEPKc";
    close_name = "_ZN4kush4data5CloseEPNS0_15Int64ColumnDataE";
    get_name = "_ZN4kush4data3GetEPNS0_15Int64ColumnDataEj";
    size_name = "_ZN4kush4data4SizeEPNS0_15Int64ColumnDataE";
    elem_type = &program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    open_name = "_ZN4kush4data4OpenEPNS0_17Float64ColumnDataEPKc";
    close_name = "_ZN4kush4data5CloseEPNS0_17Float64ColumnDataE";
    get_name = "_ZN4kush4data3GetEPNS0_17Float64ColumnDataEj";
    size_name = "_ZN4kush4data4SizeEPNS0_17Float64ColumnDataE";
    elem_type = &program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    open_name = "_ZN4kush4data4OpenEPNS0_14Int8ColumnDataEPKc";
    close_name = "_ZN4kush4data5CloseEPNS0_14Int8ColumnDataE";
    get_name = "_ZN4kush4data3GetEPNS0_14Int8ColumnDataEj";
    size_name = "_ZN4kush4data4SizeEPNS0_14Int8ColumnDataE";
    elem_type = &program.I8Type();
  }

  auto& string_type = program.PointerType(program.I8Type());
  auto& struct_type =
      program.StructType({program.PointerType(*elem_type), program.I32Type()});
  auto& struct_ptr = program.PointerType(struct_type);

  auto& open_fn = program.DeclareExternalFunction(open_name, program.VoidType(),
                                                  {struct_ptr, string_type});
  auto& close_fn = program.DeclareExternalFunction(
      close_name, program.VoidType(), {struct_ptr});
  auto& get_fn = program.DeclareExternalFunction(
      get_name, *elem_type, {struct_ptr, program.I32Type()});
  auto& size_fn = program.DeclareExternalFunction(size_name, program.I32Type(),
                                                  {struct_ptr});

  return ForwardDeclaredColumnDataFunctions<T>(struct_type, open_fn, close_fn,
                                               get_fn, size_fn);
}

INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::SMALLINT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::INT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::BIGINT);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::REAL);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::DATE);
INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::BOOLEAN);

// TODO: add after proxy::StringView implementation
// INSTANTIATE_ON_IR(ColumnData, catalog::SqlType::TEXT);

}  // namespace kush::compile::proxy