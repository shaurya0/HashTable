//#include "HashTable.h"
//#include <iostream>
//#include <random>
//#include <unordered_map>
//#include <gtest/gtest.h>
//
//using namespace ss;
//using namespace std;
//
//
//
//void stress_test_hash_table(  )
//{
//    HashTable<int,int> htable;
//    std::default_random_engine generator;
//    std::uniform_int_distribution<int> distribution(0, INT_MAX);
//    for (int i = 0; i < 1<<4; ++i)
//    {
//        htable[i] = i;
//    }
//	htable.erase(8);
//	for (HashTable<int, int>::const_iterator it = htable.begin(); it != htable.end(); ++it)
//	{
//		std::cout << it->first << " : " << it->second << std::endl;
//	}
//}
//
//int main()
//{
//	HashTable<int, int> ht;
//    stress_test_hash_table(  );
//
//	getchar();
//	return 0;
//}
