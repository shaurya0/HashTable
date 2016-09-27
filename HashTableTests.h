#pragma once
#include <array>
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


//TEST(ConstructorInt, IteratorRangeSmall)
//{
//    using namespace ss;
//    constexpr size_t N = 1 << 3;
//    std::unordered_map<int, int> items;
//
//    for (int i = 0; i < N; ++i)
//    {
//        items[i] = i;
//    }
//
//    HashTable<int, int> ht(items.begin(), items.end());
//    for( auto kv = ht.begin(); kv != ht.end(); ++kv)
//    {
//        // int value_ref = items.at(kv.first);
//        // int value = ht.at( kv.first );
//        std::cout << kv->first << ", " << kv->second << std::endl;
//    }
//
//    EXPECT_EQ(1, 1);
//}

 TEST(ConstructorInt, IteratorRange)
 {
     using namespace ss;
     constexpr size_t N = 1 << 10;
     std::unordered_map<int, int> items;

     for (int i = 0; i < N; ++i)
     {
         items[i] = i * 3;
     }

     HashTable<int, int> ht(items.begin(), items.end());
     for( const auto &kv : ht )
     {
         int value_ref = items.at(kv.first);
         int value = ht.at( kv.first );

         EXPECT_EQ( value, value_ref );
     }

     EXPECT_EQ(N, ht.size());
 }

TEST(ConstructorInt, ConstRefOther)
{
    using namespace ss;
    constexpr size_t N = 1 << 10;
    std::unordered_map<int, int> items;

    for (int i = 0; i < N; ++i)
    {
        items[i] = i * 3;
    }

    HashTable<int, int> ht1(items.begin(), items.end());
    HashTable<int, int> ht( ht1 );
    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }

    EXPECT_EQ(N, ht.size());
}

TEST(ConstructorInt, RvalueOther)
{
    using namespace ss;
    constexpr size_t N = 1 << 10;
	std::unordered_map<int, int> items;

    for (int i = 0; i < N; ++i)
    {
		items[i] = i * 3;
    }

    HashTable<int, int> ht(HashTable<int, int>(items.begin(), items.end()));
    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }

    EXPECT_EQ(N, ht.size());
}


TEST(ConstructorInt, InitializerList)
{
    using namespace ss;
    std::unordered_map<int, int> items( {{1,1}, {2,2}, {3,3}, {4,4}, {10, 123}, {112312, 132131}} );
    HashTable<int, int> ht({{1,1}, {2,2}, {3,3}, {4,4}, {10, 123}, {112312, 132131}});
    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}

TEST(Insert, OperatorInsert)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items[i] = i*3;
        ht[i] = i*3;
    }

    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}


TEST(Insert, InsertLValueType)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        auto p = std::make_pair( i, i*3 );
        items.insert(p);
        ht.insert(p);
    }

    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}


TEST(Insert, InsertRValueType)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert( {i, i*3} );
        ht.insert( {i, i*3} );
    }

    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}


TEST(Insert, InsertIterator)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert( {i, i*3} );
    }
    ht.insert( items.begin(), items.end() );
    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}

TEST(Insert, InsertInitializerList)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    items.insert( {{1,1}, {2,2}, {3,3}, {4,4}, {10, 123}, {112312, 132131}} );
    ht.insert( {{1,1}, {2,2}, {3,3}, {4,4}, {10, 123}, {112312, 132131}} );
    for( const auto &kv : ht )
    {
        int value_ref = items.at(kv.first);
        int value = ht.at( kv.first );

        EXPECT_EQ( value, value_ref );
    }
}

TEST(Find, Find)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert( {i, i*3} );
        ht.insert( {i, i*3} );
    }

    for(int i = 0; i < N; ++i)
    {
        int value_ref = items.find(i)->second;
        int value = ht.find( i )->second;

        EXPECT_EQ( value, value_ref );
    }
}


TEST(Iterator, IteratorPrefixIncrement)
{
	using namespace ss;
	std::unordered_map<int, int> items;
	HashTable<int, int> ht;
	constexpr size_t N = 1 << 10;
	for (int i = 0; i < N; ++i)
	{
		items.insert({ i, i * 3 });
		ht.insert({ i, i * 3 });
	}

    size_t count = 0;
	for (HashTable<int,int>::iterator it = ht.begin(); it != ht.end(); ++it )
	{
        ++count;
    }
	EXPECT_EQ(ht.size(), count);
}



TEST(Iterator, IteratorPostfixIncrement)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert({ i, i * 3 });
        ht.insert({ i, i * 3 });
    }

    size_t count = 0;
    for (HashTable<int,int>::iterator it = ht.begin(); it != ht.end(); it++ )
    {
        ++count;
    }
    EXPECT_EQ(ht.size(), count);
}




TEST(Erase, EraseIterator)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert( {i, i*3} );
        ht.insert( {i, i*3} );

    }

    for (int i = 0; i < N; ++i)
    {
        auto it1 = items.find( i );
        items.erase(it1);
        it1 = items.find(i);
        EXPECT_EQ( it1, items.end() );

        auto it2 = ht.find( i );
        ht.erase(it2);
        it2 = ht.find( i );
        EXPECT_EQ( it2, ht.end() );
    }
}


TEST(Erase, EraseKey)
{
    using namespace ss;
    std::unordered_map<int, int> items;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        items.insert( {i, i*3} );
        ht.insert( {i, i*3} );

    }

    for (int i = 0; i < N; ++i)
    {
        items.erase(i);
        auto it1 = items.find(i);
        EXPECT_EQ( it1, items.end() );

        ht.erase(i);
        auto it2 = ht.find( i );
        EXPECT_EQ( it2, ht.end() );
    }
}


TEST(Erase, EraseRange)
{
    using namespace ss;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        ht.insert( {i, i*3} );
    }

    ht.erase(ht.begin(), ht.end());
    EXPECT_EQ(ht.size(), 0);
}


TEST(Erase, EraseSmallRange)
{
    using namespace ss;
    HashTable<int, int> ht;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        ht.insert( {i, i*3} );
    }

	HashTable<int, int>::iterator start = ht.begin();
	std::advance(start, 5);
	HashTable<int, int>::iterator end = start;
	std::advance(end, 5);
	ht.erase(start, end);

    EXPECT_EQ(ht.size(), N-5);
}





TEST(Assignment, AssignmentLvalue)
{
    using namespace ss;
    HashTable<int, int> ht1;
    HashTable<int, int> ht2;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        ht1.insert( {i, i*3} );
    }
    ht2 = ht1;

    EXPECT_EQ(ht1, ht2);
}



TEST(Assignment, AssignmentRvalue)
{
    using namespace ss;
    HashTable<int, int> ht1;
    HashTable<int, int> ht2;
    HashTable<int, int> ht3;
    constexpr size_t N = 1 << 10;
    for (int i = 0; i < N; ++i)
    {
        ht1.insert( {i, i*3} );
    }
    ht2 = std::move(ht1);
    ht3 = ht2;

    EXPECT_EQ(ht3, ht2);
}



