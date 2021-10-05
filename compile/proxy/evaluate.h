#pragma once

#include "compile/proxy/value/sql_value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

SQLValue EvaluateBinary(plan::BinaryArithmeticOperatorType op,
                        const SQLValue& lhs, const SQLValue& rhs);

}  // namespace kush::compile::proxy