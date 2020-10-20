#pragma once
#include "compilation/translator.h"
#include "compilation/translator_registry.h"

namespace skinner {
namespace compile {

class CppTranslator {
  TranslatorRegistry registry_;

 public:
  CppTranslator();
  void Produce(algebra::Operator& op);
};

}  // namespace compile
}  // namespace skinner
