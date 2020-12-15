#include "compilation/translators/scan_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/translator.h"
#include "plan/operator.h"

namespace kush::compile {

ScanTranslator::ScanTranslator(
    plan::Scan& scan, CppTranslator& context,
    std::vector<std::unique_ptr<Translator>> children)
    : Translator(std::move(children)), scan_(scan), context_(context) {}

void ScanTranslator::Produce() {}

void ScanTranslator::Consume(Translator& src) {}

}  // namespace kush::compile