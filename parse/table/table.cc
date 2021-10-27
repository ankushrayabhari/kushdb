#include "parse/table/table.h"

#include <memory>
#include <vector>

#include "util/vector_util.h"

namespace kush::parse {

BaseTable::BaseTable(std::string_view name, std::string_view alias)
    : name_(name), alias_(alias) {}

std::string_view BaseTable::Name() const { return name_; }

std::string_view BaseTable::Alias() const { return alias_; }

CrossProductTable::CrossProductTable(
    std::vector<std::unique_ptr<Table>> children)
    : children_(std::move(children)) {}

std::vector<std::reference_wrapper<const Table>> CrossProductTable::Children()
    const {
  return util::ImmutableReferenceVector(children_);
}

}  // namespace kush::parse