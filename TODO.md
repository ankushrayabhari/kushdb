Needed for TPC-H:
- Dependent queries
- Extract from date (q7-9)

Update intermediate results:
- Something better than unordered_map
- Clear out intermediate buffers after use

Broadly, generate LLVM/MLIR instead of C++.