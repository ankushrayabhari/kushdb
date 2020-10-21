#pragma once
#include "algebra/operator.h"

namespace kush {
namespace compile {

class Translator {
 public:
  virtual void Produce(algebra::Operator& op) = 0;
};

}  // namespace compile
}  // namespace kush