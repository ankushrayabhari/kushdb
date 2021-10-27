#include <cstddef>
#include <cstring>
#include <iostream>

#include "parse/parser.h"

int main(int argc, char** argv) {
  std::string query((std::istreambuf_iterator<char>(std::cin)),
                    std::istreambuf_iterator<char>());
  auto parsed = kush::parse::Parse(query);
  return 0;
}