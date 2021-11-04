#include <cstddef>
#include <cstring>
#include <iostream>

#include "benchmark/job/schema.h"
#include "catalog/catalog_manager.h"
#include "parse/parser.h"
#include "plan/planner.h"
#include "util/time_execute.h"

using namespace kush;

int main(int argc, char** argv) {
  catalog::CatalogManager::Get().SetCurrent(Schema());

  std::string query_text;
  if (argc == 2) {
    std::ifstream fin(argv[1]);
    query_text = std::string((std::istreambuf_iterator<char>(fin)),
                             std::istreambuf_iterator<char>());
  } else {
    query_text = std::string((std::istreambuf_iterator<char>(std::cin)),
                             std::istreambuf_iterator<char>());
  }

  auto parsed = parse::Parse(query_text);

  for (const auto& stmt : parsed) {
    plan::Planner planner;
    auto query = planner.Plan(*stmt);
    {
      kush::compile::QueryTranslator translator(*query);
      auto executable_query = translator.Translate();
      executable_query.Compile();
      executable_query.Execute();
    }
  }

  return 0;
}