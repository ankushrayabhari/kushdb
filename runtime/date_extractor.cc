#include "runtime/date_extractor.h"

#include <cstdint>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

namespace kush::runtime::DateExtractor {

int32_t ExtractYear(int64_t date) {
  auto time = absl::FromUnixMillis(date);
  auto utc = absl::UTCTimeZone();
  auto day = absl::ToCivilDay(time, utc);
  return day.year();
}

}  // namespace kush::runtime::DateExtractor