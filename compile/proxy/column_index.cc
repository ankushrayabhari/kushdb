#include "compile/proxy/column_index.h"

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

template <typename T, catalog::SqlType S>
ColumnIndexImpl<T, S>::ColumnIndexImpl(ProgramBuilder<T>& program, bool global)
    : program_(program) {}

template <typename T, catalog::SqlType S>
ColumnIndexImpl<T, S>::~ColumnIndexImpl() {}

template <typename T, catalog::SqlType S>
void ColumnIndexImpl<T, S>::Insert(proxy::Value<T>& v,
                                   proxy::Int32<T>& tuple_idx) {}

template <typename T, catalog::SqlType S>
proxy::Int32<T> ColumnIndexImpl<T, S>::GetNextGreater(
    proxy::Value<T>& v, proxy::Int32<T>& tuple_idx) {
  return proxy::Int32<T>(program_, 0);
}

INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::SMALLINT);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::INT);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::BIGINT);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::REAL);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::DATE);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::BOOLEAN);
INSTANTIATE_ON_IR(ColumnIndexImpl, catalog::SqlType::TEXT);

}  // namespace kush::compile::proxy