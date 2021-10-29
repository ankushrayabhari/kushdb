#include <cstddef>
#include <cstring>
#include <iostream>

#include "benchmark/job/schema.h"
#include "catalog/catalog_manager.h"
#include "parse/parser.h"
#include "plan/planner.h"

using namespace kush;

int main(int argc, char** argv) {
  catalog::CatalogManager::Get().SetCurrent(Schema());

  std::string query((std::istreambuf_iterator<char>(std::cin)),
                    std::istreambuf_iterator<char>());
  auto parsed = parse::Parse(query);

  for (const auto& stmt : parsed) {
    plan::Planner planner;
    auto converted = planner.Plan(*stmt);
  }

  return 0;
}