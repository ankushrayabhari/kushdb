#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"
#include "khir/program_builder.h"
#include "runtime/column_index.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view CreateFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt16IndexEv";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt32IndexEv";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex16CreateInt64IndexEv";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex18CreateFloat64IndexEv";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex15CreateInt8IndexEv";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex15CreateTextIndexB5cxx11Ev";
  }
}

template <catalog::SqlType S>
void* CreateFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::CreateTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view FreeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt16IndexEPSt13unordered_"
           "mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS5_EEE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt32IndexEPSt13unordered_"
           "mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS5_EEE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex14FreeInt64IndexEPSt13unordered_"
           "mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS5_EEE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex16FreeFloat64IndexEPSt13unordered_"
           "mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS5_EEE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex13FreeInt8IndexEPSt13unordered_"
           "mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS5_EEE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex13FreeTextIndexEPSt13unordered_"
           "mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS8_ESt8equal_toIS8_"
           "ESaISt4pairIKS8_SB_EEE";
  }
}

template <catalog::SqlType S>
void* FreeFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::FreeTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view GetNextGreaterFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex22GetNextTupleInt16IndexEPSt13unordere"
           "d_mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS5_"
           "EEEsii";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex22GetNextTupleInt32IndexEPSt13unordere"
           "d_mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS5_"
           "EEEiii";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex22GetNextTupleInt64IndexEPSt13unordere"
           "d_mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS5_"
           "EEElii";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex24GetNextTupleFloat64IndexEPSt13unorde"
           "red_mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS5_"
           "EEEdii";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex21GetNextTupleInt8IndexEPSt13unordered"
           "_mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS5_"
           "EEEaii";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex21GetNextTupleTextIndexEPSt13unordered"
           "_mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS8_ESt8equal_toIS8_"
           "ESaISt4pairIKS8_SB_EEEPNS0_6String6StringEii";
  }
}

template <catalog::SqlType S>
void* GetNextGreaterFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(
        &runtime::ColumnIndex::GetNextTupleTextIndex);
  }
}

template <catalog::SqlType S>
std::string_view InsertFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt16IndexEPSt13unordered_"
           "mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS5_"
           "EEEsi";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt32IndexEPSt13unordered_"
           "mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS5_"
           "EEEii";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush7runtime11ColumnIndex16InsertInt64IndexEPSt13unordered_"
           "mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS5_"
           "EEEli";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush7runtime11ColumnIndex18InsertFloat64IndexEPSt13unordered_"
           "mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS5_"
           "EEEdi";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush7runtime11ColumnIndex15InsertInt8IndexEPSt13unordered_"
           "mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS5_"
           "EEEai";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush7runtime11ColumnIndex15InsertTextIndexEPSt13unordered_"
           "mapINSt7__cxx1112basic_stringIcSt11char_"
           "traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS8_ESt8equal_toIS8_"
           "ESaISt4pairIKS8_SB_EEEPNS0_6String6StringEi";
  }
}

template <catalog::SqlType S>
void* InsertFn() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt16Index);
  } else if constexpr (catalog::SqlType::INT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt32Index);
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt64Index);
  } else if constexpr (catalog::SqlType::REAL == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertFloat64Index);
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertInt8Index);
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return reinterpret_cast<void*>(&runtime::ColumnIndex::InsertTextIndex);
  }
}

template <catalog::SqlType S>
GlobalColumnIndexImpl<S>::GlobalColumnIndexImpl(khir::ProgramBuilder& program)
    : program_(program),
      global_ref_generator_(program.Global(
          false, false, program.PointerType(program.I8Type()),
          program.NullPtr(program.PointerType(program.I8Type())))) {
  program_.Store(global_ref_generator_(),
                 program_.Call(program_.GetFunction(CreateFnName<S>()), {}));
}

template <catalog::SqlType S>
void GlobalColumnIndexImpl<S>::Reset() {
  program_.Call(program_.GetFunction(FreeFnName<S>()),
                {program_.Load(global_ref_generator_())});
}

template <catalog::SqlType S>
std::unique_ptr<ColumnIndex> GlobalColumnIndexImpl<S>::Get() {
  auto local_v = global_ref_generator_();
  return std::make_unique<ColumnIndexImpl<S>>(program_, local_v);
}

template class GlobalColumnIndexImpl<catalog::SqlType::SMALLINT>;
template class GlobalColumnIndexImpl<catalog::SqlType::INT>;
template class GlobalColumnIndexImpl<catalog::SqlType::BIGINT>;
template class GlobalColumnIndexImpl<catalog::SqlType::REAL>;
template class GlobalColumnIndexImpl<catalog::SqlType::DATE>;
template class GlobalColumnIndexImpl<catalog::SqlType::BOOLEAN>;
template class GlobalColumnIndexImpl<catalog::SqlType::TEXT>;

template <catalog::SqlType S>
ColumnIndexImpl<S>::ColumnIndexImpl(khir::ProgramBuilder& program,
                                    khir::Value value)
    : program_(program), value_(value) {}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::Insert(const proxy::Value& v,
                                const proxy::Int32& tuple_idx) {
  program_.Call(program_.GetFunction(InsertFnName<S>()),
                {program_.Load(value_), v.Get(), tuple_idx.Get()});
}

template <catalog::SqlType S>
proxy::Int32 ColumnIndexImpl<S>::GetNextGreater(
    const proxy::Value& v, const proxy::Int32& tuple_idx,
    const proxy::Int32& cardinality) {
  return proxy::Int32(
      program_, program_.Call(program_.GetFunction(GetNextGreaterFnName<S>()),
                              {program_.Load(value_), v.Get(), tuple_idx.Get(),
                               cardinality.Get()}));
}

template <catalog::SqlType S>
void ColumnIndexImpl<S>::ForwardDeclare(khir::ProgramBuilder& program) {
  auto index_type = program.I8Type();
  auto index_ptr_type = program.PointerType(index_type);

  std::optional<typename khir::Type> value_type;
  if constexpr (catalog::SqlType::SMALLINT == S) {
    value_type = program.I16Type();
  } else if constexpr (catalog::SqlType::INT == S) {
    value_type = program.I32Type();
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    value_type = program.I64Type();
  } else if constexpr (catalog::SqlType::REAL == S) {
    value_type = program.F64Type();
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    value_type = program.I8Type();
  } else if constexpr (catalog::SqlType::TEXT == S) {
    value_type =
        program.PointerType(program.GetStructType(String::StringStructName));
  }

  program.DeclareExternalFunction(CreateFnName<S>(), index_ptr_type, {},
                                  CreateFn<S>());
  program.DeclareExternalFunction(FreeFnName<S>(), program.VoidType(),
                                  {index_ptr_type}, FreeFn<S>());
  program.DeclareExternalFunction(
      InsertFnName<S>(), program.VoidType(),
      {index_ptr_type, value_type.value(), program.I32Type()}, InsertFn<S>());
  program.DeclareExternalFunction(GetNextGreaterFnName<S>(), program.I32Type(),
                                  {index_ptr_type, value_type.value(),
                                   program.I32Type(), program.I32Type()},
                                  GetNextGreaterFn<S>());
}

template class ColumnIndexImpl<catalog::SqlType::SMALLINT>;
template class ColumnIndexImpl<catalog::SqlType::INT>;
template class ColumnIndexImpl<catalog::SqlType::BIGINT>;
template class ColumnIndexImpl<catalog::SqlType::REAL>;
template class ColumnIndexImpl<catalog::SqlType::DATE>;
template class ColumnIndexImpl<catalog::SqlType::BOOLEAN>;
template class ColumnIndexImpl<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy