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
		enum class NOT_FOUND_POLICY { THROW_EXCEPTION, INSERT_KEY_VALUE };
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

		typedef typename chain_t::iterator local_iterator;
		typedef typename chain_t::const_iterator const_local_iterator;

		static constexpr size_type DEFAULT_BUCKET_COUNT = 1 << 3;
		typedef typename HashTable<K, V, HashFunc> HashTableType;

		explicit HashTable(size_type bucket_count = DEFAULT_BUCKET_COUNT)
		{
			constexpr bool hash_func_returns_size_type = std::is_same<std::result_of<HashFunc(const K&)>::type, size_t>::value;
			static_assert(hash_func_returns_size_type, "hash function does not return size type");

            _init( bucket_count );
		}

        template <class InputIterator>
        HashTable ( InputIterator first, InputIterator last,
            size_type n = DEFAULT_BUCKET_COUNT)
        {
            const size_type first_last_distance = std::distance(first, last);
            size_type bucket_count = first_last_distance > n ? first_last_distance : n;
            _init( bucket_count );
            insert( first, last );
        }

        HashTable ( const HashTableType& ump )
        {
            *this = ump;
        }

        HashTable ( HashTableType&& ump )
        {
            *this = std::move(ump);
        }

        HashTable ( std::initializer_list<value_type> il,
                        size_type n = DEFAULT_BUCKET_COUNT)
        {
            const size_type il_size = il.size();
            size_type bucket_count = il_size > n ? il_size : n;
            _init( bucket_count );
            insert( il );
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
			_init(DEFAULT_BUCKET_COUNT);
		}

		HashFunc &hash_function() const
		{
			return _hash_func;
		}


		mapped_type& operator[](const key_type &k)
		{
			return _at(const_cast<key_type&>(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
		}

		mapped_type& operator[](K &&k)
		{
			return _at(std::move(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
		}

		mapped_type& at(const key_type& k)
		{
			return _at(k, NOT_FOUND_POLICY::THROW_EXCEPTION);
		}

		const mapped_type& at(const key_type& k) const
		{
			return _at(k, NOT_FOUND_POLICY::THROW_EXCEPTION);
		}

		size_type bucket(const key_type &k) const noexcept
		{
			return bucket_private(k);
		}

		size_type bucket_count() const noexcept
		{
			return _buckets.size();
		}

		size_type bucket_size(size_type n) const
		{
			if (n >= _buckets.size())
			{
				throw std::out_of_range("bucket index out of range");
			}

			_buckets[n].size();
		}

		size_type count(const key_type & key) const
		{
			size_type n = bucket_private(key);
			if (_buckets[n].empty())
				return 0;

			return 1;
		}

		float max_load_factor() const noexcept
		{
			return _max_load_factor;
		}

		void reserve(size_type n)
		{
			size_type nload = static_cast<size_type>(_buckets.size() * _max_load_factor);
			if (nload < n)
				rehash(n);
		}

		void max_load_factor(float z)
		{
			_max_load_factor = z;
			size_type nload = static_cast<size_type>(_buckets.size() * _max_load_factor);
			if (nload < n)
				rehash(n);
		}

		void rehash(size_type n)
		{
			if (n <= _bucket_count)
				return;

            std::vector<chain_t> buckets = std::move( _buckets );
            _init( n );

            for( auto& chain : buckets )
            {
                if( chain.empty() )
                    continue;

                for( auto& kv : chain )
                {
                    insert( std::move( kv ) );
                }
            }
		}

	private:
		//0: found, 1: bucket index, 2: chain index
		template<typename U>
		std::tuple<bool, size_t, size_t> _find(U &&key) const noexcept
		{
			size_type bucket_idx = bucket_private(std::forward<U>(key));
			const chain_t &bucket = _buckets[bucket_idx];

			bool key_found = false;

			chain_t::const_iterator it;
			if (!bucket.empty())
			{
				auto predicate = [&key](const std::pair<K, V> &item)
				{
					return std::forward<U>(key) == item.first;
				};
				it = std::find_if(bucket.begin(), bucket.end(), predicate);
				key_found = it != bucket.end();
			}

			return std::make_tuple(key_found, bucket_idx, key_found ? std::distance(bucket.begin(), it) : 0);
		}

		template<typename U>
		mapped_type& _insert_in_container(size_type bucket_idx, U &&value, size_t *chain_idx = nullptr)
		{
			size_t buckets = _buckets.size();
            float new_load_factor = static_cast<float>(_size+1) / static_cast<float>(buckets);
			bool rehashed = false;
			while (new_load_factor >= _max_load_factor)
			{
				buckets <<= 1;
				new_load_factor = static_cast<float>(_size+1) / static_cast<float>(buckets);
				rehashed = true;
			}

			rehash(buckets);

			if (rehashed)
				bucket_idx = bucket_private(std::forward<U>(value).first);

            chain_t &bucket = _buckets[bucket_idx];
            if (bucket.empty())
            {
                _non_empty_buckets[bucket_idx] = 1;

                if (bucket_idx < _first_nonempty_bucket)
                    _first_nonempty_bucket = bucket_idx;

                if (bucket_idx > _last_nonempty_bucket)
                    _last_nonempty_bucket = bucket_idx;
            }

			bucket.push_back(std::forward<U>(value));
			++_size;
			_load_factor =  static_cast<float>(_size) / static_cast<float>(buckets);

			if (chain_idx != nullptr)
				*chain_idx = bucket.size() - 1;

			return _buckets[bucket_idx].back().second;
		}

		template<typename U>
		mapped_type& _at(U &&k, NOT_FOUND_POLICY not_found_policy)
		{
			std::tuple<bool, size_t, size_t> found = _find(std::forward<U>(k));
			const bool key_found = std::get<0>(found);
			const size_type bucket_idx = std::get<1>(found);
			const size_type chain_idx = std::get<2>(found);

			if (!key_found)
			{
				if (not_found_policy == NOT_FOUND_POLICY::THROW_EXCEPTION)
				{
					throw std::out_of_range("key element does not exist in hash table");
				}
				else if (not_found_policy == NOT_FOUND_POLICY::INSERT_KEY_VALUE)
				{
					std::pair<K, V> p;
					p.first = std::forward<U>(k);

                    return _insert_in_container( bucket_idx, std::move( p ) );
				}
			}

			return std::get<1>(_buckets[bucket_idx][chain_idx]);
		}

		template<typename U>
		size_type bucket_private(U &&k) const noexcept
		{
			const size_type idx = _hash_func(std::forward<U>(k)) % _bucket_count;
			return idx;
		}

		size_type _size;
        size_type _bucket_count;
		size_type _first_nonempty_bucket;
		size_type _last_nonempty_bucket;

		float _load_factor;
		float _max_load_factor;
		std::vector<chain_t> _buckets;
		boost::dynamic_bitset<> _non_empty_buckets;
		HashFunc _hash_func;

		template<bool is_const_iterator = true>
		class _ht_iterator : public std::iterator<std::forward_iterator_tag, value_type>
		{
		public:
			typedef typename std::conditional<is_const_iterator, const HashTableType*, HashTableType*>::type container_ptr_t;
			typedef typename std::conditional<is_const_iterator, const value_type&, value_type&>::type reference_type_it_t;
			typedef typename std::conditional<is_const_iterator, const value_type*, value_type*>::type pointer_type_it_t;

			friend class _ht_iterator<true>;
			friend class HashTableType;
            _ht_iterator(container_ptr_t container = nullptr, size_type bucket_idx = 0, size_type chain_idx = 0)
                : _container(container)
                , _bucket_idx(bucket_idx)
                , _chain_idx(chain_idx) {}


			_ht_iterator(const _ht_iterator<false>& other)
				: _container(other._container)
				, _bucket_idx(other._bucket_idx)
				, _chain_idx(other._chain_idx) {}


			_ht_iterator(const _ht_iterator<true>& other)
				: _container(const_cast<container_ptr_t>(other._container))
				, _bucket_idx(other._bucket_idx)
				, _chain_idx(other._chain_idx) {}

			_ht_iterator &operator++()
			{
				assert(_container != nullptr && _bucket_idx < _container->_buckets.size() );

				const auto &chain = _container->_buckets[_bucket_idx];
				if (!chain.empty() && _chain_idx < (chain.size()-1))
				{
					++_chain_idx;
					return *this;
				}

				auto next_nonempty_bucket = _container->_non_empty_buckets.find_next(_bucket_idx);
				if (boost::dynamic_bitset<>::npos != next_nonempty_bucket)
				{
					_bucket_idx = next_nonempty_bucket;
					_chain_idx = 0;
					return *this;
				}

				// todo: hack to get end to match
				if (_chain_idx < chain.size())
					++_chain_idx;

				return _container->end();
			}

			_ht_iterator operator++(int dummy)
			{
				assert(_container != nullptr && _bucket_idx < _container->_buckets.size() && _chain_idx < _container->_buckets[_bucket_idx].size());
				_ht_iterator copy(*this);
				++*this;
				return copy;
			}

			reference_type_it_t operator*()
			{
				assert(_container != nullptr && _bucket_idx < _container->_buckets.size() && _chain_idx < _container->_buckets[_bucket_idx].size());
				return _container->_buckets[_bucket_idx][_chain_idx];
			}

			pointer_type_it_t operator->()
			{
				assert(_container != nullptr && _bucket_idx < _container->_buckets.size() && _chain_idx < _container->_buckets[_bucket_idx].size());
				return &(_container->_buckets[_bucket_idx][_chain_idx]);
			}

			bool operator==(const _ht_iterator &other) const
			{
				return (_container == other._container && _bucket_idx == other._bucket_idx && _chain_idx == other._chain_idx);
			}

            bool operator !=(const _ht_iterator &other) const
            {
                return !( *this == other );
            }

			template<bool b>
			_ht_iterator<b> &operator=(const _ht_iterator<b> &other)
			{
				if (&other == this)
					return *this;

				_container = other._container;
				_bucket_idx = other._bucket_idx;
				_chain_idx = other._chain_idx;
				return *this;
			}

		private:
			container_ptr_t _container;
			size_type _bucket_idx;
			size_type _chain_idx;
		};


	public:
		using iterator = _ht_iterator<false>;
		using const_iterator = _ht_iterator<true>;
		friend class iterator;

		iterator begin()
		{
			if (_size == 0)
				return end();

			return iterator(this, _first_nonempty_bucket, 0);
		}

		iterator end()
		{
			return iterator(this, _buckets.size() - 1, 0);
		}

		const_iterator begin() const
		{
			if (_size == 0)
				return end();

			return const_iterator(this, _first_nonempty_bucket, 0);
		}

		const_iterator end() const
		{
			return const_iterator(this, _buckets.size() - 1, 0);
		}
	public:

		iterator find(const K &key)
		{
			auto found = _find(key);
			if (std::get<0>(found))
				return iterator(this, std::get<1>(found), std::get<2>(found));

			return end();
		}

		const_iterator find(const K &key) const
		{
			auto found = _find(key);
			if (std::get<0>(found))
				return const_iterator(this, std::get<1>(found), std::get<2>(found));

			return end();
		}
	private:
		void _init(size_type bucket_count)
		{
			_buckets.assign(bucket_count, chain_t{});
			_size = 0;
            _bucket_count = bucket_count > 0 ? bucket_count - 1 : 0;
			_non_empty_buckets.resize(bucket_count);
			_non_empty_buckets.reset();
			_load_factor = 0.0f;
			_max_load_factor = 1.0f;
			_first_nonempty_bucket = bucket_count;
			_last_nonempty_bucket = 0;
		}

		template<typename U>
		std::pair<iterator, bool> _insert(U &&val)
		{
			auto found = _find(std::forward<U>(val).first);
			if(std::get<0>(found))
				return std::make_pair(end(), false);

            size_t chain_idx = 0;
			const size_t bucket_idx = std::get<1>(found);

            _insert_in_container( bucket_idx, std::forward<U>( val ), &chain_idx );
			return std::make_pair(iterator(this, bucket_idx, chain_idx), true);
		}

	public:

		std::pair<iterator, bool> insert(const value_type& val)
		{
			return _insert(val);
		}

		template <class P>
		std::pair<iterator, bool> insert(P&& val)
		{
			return _insert(std::forward<P>(val));
		}

		iterator insert(const_iterator hint, const value_type& val)
		{
			return _insert(val).first;
		}

		template <class P>
		iterator insert(const_iterator hint, P&& val)
		{
			return _insert(std::forward<P>(val)).first;
		}

		template <class InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			while (first != last)
			{
				_insert(*first);
				++first;
			}
		}

		void insert(std::initializer_list<value_type> il)
		{
			for (auto &value : il)
			{
				_insert(value);
			}
		}

		iterator erase(const_iterator position)
		{
			if (position._bucket_idx >= _buckets.size())
				return end();

			chain_t &bucket = _buckets[position._bucket_idx];
			if (position._chain_idx >= bucket.size())
				return end();

			local_iterator it = bucket.begin();
			std::advance(it, position._chain_idx);

			local_iterator result = bucket.erase(it);

			_load_factor = static_cast<float>(--_size) / static_cast<float>(_buckets.size());

			if (bucket.empty())
			{
				_non_empty_buckets[position._bucket_idx] = 0;

				if (_first_nonempty_bucket == position._bucket_idx)
				{
					auto next_nonempty_bucket = _non_empty_buckets.find_first();

					if (boost::dynamic_bitset<>::npos != next_nonempty_bucket)
					{
						_first_nonempty_bucket = next_nonempty_bucket;
						return iterator(this, next_nonempty_bucket, 0);
					}
					else
						return end();
				}

				if (_last_nonempty_bucket == 0)
					return end();

				if (_last_nonempty_bucket == position._bucket_idx)
				{
					std::vector<chain_t>::reverse_iterator rit = _buckets.rbegin();
					std::advance(rit, _buckets.size() - position._bucket_idx);

					size_t j = position._bucket_idx - 1;
					for (; rit != _buckets.rend(); ++rit)
					{
						if (!rit->empty())
						{
							_last_nonempty_bucket = j;
							return iterator(this, _last_nonempty_bucket, 0);
						}
						--j;
					}

					return end();

				}
			}

			if (result == bucket.end())
				return std::next(position);

			return position;
		}

		size_type erase(const key_type& k)
		{
			auto found = _find(k);
			if (!std::get<0>(found))
				return 0;

			const_iterator it(this, std::get<1>(found), std::get<2>(found));
			erase(it);
			return 1;
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			const_iterator it = first;
			while (it != last)
			{
				erase(it);
				++it;
			}

			return it;
		}


		HashTableType& operator= (const HashTableType& ht)
		{
            _size = ht._size;
            _bucket_count = ht._bucket_count;
            _first_nonempty_bucket = ht._first_nonempty_bucket;
            _last_nonempty_bucket = ht._last_nonempty_bucket;

            _load_factor = ht._load_factor;
            _max_load_factor = ht._max_load_factor;
            _buckets = ht._buckets;
            _non_empty_buckets = ht._non_empty_buckets;
			return *this;
		}

		HashTableType& operator= (HashTableType&& ht)
		{
            _size = std::move(ht._size);
            _bucket_count = std::move(ht._bucket_count);
            _first_nonempty_bucket = std::move(ht._first_nonempty_bucket);
            _last_nonempty_bucket = std::move(ht._last_nonempty_bucket);

            _load_factor = std::move(ht._load_factor);
            _max_load_factor = std::move(ht._max_load_factor);
            _buckets = std::move(ht._buckets);
            _non_empty_buckets = std::move(ht._non_empty_buckets);
            ht._init(DEFAULT_BUCKET_COUNT);
            return *this;
		}

		HashTableType& operator= (std::initializer_list<value_type> il)
		{
			_init(il.size());
			insert(il);
		}
	};

	template<typename K, typename V, typename HashFunc = DefaultHash>
	bool operator==(const HashTable<K, V, HashFunc> &lhs, const HashTable<K, V, HashFunc> &rhs)
	{
		if (lhs.size() != rhs.size())
			return false;

		for (const auto &kv : lhs)
		{
			const K& key = kv.first;
			HashTable<K, V, HashFunc>::const_iterator rhs_it = rhs.find(key);
			const bool not_found = rhs.end() == rhs_it;
			if (not_found)
				return false;

			if (kv.second != rhs_it->second)
				return false;
		}

		return true;
	}

	template<typename K, typename V, typename HashFunc = DefaultHash>
	bool operator!=(const HashTable<K, V, HashFunc> &lhs, const HashTable<K, V, HashFunc> &rhs)
	{
		return !(lhs == rhs);
	}
}

