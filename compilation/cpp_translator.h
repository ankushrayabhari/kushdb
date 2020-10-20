#pragma once
#include "compilation/translator.h"
#include "compilation/translator_registry.h"

namespace skinner {
namespace compile {

class CppTranslator {
 public:
  class CompilationContext;

  typedef std::function<void(CompilationContext&, algebra::Operator&,
                             std::ostream&)>
      ProduceFn;
  typedef std::function<void(CompilationContext&, algebra::Operator&,
                             algebra::Operator&, std::ostream&)>
      ConsumeFn;
  typedef std::function<void(CompilationContext&, algebra::Expression&,
                             std::ostream&)>
      ConsumeExprFn;

  class CompilationContext {
   public:
    TranslatorRegistry<ProduceFn, ConsumeFn, ConsumeExprFn> registry;
    std::unordered_map<std::string, std::string> col_to_var;
  };

 private:
  CompilationContext context_;

 public:
  CppTranslator();
  void Produce(algebra::Operator& op);
};

}  // namespace compile
}  // namespace skinner
