#pragma once

#include "HashTable.h"
#include <random>
#include <unordered_map>
#include <gtest/gtest.h>

TEST(ConstructorInt, BucketCount)
{
	using namespace ss;
	size_t bucket_count = 1 << 10;
	HashTable<int, int> ht(bucket_count);
	EXPECT_EQ(bucket_count, ht.bucket_count());
}
