#pragma once

#include <memory>
#include <vector>

namespace kush::compile {

class Translator {
 public:
  Translator(std::vector<std::unique_ptr<Translator>> children);
  virtual void Produce() = 0;
  virtual void Consume(Translator& src) = 0;
  void SetParent(Translator& parent);

 private:
  Translator* parent_;
  std::vector<std::unique_ptr<Translator>> children_;
};

}  // namespace kush::compile