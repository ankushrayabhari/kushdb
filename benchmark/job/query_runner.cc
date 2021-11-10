#include <cstddef>
#include <cstring>
#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include "benchmark/job/schema.h"
#include "catalog/catalog_manager.h"
#include "parse/parser.h"
#include "plan/planner.h"
#include "util/time_execute.h"

using namespace kush;

ABSL_FLAG(std::string, query, "query", "Query");

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("Executes query.");
  absl::ParseCommandLine(argc, argv);

  catalog::CatalogManager::Get().SetCurrent(Schema());

  std::string query_path = FLAGS_query.Get();
  if (query_path.empty()) {
    throw std::runtime_error("Invalid query path.");
  }
  std::ifstream fin(query_path);
  auto query_text = std::string((std::istreambuf_iterator<char>(fin)),
                                std::istreambuf_iterator<char>());

  auto parsed = parse::Parse(query_text);

  for (const auto& stmt : parsed) {
    plan::Planner planner;
    auto query = planner.Plan(*stmt);
    util::TimeExecute(*query);
  }

  return 0;
}