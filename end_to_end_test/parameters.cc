#include "end_to_end_test/parameters.h"

#include <string>

#include "absl/flags/flag.h"

ABSL_DECLARE_FLAG(std::string, backend);
ABSL_DECLARE_FLAG(std::string, reg_alloc);
ABSL_DECLARE_FLAG(std::string, skinner_join);
ABSL_DECLARE_FLAG(std::string, skinner_scan_select);
ABSL_DECLARE_FLAG(int32_t, budget_per_episode);
ABSL_DECLARE_FLAG(int64_t, scan_select_seed);
ABSL_DECLARE_FLAG(int32_t, scan_select_budget_per_episode);

void SetFlags(const ParameterValues& params) {
  if (!params.backend.empty()) {
    absl::SetFlag(&FLAGS_backend, params.backend);
  }

  if (!params.reg_alloc.empty()) {
    absl::SetFlag(&FLAGS_reg_alloc, params.reg_alloc);
  }

  if (!params.skinner.empty()) {
    absl::SetFlag(&FLAGS_skinner_join, params.skinner);
  }

  if (!params.skinner_scan_select.empty()) {
    absl::SetFlag(&FLAGS_skinner_scan_select, params.skinner_scan_select);
  }

  if (params.budget_per_episode > 0) {
    absl::SetFlag(&FLAGS_budget_per_episode, params.budget_per_episode);
  }

  if (params.scan_select_seed > 0) {
    absl::SetFlag(&FLAGS_scan_select_seed, params.scan_select_seed);
  }

  if (params.budget_per_episode > 0) {
    absl::SetFlag(&FLAGS_budget_per_episode, params.budget_per_episode);
  }
}