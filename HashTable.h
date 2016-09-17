#pragma once
#include <vector>
#include <iterator>
#include <stdint.h>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <functional>
#include <deque>
#include <numeric>
#include <algorithm>
#include <boost/dynamic_bitset.hpp>

namespace ss
{
	struct DefaultHash
	{
        template<typename K>
		size_t operator()(const K &k) const noexcept
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
		using chain_t = std::deque<value_type>;
		using bucket_t = std::vector<chain_t>;
        using hasher = HashFunc;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        static constexpr size_type DEFAULT_MAX_SIZE = 1<<10;

        HashTable(size_type max_buckets = DEFAULT_MAX_SIZE) :
		  _buckets(max_buckets, chain_t{})
        , _bucket_count(0)
        , _max_buckets( max_buckets )
		, _bitset(max_buckets)
        , _size( 0 )
        , _load_factor( 0.0f )
        , _max_load_factor( 1.0f )
        , _first_nonempty_bucket( std::numeric_limits<size_type>::max() )
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

        void reserve( size_type n )
        {
            _buckets.reserve( n );
            // todo: rehash?
        }

        void clear()
        {
            _buckets.assign( _max_buckets, chain_t{} );
            _size = 0;
            _bucket_count = 0;
			_bitset.reset();
			_load_factor = 0.0f;
            _first_nonempty_bucket = std::numeric_limits<size_type>::max();
        }

        HashFunc &hash_function() const
        {
            return _hash_func;
        }


		mapped_type& operator[](const key_type &k)
        {
            return at_private( const_cast<key_type&>(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
        }

		mapped_type& operator[](K &&k)
		{
			return at_private(std::move(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
		}

        mapped_type& at ( const key_type& k )
        {
            return at_private( k, NOT_FOUND_POLICY::THROW_EXCEPTION);
        }

        const mapped_type& at ( const key_type& k ) const
        {
            return at_private( k, NOT_FOUND_POLICY::THROW_EXCEPTION);
        }

        size_type bucket( const key_type &k ) const noexcept
        {
            return bucket_private( k );
        }

        size_type bucket_count( ) const noexcept
        {
            return _bucket_count;
        }

        size_type bucket_size( size_type n ) const
        {
            if( n >= _bucket_count )
            {
                throw std::out_of_range( "bucket index out of range" );
            }

			_buckets[n].size();
        }

        size_type count( const key_type & key) const
        {
            size_type n = bucket_private( key );
            if( _buckets[n].empty() )
                return 0;

            return 1;
        }

        float max_load_factor() const noexcept
        {
            return _max_load_factor;
        }

        void max_load_factor ( float z )
        {
            _max_load_factor = z;
            // rehash?
        }

        void rehash( size_type n )
        {
            // todo
        }

		class iterator
		{
		public:
			using self_type = iterator;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::forward_iterator_tag;
			using difference_type = ptrdiff_t;
			typedef typename HashTable<K, V, HashFunc> container_t;

			iterator(){}

			self_type operator++()
			{
				return *this;
			}

			reference operator*()
			{
				return *this;
			}

			pointer operator->()
			{
				return this;
			}

			bool operator==(const self_type &other)
			{
				return _ptr = other._ptr;
			}

			bool operator !=(const self_type &other)
			{
				return _ptr != other._ptr;
			}

			self_type operator=(const self_type &other)
			{
				_ptr = other._ptr;
				return *this;
			}

		private:
			pointer _ptr;
            size_type bucket_idx;
		};

        iterator begin();
        iterator end();

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
					if (bucket.empty())
                    {
						++_bucket_count;
                        if( bucket_idx < _first_nonempty_bucket )
                            _first_nonempty_bucket = bucket_idx;
                    }

					std::pair<K, V> p;
					p.first = std::forward<U>(k);
					bucket.emplace_back(std::move(p));
					++_size;
					_bitset[bucket_idx] = 1;
					return bucket.back().second;
                }
            }

			return it->second;
        }

        template<typename U>
        size_type bucket_private( U &&k ) const noexcept
        {
            const size_type idx = _hash_func( std::forward<U>(k) ) % _max_buckets;
            return idx;
        }

        size_type _max_buckets;
        size_type _size;
        size_type _bucket_count;
        size_type _first_nonempty_bucket;
        float _load_factor;
        float _max_load_factor;
		std::vector<chain_t> _buckets;
		boost::dynamic_bitset<> _bitset;
        HashFunc _hash_func;
	};
}

