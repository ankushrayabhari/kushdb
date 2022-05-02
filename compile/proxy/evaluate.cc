#include "compile/proxy/evaluate.h"

#include "catalog/sql_type.h"
#include "compile/proxy/control_flow/if.h"
#include "compile/proxy/value/ir_value.h"
#include "compile/proxy/value/sql_value.h"
#include "plan/expression/arithmetic_expression.h"

namespace kush::compile::proxy {

SQLValue EvaluateBinaryBool(plan::BinaryArithmeticExpressionType op_type,
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
          case plan::BinaryArithmeticExpressionType::AND:
            return SQLValue(lhs_bool && rhs_bool, null);

          case plan::BinaryArithmeticExpressionType::OR:
            return SQLValue(lhs_bool || rhs_bool, null);

          case plan::BinaryArithmeticExpressionType::EQ:
            return SQLValue(lhs_bool == rhs_bool, null);

          case plan::BinaryArithmeticExpressionType::NEQ:
            return SQLValue(lhs_bool != rhs_bool, null);

          default:
            throw std::runtime_error("Invalid operator on Bool");
        }
      });
}

SQLValue EvaluateBinaryDate(plan::BinaryArithmeticExpressionType op_type,
                            const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  switch (op_type) {
    case plan::BinaryArithmeticExpressionType::EQ:
    case plan::BinaryArithmeticExpressionType::NEQ:
    case plan::BinaryArithmeticExpressionType::LT:
    case plan::BinaryArithmeticExpressionType::LEQ:
    case plan::BinaryArithmeticExpressionType::GT:
    case plan::BinaryArithmeticExpressionType::GEQ: {
      return NullableTernary<Bool>(
          program, lhs.IsNull() || rhs.IsNull(),
          [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
          [&]() {
            const Date& lhs_v = dynamic_cast<const Date&>(lhs.Get());
            const Date& rhs_v = dynamic_cast<const Date&>(rhs.Get());
            Bool null(program, false);

            switch (op_type) {
              case plan::BinaryArithmeticExpressionType::EQ:
                return SQLValue(lhs_v == rhs_v, null);

              case plan::BinaryArithmeticExpressionType::NEQ:
                return SQLValue(lhs_v != rhs_v, null);

              case plan::BinaryArithmeticExpressionType::LT:
                return SQLValue(lhs_v < rhs_v, null);

              case plan::BinaryArithmeticExpressionType::LEQ:
                return SQLValue(lhs_v <= rhs_v, null);

              case plan::BinaryArithmeticExpressionType::GT:
                return SQLValue(lhs_v > rhs_v, null);

              case plan::BinaryArithmeticExpressionType::GEQ:
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

template <typename V>
SQLValue EvaluateBinaryNumeric(plan::BinaryArithmeticExpressionType op_type,
                               const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  switch (op_type) {
    case plan::BinaryArithmeticExpressionType::ADD:
    case plan::BinaryArithmeticExpressionType::SUB:
    case plan::BinaryArithmeticExpressionType::MUL: {
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
              case plan::BinaryArithmeticExpressionType::ADD:
                return SQLValue(lhs_v + rhs_v, null);

              case plan::BinaryArithmeticExpressionType::SUB:
                return SQLValue(lhs_v - rhs_v, null);

              case plan::BinaryArithmeticExpressionType::MUL:
                return SQLValue(lhs_v * rhs_v, null);

              default:
                throw std::runtime_error("Unreachable");
            }
          });
    }

    case plan::BinaryArithmeticExpressionType::DIV: {
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

    case plan::BinaryArithmeticExpressionType::EQ:
    case plan::BinaryArithmeticExpressionType::NEQ:
    case plan::BinaryArithmeticExpressionType::LT:
    case plan::BinaryArithmeticExpressionType::LEQ:
    case plan::BinaryArithmeticExpressionType::GT:
    case plan::BinaryArithmeticExpressionType::GEQ: {
      return NullableTernary<Bool>(
          program, lhs.IsNull() || rhs.IsNull(),
          [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
          [&]() {
            const V& lhs_v = dynamic_cast<const V&>(lhs.Get());
            const V& rhs_v = dynamic_cast<const V&>(rhs.Get());
            Bool null(program, false);

            switch (op_type) {
              case plan::BinaryArithmeticExpressionType::EQ:
                return SQLValue(lhs_v == rhs_v, null);

              case plan::BinaryArithmeticExpressionType::NEQ:
                return SQLValue(lhs_v != rhs_v, null);

              case plan::BinaryArithmeticExpressionType::LT:
                return SQLValue(lhs_v < rhs_v, null);

              case plan::BinaryArithmeticExpressionType::LEQ:
                return SQLValue(lhs_v <= rhs_v, null);

              case plan::BinaryArithmeticExpressionType::GT:
                return SQLValue(lhs_v > rhs_v, null);

              case plan::BinaryArithmeticExpressionType::GEQ:
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

SQLValue EvaluateBinaryEnum(plan::BinaryArithmeticExpressionType op_type,
                            const SQLValue& lhs, const SQLValue& rhs) {
  auto& program = lhs.ProgramBuilder();
  return NullableTernary<Bool>(
      program, lhs.IsNull() || rhs.IsNull(),
      [&]() { return SQLValue(Bool(program, false), Bool(program, true)); },
      [&]() {
        const Enum& lhs_v = dynamic_cast<const Enum&>(lhs.Get());
        Bool null(program, false);

        switch (op_type) {
          case plan::BinaryArithmeticExpressionType::EQ: {
            const Enum& rhs_v = dynamic_cast<const Enum&>(rhs.Get());
            return SQLValue(lhs_v == rhs_v, null);
          }

          case plan::BinaryArithmeticExpressionType::NEQ: {
            const Enum& rhs_v = dynamic_cast<const Enum&>(rhs.Get());
            return SQLValue(lhs_v != rhs_v, null);
          }

          case plan::BinaryArithmeticExpressionType::LT: {
            if (auto* rhs_v = dynamic_cast<const Enum*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() < rhs_v->ToString(), null);
            }

            if (auto* rhs_v = dynamic_cast<const String*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() < *rhs_v, null);
            }

            throw std::runtime_error("Invalid types for enum LT");
          }

          case plan::BinaryArithmeticExpressionType::LEQ: {
            if (auto* rhs_v = dynamic_cast<const Enum*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() <= rhs_v->ToString(), null);
            }

            if (auto* rhs_v = dynamic_cast<const String*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() <= *rhs_v, null);
            }

            throw std::runtime_error("Invalid types for enum LEQ");
          }

          case plan::BinaryArithmeticExpressionType::GT: {
            if (auto* rhs_v = dynamic_cast<const Enum*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() > rhs_v->ToString(), null);
            }

            if (auto* rhs_v = dynamic_cast<const String*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() > *rhs_v, null);
            }

            throw std::runtime_error("Invalid types for enum GT");
          }

          case plan::BinaryArithmeticExpressionType::GEQ: {
            if (auto* rhs_v = dynamic_cast<const Enum*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() >= rhs_v->ToString(), null);
            }

            if (auto* rhs_v = dynamic_cast<const String*>(&rhs.Get())) {
              return SQLValue(lhs_v.ToString() >= *rhs_v, null);
            }

            throw std::runtime_error("Invalid types for enum GEQ");
          }

          case plan::BinaryArithmeticExpressionType::STARTS_WITH: {
            const String& rhs_v = dynamic_cast<const String&>(rhs.Get());
            return SQLValue(lhs_v.ToString().StartsWith(rhs_v), null);
          }

          case plan::BinaryArithmeticExpressionType::ENDS_WITH: {
            const String& rhs_v = dynamic_cast<const String&>(rhs.Get());
            return SQLValue(lhs_v.ToString().EndsWith(rhs_v), null);
          }

          case plan::BinaryArithmeticExpressionType::CONTAINS: {
            const String& rhs_v = dynamic_cast<const String&>(rhs.Get());
            return SQLValue(lhs_v.ToString().Contains(rhs_v), null);
          }

          default:
            throw std::runtime_error("Invalid operator on enum");
        }
      });
}

SQLValue EvaluateBinaryString(plan::BinaryArithmeticExpressionType op_type,
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
          case plan::BinaryArithmeticExpressionType::STARTS_WITH:
            return SQLValue(lhs_v.StartsWith(rhs_v), null);

          case plan::BinaryArithmeticExpressionType::ENDS_WITH:
            return SQLValue(lhs_v.EndsWith(rhs_v), null);

          case plan::BinaryArithmeticExpressionType::CONTAINS:
            return SQLValue(lhs_v.Contains(rhs_v), null);

          case plan::BinaryArithmeticExpressionType::EQ:
            return SQLValue(lhs_v == rhs_v, null);

          case plan::BinaryArithmeticExpressionType::NEQ:
            return SQLValue(lhs_v != rhs_v, null);

          case plan::BinaryArithmeticExpressionType::LT:
            return SQLValue(lhs_v < rhs_v, null);

          case plan::BinaryArithmeticExpressionType::LEQ:
            return SQLValue(lhs_v <= rhs_v, null);

          case plan::BinaryArithmeticExpressionType::GT:
            return SQLValue(lhs_v > rhs_v, null);

          case plan::BinaryArithmeticExpressionType::GEQ:
            return SQLValue(lhs_v >= rhs_v, null);

          default:
            throw std::runtime_error("Invalid operator on string");
        }
      });
}

SQLValue EvaluateBinary(plan::BinaryArithmeticExpressionType op,
                        const SQLValue& lhs, const SQLValue& rhs) {
  switch (lhs.Type().type_id) {
    case catalog::TypeId::BOOLEAN:
      return EvaluateBinaryBool(op, lhs, rhs);
    case catalog::TypeId::SMALLINT:
      return EvaluateBinaryNumeric<Int16>(op, lhs, rhs);
    case catalog::TypeId::INT:
      return EvaluateBinaryNumeric<Int32>(op, lhs, rhs);
    case catalog::TypeId::DATE:
      return EvaluateBinaryDate(op, lhs, rhs);
    case catalog::TypeId::BIGINT:
      return EvaluateBinaryNumeric<Int64>(op, lhs, rhs);
    case catalog::TypeId::REAL:
      return EvaluateBinaryNumeric<Float64>(op, lhs, rhs);
    case catalog::TypeId::TEXT:
      return EvaluateBinaryString(op, lhs, rhs);
    case catalog::TypeId::ENUM:
      return EvaluateBinaryEnum(op, lhs, rhs);
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
                    switch (lhs.Type().type_id) {
                      case catalog::TypeId::SMALLINT: {
                        auto& lhs_v = static_cast<Int16&>(lhs.Get());
                        auto& rhs_v = static_cast<Int16&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::INT: {
                        auto& lhs_v = static_cast<Int32&>(lhs.Get());
                        auto& rhs_v = static_cast<Int32&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::DATE: {
                        auto& lhs_v = static_cast<Date&>(lhs.Get());
                        auto& rhs_v = static_cast<Date&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::BIGINT: {
                        auto& lhs_v = static_cast<Int64&>(lhs.Get());
                        auto& rhs_v = static_cast<Int64&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::REAL: {
                        auto& lhs_v = static_cast<Float64&>(lhs.Get());
                        auto& rhs_v = static_cast<Float64&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::TEXT: {
                        auto& lhs_v = static_cast<String&>(lhs.Get());
                        auto& rhs_v = static_cast<String&>(rhs.Get());
                        return lhs_v < rhs_v;
                      }

                      case catalog::TypeId::ENUM: {
                        auto& lhs_v = static_cast<Enum&>(lhs.Get());
                        auto& rhs_v = static_cast<Enum&>(rhs.Get());
                        return lhs_v.ToString() < rhs_v.ToString();
                      }

                      case catalog::TypeId::BOOLEAN:
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
              switch (lhs.Type().type_id) {
                case catalog::TypeId::BOOLEAN: {
                  auto& lhs_v = static_cast<Bool&>(lhs.Get());
                  auto& rhs_v = static_cast<Bool&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::SMALLINT: {
                  auto& lhs_v = static_cast<Int16&>(lhs.Get());
                  auto& rhs_v = static_cast<Int16&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::INT: {
                  auto& lhs_v = static_cast<Int32&>(lhs.Get());
                  auto& rhs_v = static_cast<Int32&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::DATE: {
                  auto& lhs_v = static_cast<Date&>(lhs.Get());
                  auto& rhs_v = static_cast<Date&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::BIGINT: {
                  auto& lhs_v = static_cast<Int64&>(lhs.Get());
                  auto& rhs_v = static_cast<Int64&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::REAL: {
                  auto& lhs_v = static_cast<Float64&>(lhs.Get());
                  auto& rhs_v = static_cast<Float64&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::TEXT: {
                  auto& lhs_v = static_cast<String&>(lhs.Get());
                  auto& rhs_v = static_cast<String&>(rhs.Get());
                  return lhs_v == rhs_v;
                }

                case catalog::TypeId::ENUM: {
                  auto& lhs_v = static_cast<Enum&>(lhs.Get());
                  auto& rhs_v = static_cast<Enum&>(rhs.Get());
                  return lhs_v == rhs_v;
                }
              }
            });
      });
}

}  // namespace kush::compile::proxy