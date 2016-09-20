#include "HashTable.h"
#include <iostream>
#include <random>
#include <unordered_map>

using namespace ss;
using namespace std;


void stress_test_hash_table(  )
{
    HashTable<int,int> htable;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(INT_MIN, INT_MAX);
    for (int i = 0; i < 1<<10; ++i)
    {
        int key = distribution(generator);
        int value = distribution(generator);
        htable[key] = value;
    }
}

int main()
{
	HashTable<int, int> ht;
	HashTable<int, int> ht1;
	ht.insert({ std::make_pair(1, 1), std::make_pair(2,2) });
	ht == ht1;
    ht!=ht1;
	ht[2] = 3;
	ht.clear();
    stress_test_hash_table(  );
	getchar();
	return 0;
}
