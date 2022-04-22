#include "runtime/date.h"

#include <cstdint>

namespace kush::runtime::Date {

DateBuilder::DateBuilder(int32_t y, int32_t m, int32_t d)
    : year(y), month(m), day(d) {}

uint32_t BuildDate(uint32_t year, uint32_t month, uint32_t day) {
  if (month > 2) {
    month += 1;
    year += 4800;
  } else {
    month += 13;
    year += 4799;
  }

  int32_t century = year / 100;
  int32_t julian = year * 365 - 32167;
  julian += year / 4 - century + century / 4;
  julian += 7834 * month / 256 + day;

  return julian;
}

uint32_t DateBuilder::Build() const { return BuildDate(year, month, day); }

void SplitDate(int32_t jd, int32_t *year, int32_t *month, int32_t *day) {
  uint32_t julian = jd;
  julian += 32044;
  uint32_t quad = julian / 146097;
  uint32_t extra = (julian - quad * 146097) * 4 + 3;
  julian += 60 + quad * 3 + extra / 146097;
  quad = julian / 1461;
  julian -= quad * 1461;
  int32_t y = julian * 4 / 1461;
  julian = ((y != 0) ? ((julian + 305) % 365) : ((julian + 306) % 366)) + 123;
  y += quad * 4;
  *year = y - 4800;
  quad = julian * 2141 / 65536;
  *day = julian - 7834 * quad / 256;
  *month = (quad + 10) % 12 + 1;
}

int32_t ExtractYear(int32_t value) {
  int32_t year, month, day;
  SplitDate(value, &year, &month, &day);
  return year;
}

}  // namespace kush::runtime::Date