#pragma once
#include "algebra/operator.h"

namespace skinner {

class Translator {
 public:
  virtual void Produce(algebra::Operator& op) = 0;
};

}  // namespace skinner