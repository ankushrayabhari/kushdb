#pragma once

#include "compile/proxy/value/sql_value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

SQLValue EvaluateBinary(plan::BinaryArithmeticOperatorType op,
                        const SQLValue& lhs, const SQLValue& rhs);

// Differs from the above in that LT(x, y) is null when x is null or y is null
// This returns a not null boolean treating null as greater than any not-null
// val
Bool LessThan(const SQLValue& lhs, const SQLValue& rhs);

// Differs from the above in that EQ(x, y) is null when x is null or y is null
// This returns a not null boolean treating as only equal to null
Bool Equal(const SQLValue& lhs, const SQLValue& rhs);

}  // namespace kush::compile::proxy