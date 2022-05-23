#pragma once

#define SKINNER_TEST(TestSuite)                                                \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_StackSpill_Recompile_HighBudget, TestSuite,                   \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "stack_spill",              \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_StackSpill_Permute_HighBudget, TestSuite,                     \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "stack_spill",              \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_StackSpill_Recompile_LowBudget, TestSuite,                    \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "stack_spill",              \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_StackSpill_Permute_LowBudget, TestSuite,                      \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "stack_spill",              \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Recompile_HighBudget, TestSuite,                   \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Permute_HighBudget, TestSuite,                     \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Recompile_LowBudget, TestSuite,                    \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Permute_LowBudget, TestSuite,                      \
      testing::Values(ParameterValues{.backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Recompile_HighBudget, TestSuite,                             \
      testing::Values(ParameterValues{.backend = "llvm",                       \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Permute_HighBudget, TestSuite,                               \
      testing::Values(ParameterValues{.backend = "llvm",                       \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Recompile_LowBudget, TestSuite,                              \
      testing::Values(ParameterValues{.backend = "llvm",                       \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Permute_LowBudget, TestSuite,                                \
      testing::Values(ParameterValues{.backend = "llvm",                       \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      Hybrid_LowBudget, TestSuite,                                             \
      testing::Values(                                                         \
          ParameterValues{.skinner = "hybrid", .budget_per_episode = 10}));    \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      Hybrid_HighBudget, TestSuite,                                            \
      testing::Values(                                                         \
          ParameterValues{.skinner = "hybrid", .budget_per_episode = 10000})); \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Permute_HighBudget_Adaptive, TestSuite,            \
      testing::Values(ParameterValues{.pipeline_mode = "adaptive",             \
                                      .backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      ASMBackend_LinearScan_Recompile_LowBudget_Adaptive, TestSuite,           \
      testing::Values(ParameterValues{.pipeline_mode = "adaptive",             \
                                      .backend = "asm",                        \
                                      .reg_alloc = "linear_scan",              \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10}));             \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Recompile_HighBudget_Adaptive, TestSuite,                    \
      testing::Values(ParameterValues{.pipeline_mode = "adaptive",             \
                                      .backend = "llvm",                       \
                                      .skinner = "recompile",                  \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      LLVMBackend_Permute_HighBudget_Adaptive, TestSuite,                      \
      testing::Values(ParameterValues{.pipeline_mode = "adaptive",             \
                                      .backend = "llvm",                       \
                                      .skinner = "permute",                    \
                                      .budget_per_episode = 10000}));          \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      Hybrid_HighBudget_Adaptive, TestSuite,                                   \
      testing::Values(ParameterValues{.pipeline_mode = "adaptive",             \
                                      .skinner = "hybrid",                     \
                                      .budget_per_episode = 10000}));