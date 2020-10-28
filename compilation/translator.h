#pragma once
#include "plan/operator.h"

namespace kush {
namespace compile {

class Translator {
 public:
  virtual void Produce(plan::Operator& op) = 0;
};

}  // namespace compile
}  // namespace kush