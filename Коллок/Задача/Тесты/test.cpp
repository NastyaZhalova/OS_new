
#include <gtest/gtest.h>

#include "Factorial.h"
#include "Unique.h"
#include "LinkedList.h"


TEST(FactorialTests, BasicValues) {
    auto v = math::Factorial::generate(5);

    ASSERT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 1ull);
    EXPECT_EQ(v[1], 2ull);
    EXPECT_EQ(v[2], 6ull);
    EXPECT_EQ(v[3], 24ull);
    EXPECT_EQ(v[4], 120ull);
}

TEST(FactorialTests, InvalidArgument) {
    EXPECT_THROW(math::Factorial::generate(0), std::invalid_argument);
}

TEST(FactorialTests, Overflow) {
    EXPECT_THROW(math::Factorial::generate(21), std::overflow_error);
}


TEST(UniqueTests, Basic) {
    std::vector<int> v{ 1,2,2,3,1,4,3 };
    auto out = util::removeDuplicates(v);

    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 1);
    EXPECT_EQ(out[1], 2);
    EXPECT_EQ(out[2], 3);
    EXPECT_EQ(out[3], 4);
}

TEST(UniqueTests, Empty) {
    std::vector<int> v;
    auto out = util::removeDuplicates(v);

    EXPECT_TRUE(out.empty());
}

TEST(UniqueTests, Single) {
    std::vector<int> v{ 42 };
    auto out = util::removeDuplicates(v);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 42);
}


TEST(LinkedListTests, ReverseBasic) {
    ds::LinkedList list{ 1,2,3,4 };
    list.reverseRecursive();

    auto v = list.toVector();

    ASSERT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 1);
}

TEST(LinkedListTests, ReverseSingle) {
    ds::LinkedList list{ 10 };
    list.reverseRecursive();

    auto v = list.toVector();

    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 10);
}

TEST(LinkedListTests, ReverseEmpty) {
    ds::LinkedList list;
    list.reverseRecursive();

    auto v = list.toVector();

    EXPECT_TRUE(v.empty());
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
