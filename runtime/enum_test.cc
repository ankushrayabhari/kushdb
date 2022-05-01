
#include "runtime/enum.h"

#include "gtest/gtest.h"

using namespace kush::runtime::Enum;

TEST(EnumTest, asdf) {
  std::unordered_map<std::string, int32_t> map;
  map["a"] = 0;
  map["asdf"] = 1;
  map["qwer"] = 2;
  map["zxcv"] = 3;
  map["b"] = 4;
  map["c"] = 5;

  Serialize("/tmp/asdf.kdbenum", map);

  auto id = EnumManager::Get().Register("/tmp/asdf.kdbenum");

  EXPECT_EQ(GetValue(id, "a"), 0);
  EXPECT_EQ(GetValue(id, "asdf"), 1);
  EXPECT_EQ(GetValue(id, "qwer"), 2);
  EXPECT_EQ(GetValue(id, "zxcv"), 3);
  EXPECT_EQ(GetValue(id, "b"), 4);
  EXPECT_EQ(GetValue(id, "c"), 5);
  EXPECT_EQ(GetValue(id, "123"), -1);
  EXPECT_EQ(GetValue(id, "qwerty"), -1);

  kush::runtime::String::String s;
  GetKey(id, 0, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "a");
  GetKey(id, 1, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "asdf");
  GetKey(id, 2, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "qwer");
  GetKey(id, 3, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "zxcv");
  GetKey(id, 4, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "b");
  GetKey(id, 5, &s);
  EXPECT_EQ(std::string_view(s.data, s.length), "c");
}
