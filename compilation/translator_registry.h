#pragma once
#include <functional>
#include <iostream>
#include <unordered_map>

#include "algebra/operator.h"

namespace skinner {
namespace compile {

class TranslatorRegistry;

using ProduceFn = std::function<void(const TranslatorRegistry&,
                                     algebra::Operator&, std::ostream&)>;
using ConsumeFn =
    std::function<void(const TranslatorRegistry&, algebra::Operator&,
                       algebra::Operator&, std::ostream&)>;

using ConsumeExprFn = std::function<void(const TranslatorRegistry&,
                                         algebra::Expression&, std::ostream&)>;

class TranslatorRegistry {
  std::unordered_map<std::string, ProduceFn> id_to_produce;
  std::unordered_map<std::string, ConsumeFn> id_to_consume;
  std::unordered_map<std::string, ConsumeExprFn> id_to_expr_consume;

 public:
  void RegisterProducer(const std::string& id, ProduceFn produce);
  void RegisterConsumer(const std::string& id, ConsumeFn consume);
  void RegisterExprConsumer(const std::string& id, ConsumeExprFn consume);
  ProduceFn GetProducer(const std::string& id) const;
  ConsumeFn GetConsumer(const std::string& id) const;
  ConsumeExprFn GetExprConsumer(const std::string& id) const;
};

}  // namespace compile
}  // namespace skinner