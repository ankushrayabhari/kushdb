#include "compilation/translators/scan_translator.h"

#include "compilation/cpp_translator.h"
#include "compilation/translators/operator_translator.h"
#include "plan/operator.h"

namespace kush::compile {

ScanTranslator::ScanTranslator(
    plan::Scan& scan, CppTranslator& context,
    std::vector<std::unique_ptr<OperatorTranslator>> children)
    : OperatorTranslator(std::move(children)), scan_(scan), context_(context) {}

void ScanTranslator::Produce() {}

void ScanTranslator::Consume(OperatorTranslator& src) {}

}  // namespace kush::compile