#pragma once
#include <functional>
#include <iostream>
#include <unordered_map>

namespace skinner {
namespace compile {

template <class ProduceFn, class ConsumeFn, class ConsumeExprFn>
class TranslatorRegistry {
  std::unordered_map<std::string, ProduceFn> id_to_produce;
  std::unordered_map<std::string, ConsumeFn> id_to_consume;
  std::unordered_map<std::string, ConsumeExprFn> id_to_expr_consume;

 public:
  void RegisterProducer(const std::string& id, ProduceFn produce) {
    id_to_produce[id] = produce;
  }

  void RegisterConsumer(const std::string& id, ConsumeFn consume) {
    id_to_consume[id] = consume;
  }

  void RegisterExprConsumer(const std::string& id, ConsumeExprFn consume) {
    id_to_expr_consume[id] = consume;
  }

  ProduceFn GetProducer(const std::string& id) const {
    return id_to_produce.at(id);
  }

  ConsumeFn GetConsumer(const std::string& id) const {
    return id_to_consume.at(id);
  }

  ConsumeExprFn GetExprConsumer(const std::string& id) const {
    return id_to_expr_consume.at(id);
  }
};

}  // namespace compile
}  // namespace skinner