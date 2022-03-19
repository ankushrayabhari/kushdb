#pragma once

#include <string>

struct ParameterValues {
  std::string backend;
  std::string reg_alloc;
  std::string skinner;
  int32_t budget_per_episode = 0;
  int64_t scan_select_seed = 0;
  int32_t scan_select_budget_per_episode = 0;
};

void SetFlags(const ParameterValues& params);