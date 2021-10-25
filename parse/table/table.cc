#include "parse/table/table.h"

#include <memory>
#include <vector>

namespace kush::parse {

BaseTable::BaseTable(std::string_view name, std::string_view alias)
    : name_(name), alias_(alias) {}

CrossProductTable::CrossProductTable(
    std::vector<std::unique_ptr<Table>> children)
    : children_(std::move(children)) {}

}  // namespace kush::parse