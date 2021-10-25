#include <cstddef>
#include <cstring>
#include <iostream>

#include "parse/parser.h"

int main(int argc, char** argv) {
  std::string_view query(argv[1]);
  auto parsed = kush::parse::Parse(query);
  return 0;
}