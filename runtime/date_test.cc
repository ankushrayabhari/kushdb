
#include "runtime/date.h"

#include "gtest/gtest.h"

using namespace kush::runtime::Date;

TEST(DateTest, SerializeParse) {
  int32_t year = 2022;
  int32_t month = 2;
  int32_t day = 28;
  int32_t value = DateBuilder(year, month, day).Build();

  int32_t y, m, d;
  SplitDate(value, &y, &m, &d);
  EXPECT_EQ(year, y);
  EXPECT_EQ(month, m);
  EXPECT_EQ(day, d);
}

TEST(DateTest, Ordering) {
  int32_t value1 = DateBuilder(1, 1, 1).Build();
  int32_t value2 = DateBuilder(2022, 5, 28).Build();
  EXPECT_LT(value1, value2);
}

TEST(DateTest, YearExtract) {
  int32_t value1 = DateBuilder(1, 1, 1).Build();
  EXPECT_EQ(ExtractYear(value1), 1);
}