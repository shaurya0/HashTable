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

	std::unordered_map<int, int> um;
	int x = um.bucket_count();
	for (int i = 0; i < 16; ++i)
	{
		um[i] = i;
	}

	x = um.bucket_count();

	um.clear();
	x = um.bucket_count();
	HashTable<int, int> ht;
	HashTable<int, int> ht1;
	ht[2] = 3;
	ht.insert(std::make_pair(3, 4));
    stress_test_hash_table(  );
	getchar();
	return 0;
}
