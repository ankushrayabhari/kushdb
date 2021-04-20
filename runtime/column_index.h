#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "runtime/string.h"

namespace kush::data {

// Create the column index
std::unordered_map<int8_t, std::vector<int32_t>>* CreateInt8Index();
std::unordered_map<int16_t, std::vector<int32_t>>* CreateInt16Index();
std::unordered_map<int32_t, std::vector<int32_t>>* CreateInt32Index();
std::unordered_map<int64_t, std::vector<int32_t>>* CreateInt64Index();
std::unordered_map<double, std::vector<int32_t>>* CreateFloat64Index();
std::unordered_map<std::string, std::vector<int32_t>>* CreateStringIndex();

// Free the column index
void Free(std::unordered_map<int8_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int16_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int32_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int64_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<double, std::vector<int32_t>>* index);
void Free(std::unordered_map<std::string, std::vector<int32_t>>* index);

// Insert tuple idx
void Insert(std::unordered_map<int8_t, std::vector<int32_t>>* index,
            int8_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int16_t, std::vector<int32_t>>* index,
            int16_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int32_t, std::vector<int32_t>>* index,
            int32_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int64_t, std::vector<int32_t>>* index,
            int64_t value, int32_t tuple_idx);
void Insert(std::unordered_map<double, std::vector<int32_t>>* index,
            double value, int32_t tuple_idx);
void Insert(std::unordered_map<std::string, std::vector<int32_t>>* index,
            String* value, int32_t tuple_idx);

// Get the next tuple from index
int32_t GetNextTuple(std::unordered_map<int8_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int16_t, std::vector<int32_t>>* index,
                     int16_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int32_t, std::vector<int32_t>>* index,
                     int32_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int64_t, std::vector<int32_t>>* index,
                     int64_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<double, std::vector<int32_t>>* index,
                     double value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(
    std::unordered_map<std::string, std::vector<int32_t>>* index, String* value,
    int32_t prev_tuple, int32_t cardinality);

}  // namespace kush::data