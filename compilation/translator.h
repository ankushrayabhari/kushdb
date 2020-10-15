#pragma once
#include "algebra/operator.h"

namespace skinner {

class Translator {
 public:
  virtual void Produce(Operator& op) = 0;
};

}  // namespace skinner