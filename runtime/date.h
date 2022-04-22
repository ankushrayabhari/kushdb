#pragma once

#include <cstdint>

namespace kush::runtime::Date {

class DateBuilder {
 public:
  int32_t year;
  int32_t month;
  int32_t day;

  DateBuilder(int32_t y, int32_t m, int32_t d);
  uint32_t Build() const;
};

void SplitDate(int32_t jd, int32_t *year, int32_t *month, int32_t *day);
int32_t ExtractYear(int32_t value);

}  // namespace kush::runtime::Date