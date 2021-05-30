#include "compile/proxy/column_index.h"

#include <iostream>
#include <memory>

#include "catalog/sql_type.h"
#include "compile/khir/program_builder.h"
#include "compile/proxy/float.h"
#include "compile/proxy/int.h"
#include "compile/proxy/ptr.h"
#include "compile/proxy/string.h"
#include "compile/proxy/value.h"

namespace kush::compile::proxy {

template <catalog::SqlType S>
std::string_view CreateFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data16CreateInt16IndexEv";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data16CreateInt32IndexEv";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data16CreateInt64IndexEv";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data18CreateFloat64IndexEv";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data15CreateInt8IndexEv";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data17CreateStringIndexB5cxx11Ev";
  }
}

template <catalog::SqlType S>
std::string_view FreeFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_"
           "mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS4_EEE";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_"
           "mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS4_EEE";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_"
           "mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS4_EEE";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_"
           "mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS4_EEE";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_"
           "mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS4_EEE";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data4FreeEPSt13unordered_mapINSt7__cxx1112basic_"
           "stringIcSt11char_traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS7_"
           "ESt8equal_toIS7_ESaISt4pairIKS7_SA_EEE";
  }
}

template <catalog::SqlType S>
std::string_view GetNextGreaterFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_"
           "mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS4_"
           "EEEsii";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_"
           "mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS4_"
           "EEEiii";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_"
           "mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS4_"
           "EEElii";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_"
           "mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS4_"
           "EEEdii";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_"
           "mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS4_"
           "EEEaii";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data12GetNextTupleEPSt13unordered_mapINSt7__cxx1112basic_"
           "stringIcSt11char_traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS7_"
           "ESt8equal_toIS7_ESaISt4pairIKS7_SA_EEEPNS0_6StringEii";
  }
}

template <catalog::SqlType S>
std::string_view InsertFnName() {
  if constexpr (catalog::SqlType::SMALLINT == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_"
           "mapIsSt6vectorIiSaIiEESt4hashIsESt8equal_toIsESaISt4pairIKsS4_"
           "EEEsi";
  } else if constexpr (catalog::SqlType::INT == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_"
           "mapIiSt6vectorIiSaIiEESt4hashIiESt8equal_toIiESaISt4pairIKiS4_"
           "EEEii";
  } else if constexpr (catalog::SqlType::BIGINT == S ||
                       catalog::SqlType::DATE == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_"
           "mapIlSt6vectorIiSaIiEESt4hashIlESt8equal_toIlESaISt4pairIKlS4_"
           "EEEli";
  } else if constexpr (catalog::SqlType::REAL == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_"
           "mapIdSt6vectorIiSaIiEESt4hashIdESt8equal_toIdESaISt4pairIKdS4_"
           "EEEdi";
  } else if constexpr (catalog::SqlType::BOOLEAN == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_"
           "mapIaSt6vectorIiSaIiEESt4hashIaESt8equal_toIaESaISt4pairIKaS4_"
           "EEEai";
  } else if constexpr (catalog::SqlType::TEXT == S) {
    return "_ZN4kush4data6InsertEPSt13unordered_mapINSt7__cxx1112basic_"
           "stringIcSt11char_traitsIcESaIcEEESt6vectorIiSaIiEESt4hashIS7_"
           "ESt8equal_toIS7_ESaISt4pairIKS7_SA_EEEPNS0_6StringEi";
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

  program.DeclareExternalFunction(CreateFnName<S>(), index_ptr_type, {});
  program.DeclareExternalFunction(FreeFnName<S>(), program.VoidType(),
                                  {index_ptr_type});
  program.DeclareExternalFunction(
      InsertFnName<S>(), program.VoidType(),
      {index_ptr_type, value_type.value(), program.I32Type()});
  program.DeclareExternalFunction(GetNextGreaterFnName<S>(), program.I32Type(),
                                  {index_ptr_type, value_type.value(),
                                   program.I32Type(), program.I32Type()});
}

template class ColumnIndexImpl<catalog::SqlType::SMALLINT>;
template class ColumnIndexImpl<catalog::SqlType::INT>;
template class ColumnIndexImpl<catalog::SqlType::BIGINT>;
template class ColumnIndexImpl<catalog::SqlType::REAL>;
template class ColumnIndexImpl<catalog::SqlType::DATE>;
template class ColumnIndexImpl<catalog::SqlType::BOOLEAN>;
template class ColumnIndexImpl<catalog::SqlType::TEXT>;

}  // namespace kush::compile::proxy