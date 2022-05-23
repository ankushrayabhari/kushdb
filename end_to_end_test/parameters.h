#pragma once

#include <string>

struct ParameterValues {
  std::string pipeline_mode;
  std::string backend;
  std::string reg_alloc;
  std::string skinner;
  int32_t budget_per_episode = 0;
};

void SetFlags(const ParameterValues& params);