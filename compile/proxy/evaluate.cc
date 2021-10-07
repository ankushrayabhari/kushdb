#include "compile/proxy/evaluate.h"

#include "catalog/sql_type.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "plan/expression/binary_arithmetic_expression.h"

namespace kush::compile::proxy {

SQLValue EvaluateBinaryBool(plan::BinaryArithmeticOperatorType op_type,
                            const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  return NullableTernary<Bool>(
      program, lhs.IsNull() || rhs.IsNull(),
      [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
      [&]() {
        const Bool& lhs_bool = dynamic_cast<const Bool&>(lhs.Get());
        const Bool& rhs_bool = dynamic_cast<const Bool&>(rhs.Get());
        Bool null(program, false);

        switch (op_type) {
          case plan::BinaryArithmeticOperatorType::AND:
            return SQLValue(lhs_bool && rhs_bool, null);

          case plan::BinaryArithmeticOperatorType::OR:
            return SQLValue(lhs_bool || rhs_bool, null);

          case plan::BinaryArithmeticOperatorType::EQ:
            return SQLValue(lhs_bool == rhs_bool, null);

          case plan::BinaryArithmeticOperatorType::NEQ:
            return SQLValue(lhs_bool != rhs_bool, null);

          default:
            throw std::runtime_error("Invalid operator on Bool");
        }
      });
}

template <typename V>
SQLValue EvaluateBinaryNumeric(plan::BinaryArithmeticOperatorType op_type,
                               const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  switch (op_type) {
    case plan::BinaryArithmeticOperatorType::ADD:
    case plan::BinaryArithmeticOperatorType::SUB:
    case plan::BinaryArithmeticOperatorType::MUL: {
      return NullableTernary<V>(
          program, lhs.IsNull() || rhs.IsNull(),
          [&]() {
            return SQLValue(static_cast<V&>(lhs.Get()), Bool(program, true));
          },
          [&]() {
            const V& lhs_v = dynamic_cast<const V&>(lhs.Get());
            const V& rhs_v = dynamic_cast<const V&>(rhs.Get());
            Bool null(program, false);

            switch (op_type) {
              case plan::BinaryArithmeticOperatorType::ADD:
                return SQLValue(lhs_v + rhs_v, null);

              case plan::BinaryArithmeticOperatorType::SUB:
                return SQLValue(lhs_v - rhs_v, null);

              case plan::BinaryArithmeticOperatorType::MUL:
                return SQLValue(lhs_v * rhs_v, null);

              default:
                throw std::runtime_error("Unreachable");
            }
          });
    }

    case plan::BinaryArithmeticOperatorType::DIV: {
      if constexpr (!std::is_same_v<V, Float64>) {
        throw std::runtime_error("Invalid arguments to div");
      }
      return NullableTernary<Float64>(
          program, lhs.IsNull() || rhs.IsNull(),
          [&]() { return SQLValue(Float64(program, 0), Bool(program, true)); },
          [&]() {
            const Float64& lhs_v = dynamic_cast<const Float64&>(lhs.Get());
            const Float64& rhs_v = dynamic_cast<const Float64&>(rhs.Get());
            Bool null(program, false);
            return SQLValue(lhs_v / rhs_v, null);
          });
    }

    case plan::BinaryArithmeticOperatorType::EQ:
    case plan::BinaryArithmeticOperatorType::NEQ:
    case plan::BinaryArithmeticOperatorType::LT:
    case plan::BinaryArithmeticOperatorType::LEQ:
    case plan::BinaryArithmeticOperatorType::GT:
    case plan::BinaryArithmeticOperatorType::GEQ: {
      return NullableTernary<Bool>(
          program, lhs.IsNull() || rhs.IsNull(),
          [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
          [&]() {
            const V& lhs_v = dynamic_cast<const V&>(lhs.Get());
            const V& rhs_v = dynamic_cast<const V&>(rhs.Get());
            Bool null(program, false);

            switch (op_type) {
              case plan::BinaryArithmeticOperatorType::EQ:
                return SQLValue(lhs_v == rhs_v, null);

              case plan::BinaryArithmeticOperatorType::NEQ:
                return SQLValue(lhs_v != rhs_v, null);

              case plan::BinaryArithmeticOperatorType::LT:
                return SQLValue(lhs_v < rhs_v, null);

              case plan::BinaryArithmeticOperatorType::LEQ:
                return SQLValue(lhs_v <= rhs_v, null);

              case plan::BinaryArithmeticOperatorType::GT:
                return SQLValue(lhs_v > rhs_v, null);

              case plan::BinaryArithmeticOperatorType::GEQ:
                return SQLValue(lhs_v >= rhs_v, null);

              default:
                throw std::runtime_error("Unreachable");
            }
          });
    }

    default:
      throw std::runtime_error("Invalid operator on numeric");
  }
}

SQLValue EvaluateBinaryString(plan::BinaryArithmeticOperatorType op_type,
                              const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  return NullableTernary<Bool>(
      program, lhs.IsNull() || rhs.IsNull(),
      [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
      [&]() {
        const String& lhs_v = dynamic_cast<const String&>(lhs.Get());
        const String& rhs_v = dynamic_cast<const String&>(rhs.Get());
        Bool null(program, false);

        switch (op_type) {
          case plan::BinaryArithmeticOperatorType::STARTS_WITH:
            return SQLValue(lhs_v.StartsWith(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::ENDS_WITH:
            return SQLValue(lhs_v.EndsWith(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::CONTAINS:
            return SQLValue(lhs_v.Contains(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::LIKE:
            return SQLValue(lhs_v.Like(rhs_v), null);

          case plan::BinaryArithmeticOperatorType::EQ:
            return SQLValue(lhs_v == rhs_v, null);

          case plan::BinaryArithmeticOperatorType::NEQ:
            return SQLValue(lhs_v != rhs_v, null);

          case plan::BinaryArithmeticOperatorType::LT:
            return SQLValue(lhs_v < rhs_v, null);

          default:
            throw std::runtime_error("Invalid operator on string");
        }
      });
}

SQLValue EvaluateBinary(plan::BinaryArithmeticOperatorType op,
                        const SQLValue& lhs, const SQLValue& rhs) {
  switch (lhs.Type()) {
    case catalog::SqlType::BOOLEAN:
      return EvaluateBinaryBool(op, lhs, rhs);
    case catalog::SqlType::SMALLINT:
      return EvaluateBinaryNumeric<Int16>(op, lhs, rhs);
    case catalog::SqlType::INT:
      return EvaluateBinaryNumeric<Int32>(op, lhs, rhs);
    case catalog::SqlType::DATE:
    case catalog::SqlType::BIGINT:
      return EvaluateBinaryNumeric<Int64>(op, lhs, rhs);
    case catalog::SqlType::REAL:
      return EvaluateBinaryNumeric<Float64>(op, lhs, rhs);
    case catalog::SqlType::TEXT:
      return EvaluateBinaryString(op, lhs, rhs);
  }
}

Bool LessThan(const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  auto lhs_null = lhs.IsNull();
  auto rhs_null = rhs.IsNull();

  return Ternary(
      program, lhs_null && rhs_null, [&]() { return Bool(program, false); },
      [&]() {
        return Ternary(
            program, lhs_null, [&]() { return Bool(program, false); },
            [&]() {
              return Ternary(
                  program, rhs_null, [&]() { return Bool(program, true); },
                  [&]() {
                    switch (lhs.Type()) {
                      case catalog::SqlType::SMALLINT: {
                        auto& lhs_v = static_cast<Int16&>(lhs.Get());
                        auto& rhs_v = static_cast<Int16&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::SqlType::INT: {
                        auto& lhs_v = static_cast<Int32&>(lhs.Get());
                        auto& rhs_v = static_cast<Int32&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::SqlType::DATE:
                      case catalog::SqlType::BIGINT: {
                        auto& lhs_v = static_cast<Int64&>(lhs.Get());
                        auto& rhs_v = static_cast<Int64&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::SqlType::REAL: {
                        auto& lhs_v = static_cast<Float64&>(lhs.Get());
                        auto& rhs_v = static_cast<Float64&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::SqlType::TEXT: {
                        auto& lhs_v = static_cast<String&>(lhs.Get());
                        auto& rhs_v = static_cast<String&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::SqlType::BOOLEAN:
                        throw std::runtime_error("Can't be less than.");
                    }
                  });
            });
      });
}

Bool Equal(const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  auto lhs_null = lhs.IsNull();
  auto rhs_null = rhs.IsNull();

  return Ternary(
      program, lhs_null && rhs_null, [&]() { return Bool(program, true); },
      [&]() {
        return Ternary(
            program, lhs_null || rhs_null,
            [&]() { return Bool(program, false); },
            [&]() {
              switch (lhs.Type()) {
                case catalog::SqlType::BOOLEAN: {
                  auto& lhs_v = static_cast<Bool&>(lhs.Get());
                  auto& rhs_v = static_cast<Bool&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::SqlType::SMALLINT: {
                  auto& lhs_v = static_cast<Int16&>(lhs.Get());
                  auto& rhs_v = static_cast<Int16&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::SqlType::INT: {
                  auto& lhs_v = static_cast<Int32&>(lhs.Get());
                  auto& rhs_v = static_cast<Int32&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::SqlType::DATE:
                case catalog::SqlType::BIGINT: {
                  auto& lhs_v = static_cast<Int64&>(lhs.Get());
                  auto& rhs_v = static_cast<Int64&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::SqlType::REAL: {
                  auto& lhs_v = static_cast<Float64&>(lhs.Get());
                  auto& rhs_v = static_cast<Float64&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::SqlType::TEXT: {
                  auto& lhs_v = static_cast<String&>(lhs.Get());
                  auto& rhs_v = static_cast<String&>(rhs.Get());
                  return lhs_v == rhs_v;
                }
              }
            });
      });
}

}  // namespace kush::compile::proxy