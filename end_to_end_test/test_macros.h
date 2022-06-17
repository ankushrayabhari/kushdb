#pragma once

#define SKINNER_TEST(TestSuite)                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_HighBudget,         \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "stack_spill",                     \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_HighBudget,           \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "stack_spill",                     \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Recompile_LowBudget,          \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "stack_spill",                     \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Permute_LowBudget, TestSuite, \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "stack_spill",                     \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_HighBudget,         \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_HighBudget,           \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_LowBudget,          \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_LowBudget, TestSuite, \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Recompile_HighBudget, TestSuite,        \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "llvm",                              \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Permute_HighBudget, TestSuite,          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "llvm",                              \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Recompile_LowBudget, TestSuite,         \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "llvm",                              \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Permute_LowBudget, TestSuite,           \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .backend = "llvm",                              \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(Hybrid_LowBudget, TestSuite,                        \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .skinner = "hybrid",                            \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(Hybrid_HighBudget, TestSuite,                       \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "static",                      \
                               .skinner = "hybrid",                            \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Permute_HighBudget_Adaptive,  \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "adaptive",                    \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Recompile_LowBudget_Adaptive, \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "adaptive",                    \
                               .backend = "asm",                               \
                               .reg_alloc = "linear_scan",                     \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10,                       \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Recompile_HighBudget_Adaptive,          \
                           TestSuite,                                          \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "adaptive",                    \
                               .backend = "llvm",                              \
                               .skinner = "recompile",                         \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Permute_HighBudget_Adaptive, TestSuite, \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "adaptive",                    \
                               .backend = "llvm",                              \
                               .skinner = "permute",                           \
                               .budget_per_episode = 10000,                    \
                           }));                                                \
  INSTANTIATE_TEST_SUITE_P(Hybrid_HighBudget_Adaptive, TestSuite,              \
                           testing::Values(ParameterValues{                    \
                               .pipeline_mode = "adaptive",                    \
                               .skinner = "hybrid",                            \
                               .budget_per_episode = 10000,                    \
                           }));

#define NORMAL_TEST(TestSuite)                                        \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill, TestSuite,          \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "static",             \
                               .backend = "asm",                      \
                               .reg_alloc = "stack_spill",            \
                           }));                                       \
                                                                      \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan, TestSuite,          \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "static",             \
                               .backend = "asm",                      \
                               .reg_alloc = "linear_scan",            \
                           }));                                       \
                                                                      \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend, TestSuite,                    \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "static",             \
                               .backend = "llvm",                     \
                           }));                                       \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_StackSpill_Adaptive, TestSuite, \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "adaptive",           \
                               .backend = "asm",                      \
                               .reg_alloc = "stack_spill",            \
                           }));                                       \
                                                                      \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_LinearScan_Adaptive, TestSuite, \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "adaptive",           \
                               .backend = "asm",                      \
                               .reg_alloc = "linear_scan",            \
                           }));                                       \
                                                                      \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Adaptive, TestSuite,           \
                           testing::Values(ParameterValues{           \
                               .pipeline_mode = "adaptive",           \
                               .backend = "llvm",                     \
                           }));

#define ORDER_TEST(TestSuite)                                                \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_StackSpill, OrderByTest,           \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "asm",                             \
                               .reg_alloc = "stack_spill",                   \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_StackSpill, OrderByTest,          \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "asm",                             \
                               .reg_alloc = "stack_spill",                   \
                               .asc = false,                                 \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_LinearScan, OrderByTest,           \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "asm",                             \
                               .reg_alloc = "linear_scan",                   \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_LinearScan, OrderByTest,          \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "asm",                             \
                               .reg_alloc = "linear_scan",                   \
                               .asc = false,                                 \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Asc, OrderByTest,                     \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "llvm",                            \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Desc, OrderByTest,                    \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "static",                    \
                               .backend = "llvm",                            \
                               .asc = false,                                 \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_StackSpill_Adaptive, OrderByTest,  \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "asm",                             \
                               .reg_alloc = "stack_spill",                   \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_StackSpill_Adaptive, OrderByTest, \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "asm",                             \
                               .reg_alloc = "stack_spill",                   \
                               .asc = false,                                 \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Asc_LinearScan_Adaptive, OrderByTest,  \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "asm",                             \
                               .reg_alloc = "linear_scan",                   \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(ASMBackend_Desc_LinearScan_Adaptive, OrderByTest, \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "asm",                             \
                               .reg_alloc = "linear_scan",                   \
                               .asc = false,                                 \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Asc_Adaptive, OrderByTest,            \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "llvm",                            \
                               .asc = true,                                  \
                           }));                                              \
                                                                             \
  INSTANTIATE_TEST_SUITE_P(LLVMBackend_Desc_Adaptive, OrderByTest,           \
                           testing::Values(ParameterValues{                  \
                               .pipeline_mode = "adaptive",                  \
                               .backend = "llvm",                            \
                               .asc = false,                                 \
                           }));