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
#include <assert.h>
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
        typedef typename HashTable<K,V,HashFunc> HashTableType;

        HashTable(size_type max_buckets = DEFAULT_MAX_SIZE) :
		  _buckets(max_buckets, chain_t{})
        , _bucket_count(0)
        , _max_buckets( max_buckets )
		, _non_empty_buckets(max_buckets)
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

        void clear()
        {
            _buckets.assign( _max_buckets, chain_t{} );
            _size = 0;
            _bucket_count = 0;
			_non_empty_buckets.reset();
			_load_factor = 0.0f;
            _first_nonempty_bucket = std::numeric_limits<size_type>::max();
            _last_nonempty_bucket = std::numeric_limits<size_type>::max();
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

        size_type bucket_count() const noexcept
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

        void reserve( size_type n )
        {
            size_type nload = static_cast<size_type>(n * _max_load_factor);
            if( nload > _max_buckets )
                rehash(nload);
        }

        void max_load_factor ( float z )
        {
            size_type nload = static_cast<size_type>(n * _max_load_factor);
            if( nload > _max_buckets )
                rehash(nload);
        }

        void rehash( size_type n )
        {

        }

    private:
		friend class iterator;
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
                        _non_empty_buckets[bucket_idx] = 1;

                        if( bucket_idx < _first_nonempty_bucket )
                            _first_nonempty_bucket = bucket_idx;

                        if( bucket_idx > _last_nonempty_bucket )
                            _last_nonempty_bucket = bucket_idx;
                    }

					std::pair<K, V> p;
					p.first = std::forward<U>(k);
					bucket.emplace_back(std::move(p));
					++_size;
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
        size_type _last_nonempty_bucket;

        float _load_factor;
        float _max_load_factor;
		std::vector<chain_t> _buckets;
		boost::dynamic_bitset<> _non_empty_buckets;
        HashFunc _hash_func;

    public:
        class iterator
        {
        public:
            using self_type = iterator;
            using reference = value_type&;
            using pointer = value_type*;
            using iterator_category = std::forward_iterator_tag;
            using difference_type = ptrdiff_t;
            typedef typename HashTable<K, V, HashFunc> container_t;

            iterator()
            : _container( nullptr )
            , _bucket_idx(std::numeric_limits<size_type>::max())
            , _chain_idx(std::numeric_limits<size_type>::max()) {}

            self_type operator++()
            {
                assert( _container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
                const auto &chain = _container->_buckets[_bucket_idx];
                if( _chain_idx < (chain.size() - 1))
                {
                    return iterator( _container, _bucket_idx, ++_chain_idx );
                }

                auto next_nonempty_bucket = _container->_non_empty_buckets.find_next(_bucket_idx);
                if (boost::dynamic_bitset<>::npos != next_nonempty_bucket)
                {
                    return iterator(_container, next_nonempty_bucket, 0);
                }

                return _container->end();
            }

            self_type operator++(int dummy)
            {
                assert( _container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
                const auto &chain = _container->_buckets[_bucket_idx];
                if( _chain_idx < (chain.size() - 1))
                {
                    self_type me = *this;
                    _chain_idx++;
                    return me;
                }

				auto next_nonempty_bucket = _container->_non_empty_buckets.find_next(_bucket_idx);
				if (boost::dynamic_bitset<>::npos != next_nonempty_bucket)
				{
                    self_type me = *this;
                    _bucket_idx = next_nonempty_bucket;
                    _chain_idx = 0;
                    return me;
				}

				return _container->end();
            }

            reference operator*()
            {
                assert( _container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
                return _container->_buckets[_bucket_idx][_chain_idx];
            }

            pointer operator->()
            {
                assert( _container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
                return &(_container->_buckets[_bucket_idx][_chain_idx]);
            }

            bool operator==(const self_type &other)
            {
                // return ( _container == other.container && _bucket_idx == other.bucket_idx && _chain_idx == other.chain_idx);
                return ( _bucket_idx == other.bucket_idx && _chain_idx == other.chain_idx);
            }

            bool operator !=(const self_type &other)
            {
                // return ( _container != other.container || _bucket_idx != other.bucket_idx || _chain_idx != other.chain_idx);
                return ( _bucket_idx != other.bucket_idx && _chain_idx != other.chain_idx);
            }

            self_type operator=(const self_type &other)
            {
                if( &other == this )
                    return *this;

				_container = other._container;
				_bucket_idx = other._bucket_idx;
				_chain_idx = other._chain_idx;
                return *this;
            }

        private:
			friend class container_t;
            iterator(container_t *container, size_type bucket_index, size_type chain_index)
                : _container( container )
                , _bucket_idx( bucket_index )
                , _chain_idx( chain_index )
            {
                assert( _container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
            }

            container_t *_container;
            size_type _bucket_idx;
            size_type _chain_idx;
        };

        iterator begin()
        {
            return iterator(this, _first_nonempty_bucket, 0);
        }

        iterator end()
        {
            return iterator( this, _last_nonempty_bucket, _buckets[_last_nonempty_bucket].size() );
        }

        iterator find( const K &key )
        {
            size_type bucket_idx = bucket_private( key );
            chain_t &bucket = _buckets[bucket_idx];

            if (!bucket.empty())
            {
                auto predicate = [&key](const std::pair<K, V> &item)
                {
                    return key == item.first;
                };

                const chain_t::iterator it = std::find_if(bucket.begin(), bucket.end(), predicate);
                const bool key_found = it != bucket.end();

                if( key_found )
                    return iterator( this, bucket_idx, std::distance( bucket.begin(), it ) );
            }

            return end();
        }
	};

    template<typename K, typename V, typename HashFunc = DefaultHash>
    bool operator==( const HashTable<K,V,HashFunc> &lhs, const HashTable<K,V,HashFunc> &rhs)
    {
        return false;
    }

    template<typename K, typename V, typename HashFunc = DefaultHash>
    bool operator!=( const HashTable<K,V,HashFunc> &lhs, const HashTable<K,V,HashFunc> &rhs)
    {
        return false;
    }


}

