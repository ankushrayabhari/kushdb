#include "compilation/translators/translator.h"

namespace kush::compile {

Translator::Translator(std::vector<std::unique_ptr<Translator>> children)
    : parent_(nullptr), children_(std::move(children)) {
  for (auto& child : children_) {
    child->SetParent(*this);
  }
}

void Translator::SetParent(Translator& translator) { parent_ = &translator; }

}  // namespace kush::compile