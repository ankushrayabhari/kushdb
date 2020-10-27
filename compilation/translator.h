#pragma once
#include "physical/operator.h"

namespace kush {
namespace compile {

class Translator {
 public:
  virtual void Produce(physical::Operator& op) = 0;
};

}  // namespace compile
}  // namespace kush