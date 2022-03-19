#include "end_to_end_test/parameters.h"

#include <string>

#include "absl/flags/flag.h"

ABSL_DECLARE_FLAG(std::string, backend);
ABSL_DECLARE_FLAG(std::string, reg_alloc);
ABSL_DECLARE_FLAG(std::string, skinner);
ABSL_DECLARE_FLAG(int32_t, budget_per_episode);
ABSL_DECLARE_FLAG(int64_t, scan_select_seed);

void SetFlags(const ParameterValues& params) {
  if (!params.backend.empty()) {
    absl::SetFlag(&FLAGS_backend, params.backend);
  }

  if (!params.reg_alloc.empty()) {
    absl::SetFlag(&FLAGS_reg_alloc, params.reg_alloc);
  }

  if (!params.skinner.empty()) {
    absl::SetFlag(&FLAGS_skinner, params.skinner);
  }

  if (params.budget_per_episode > 0) {
    absl::SetFlag(&FLAGS_budget_per_episode, params.budget_per_episode);
  }

  if (params.scan_select_seed > 0) {
    absl::SetFlag(&FLAGS_scan_select_seed, params.scan_select_seed);
  }
}