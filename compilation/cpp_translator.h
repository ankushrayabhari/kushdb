#pragma once
#include "compilation/translator.h"
#include "compilation/translator_registry.h"

namespace skinner {

class CppTranslator {
  TranslatorRegistry registry_;

 public:
  CppTranslator();
  void Produce(Operator& op);
};

}  // namespace skinner
