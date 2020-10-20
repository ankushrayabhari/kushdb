#include "compilation/translator_registry.h"

#include <functional>
#include <iostream>
#include <unordered_map>

#include "algebra/operator.h"

namespace skinner {
namespace compile {

void TranslatorRegistry::RegisterProducer(const std::string& id,
                                          ProduceFn produce) {
  id_to_produce[id] = produce;
}

void TranslatorRegistry::RegisterExprConsumer(const std::string& id,
                                              ConsumeExprFn consume) {
  id_to_expr_consume[id] = consume;
}

void TranslatorRegistry::RegisterConsumer(const std::string& id,
                                          ConsumeFn consume) {
  id_to_consume[id] = consume;
}

ProduceFn TranslatorRegistry::GetProducer(const std::string& id) const {
  return id_to_produce.at(id);
}

ConsumeFn TranslatorRegistry::GetConsumer(const std::string& id) const {
  return id_to_consume.at(id);
}

ConsumeExprFn TranslatorRegistry::GetExprConsumer(const std::string& id) const {
  return id_to_expr_consume.at(id);
}

}  // namespace compile
}  // namespace skinner