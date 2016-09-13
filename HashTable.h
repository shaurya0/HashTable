#pragma once
#include <vector>
#include <iterator>
#include <stdint.h>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <functional>

namespace ss
{
	struct DefaultHash
	{
        template<typename K>
		size_t operator()(K &&k) const
		{
			return std::hash<K>{}(k);
		}
	};

	template<typename K, typename V, typename HashFunc = DefaultHash>
	class HashTable
	{
    private:
        enum class NOT_FOUND_POLICY{THROW_EXCEPTION, INSERT_KEY_VALUE};
	public:
        using key_type = K;
        using mapped_type = V;
        using value_type = std::pair<K, V>;
		using chain_t = std::list<value_type>;
        using hasher = HashFunc;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        static constexpr size_type DEFAULT_MAX_SIZE = 1<<10;

        HashTable(size_type max_size = DEFAULT_MAX_SIZE) :
          _MAX_SIZE( max_size )
		, _buckets(max_size, chain_t{})
        , _bucket_count( max_size )
        , _size( 0 )
        {
			constexpr bool hash_func_returns_size_type = std::is_same<std::result_of<HashFunc(const K&)>::type, size_t>::value;
			static_assert(hash_func_returns_size_type, "hash function does not return size type");
        }

        bool empty() const noexcept
        {
            return _size == 0;
        }

        size_type size() const noexcept
        {
            return _size;
        }

        size_type max_size() const noexcept
        {
            return MAX_SIZE;
        }

        void reserve( size_type n )
        {
            _buckets.reserve( n );
        }

        template<typename U>
        void insert( U &&item );

		void emplace();

        HashFunc &hash_function() const
        {
            return _hash_func;
        }

        template<typename U>
		mapped_type& operator[](U &&k)
        {
            return ( at_private( std::forward<U>(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE) );
        }


        mapped_type& at ( const key_type& k )
        {
            return const_cast<mapped_type&>( at_private( k, NOT_FOUND_POLICY::THROW_EXCEPTION) );
        }

        const mapped_type& at ( const key_type& k ) const
        {
            return at_private( k, NOT_FOUND_POLICY::THROW_EXCEPTION);
        }


        size_type bucket( const key_type &k ) const noexcept
        {
            return bucket_private( k );
        }

        size_type bucket_count( const key_type &k ) const noexcept
        {
            return _bucket_count;
        }

        size_type bucket_size( size_type n ) const
        {
            if( n >= _bucket_count )
            {
                throw std::out_of_range( "bucket index out of range" );
            }

            const auto *node = _buckets[n];
            return node->size();
        }

    private:

        template<typename U>
        mapped_type& at_private( U &&k, NOT_FOUND_POLICY not_found_policy )
        {
            size_type bucket_idx = bucket_private( std::forward<U>(k) );
            chain_t &bucket = _buckets[bucket_idx];
			
			bool key_found = false;
			chain_t::iterator it;
			if (!bucket.empty())
			{
				auto predicate = [&k](const std::pair<K, V> &item)
				{
					return std::forward<U>(k) == item.first;
				};
				it = std::find_if(bucket.begin(), bucket.end(), predicate);
				key_found = it != bucket.end();
			}

            if( !key_found )
            {
                if ( not_found_policy == NOT_FOUND_POLICY::THROW_EXCEPTION )
                {
                    throw std::out_of_range( "key element does not exist in hash table" );
                }
                else if ( not_found_policy == NOT_FOUND_POLICY::INSERT_KEY_VALUE )
                {
					std::pair<K, V> p;
					p.first = std::forward<U>(k);
					bucket.push_back(std::move(p));
					return bucket.back().second;
                }
            }
			else
			{
				return it->second;
			}
        }

        template<typename U>
        size_type bucket_private( U &&k ) const noexcept
        {
            const size_type idx = _hash_func( std::forward<U>(k) ) % _bucket_count;
            return idx;
        }

        size_type _size;
        size_type _bucket_count;
        const size_type _MAX_SIZE;
		std::vector<chain_t> _buckets;
        HashFunc _hash_func;
	};
}