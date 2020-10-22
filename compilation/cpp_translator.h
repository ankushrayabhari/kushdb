#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "compilation/translator.h"
#include "compilation/translator_registry.h"

namespace kush {
namespace compile {

class CppTranslator {
 public:
  struct Column {
    std::string name;
    std::string type;

    Column(const std::string& n, const std::string& t) : name(n), type(t) {}
  };

  class CompilationContext;

  typedef std::function<void(CompilationContext&, algebra::Operator&,
                             std::ostream&)>
      ProduceFn;
  typedef std::function<void(CompilationContext&, algebra::Operator&,
                             algebra::Operator&, std::vector<Column>,
                             std::ostream&)>
      ConsumeFn;
  typedef std::function<void(CompilationContext&, algebra::Expression&,
                             std::ostream&)>
      ConsumeExprFn;

  class CompilationContext {
   public:
    const catalog::Database& database;
    CompilationContext(const catalog::Database& db) : database(db) {}
    TranslatorRegistry<ProduceFn, ConsumeFn, ConsumeExprFn> registry;
    std::unordered_map<std::string, std::string> col_to_var;
  };

 private:
  CompilationContext context_;

 public:
  CppTranslator(const catalog::Database& db);
  void Produce(algebra::Operator& op);
};

}  // namespace compile
}  // namespace kush
