#pragma once
#include <functional>
#include <iostream>
#include <unordered_map>

#include "algebra/operator.h"

namespace skinner {

class TranslatorRegistry;

using ProduceFn =
    std::function<void(const TranslatorRegistry&, Operator&, std::ostream&)>;
using ConsumeFn = std::function<void(const TranslatorRegistry&, Operator&,
                                     Operator&, std::ostream&)>;

class TranslatorRegistry {
  std::unordered_map<std::string, ProduceFn> id_to_produce;
  std::unordered_map<std::string, ConsumeFn> id_to_consume;

 public:
  void RegisterProducer(const std::string& id, ProduceFn produce);
  void RegisterConsumer(const std::string& id, ConsumeFn consume);
  ProduceFn GetProducer(const std::string& id) const;
  ConsumeFn GetConsumer(const std::string& id) const;
};

}  // namespace skinner