#include <cstddef>
#include <cstring>
#include <iostream>

#include "parse/parser.h"
#include "plan/planner.h"

int main(int argc, char** argv) {
  std::string query((std::istreambuf_iterator<char>(std::cin)),
                    std::istreambuf_iterator<char>());
  auto parsed = kush::parse::Parse(query);

  for (const auto& stmt : parsed) {
    kush::plan::Planner planner;
    auto converted = planner.Plan(*stmt);
  }

  return 0;
}