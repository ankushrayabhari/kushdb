#pragma once

#include "plan/operator.h"

namespace kush::compile {

template <typename T>
class Translator {
 public:
  virtual void Produce(T& op) = 0;
  virtual void Consume(T& op, plan::Operator& src) = 0;
};

}  // namespace kush::compile