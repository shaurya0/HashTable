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
		static constexpr size_type DEFAULT_MAX_SIZE = 1 << 10;
		typedef typename HashTable<K, V, HashFunc> HashTableType;

		HashTable(size_type max_buckets = DEFAULT_MAX_SIZE) :
			_buckets(max_buckets, chain_t{})
			, _bucket_count(0)
			, _max_buckets(max_buckets)
			, _non_empty_buckets(max_buckets)
			, _size(0)
			, _load_factor(0.0f)
			, _max_load_factor(1.0f)
			, _first_nonempty_bucket(std::numeric_limits<size_type>::max())
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
			_buckets.assign(_max_buckets, chain_t{});
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
			return at_private(const_cast<key_type&>(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
		}

		mapped_type& operator[](K &&k)
		{
			return at_private(std::move(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE);
		}

		mapped_type& at(const key_type& k)
		{
			return at_private(k, NOT_FOUND_POLICY::THROW_EXCEPTION);
		}

		const mapped_type& at(const key_type& k) const
		{
			return at_private(k, NOT_FOUND_POLICY::THROW_EXCEPTION);
		}

		size_type bucket(const key_type &k) const noexcept
		{
			return bucket_private(k);
		}

		size_type bucket_count() const noexcept
		{
			return _bucket_count;
		}

		size_type bucket_size(size_type n) const
		{
			if (n >= _bucket_count)
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
			size_type nload = static_cast<size_type>(n * _max_load_factor);
			if (nload > _max_buckets)
				rehash(nload);
		}

		void max_load_factor(float z)
		{
			size_type nload = static_cast<size_type>(n * _max_load_factor);
			if (nload > _max_buckets)
				rehash(nload);
		}

		void rehash(size_type n)
		{

		}

	private:
		//0: found, 1: bucket index, 2: chain index
		template<typename U>
		std::tuple<bool, size_t, size_t> find_private(U &&k) const noexcept
		{
			size_type bucket_idx = bucket_private(std::forward<U>(k));
			const chain_t &bucket = _buckets[bucket_idx];

			bool key_found = false;

			chain_t::const_iterator it;
			if (!bucket.empty())
			{
				auto predicate = [&k](const std::pair<K, V> &item)
				{
					return std::forward<U>(k) == item.first;
				};
				it = std::find_if(bucket.begin(), bucket.end(), predicate);
				key_found = it != bucket.end();
			}

			return std::make_tuple(key_found, bucket_idx, key_found ? std::distance(bucket.begin(), it) : 0);
		}

		template<typename U>
		mapped_type& at_private(U &&k, NOT_FOUND_POLICY not_found_policy)
		{
			auto found = find_private(std::forward<U>(k));
			const bool key_found = std::get<0>(found);
			size_t bucket_idx = std::get<1>(found);
			size_t chain_idx = std::get<2>(found);

			chain_t &bucket = _buckets[bucket_idx];
			if (!key_found)
			{
				if (not_found_policy == NOT_FOUND_POLICY::THROW_EXCEPTION)
				{
					throw std::out_of_range("key element does not exist in hash table");
				}
				else if (not_found_policy == NOT_FOUND_POLICY::INSERT_KEY_VALUE)
				{
					if (bucket.empty())
					{
						++_bucket_count;
						_non_empty_buckets[bucket_idx] = 1;

						if (bucket_idx < _first_nonempty_bucket)
							_first_nonempty_bucket = bucket_idx;

						if (bucket_idx > _last_nonempty_bucket)
							_last_nonempty_bucket = bucket_idx;
					}

					std::pair<K, V> p;
					p.first = std::forward<U>(k);
					bucket.emplace_back(std::move(p));
					++_size;
					return bucket.back().second;
				}
			}

			return std::get<1>(_buckets[bucket_idx][chain_idx]);
		}

		template<typename U>
		size_type bucket_private(U &&k) const noexcept
		{
			const size_type idx = _hash_func(std::forward<U>(k)) % _max_buckets;
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

		template<bool is_const_iterator = true>
		class _ht_iterator : public std::iterator<std::forward_iterator_tag, value_type>
		{
		public:
			typedef typename std::conditional<is_const_iterator, const HashTableType*, HashTableType*>::type container_ptr_t;
			typedef typename std::conditional<is_const_iterator, const value_type&, value_type&>::type reference_type_it_t;
			typedef typename std::conditional<is_const_iterator, const value_type*, value_type*>::type pointer_type_it_t;
			friend class container_t;
			friend class _ht_iterator<false>;
			friend class _ht_iterator<true>;
			_ht_iterator(container_ptr_t container = nullptr, size_type bucket_idx = std::numeric_limits<size_type>::max(), size_type chain_idx = std::numeric_limits<size_type>::max())
				: _container(container)
				, _bucket_idx(bucket_idx)
				, _chain_idx(chain_idx) {}

			_ht_iterator(const _ht_iterator<false> &other)
				: _container(other._container)
				, _bucket_idx(other._bucket_idx)
				, _chain_idx(other._chain_idx)
			{}

			_ht_iterator &operator++()
			{
				assert(_container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
				const auto &chain = _container->_buckets[_bucket_idx];
				if (_chain_idx < (chain.size() - 1))
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

				return _container->end();
			}

			_ht_iterator operator++(int dummy)
			{
				assert(_container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
				const auto &chain = _container->_buckets[_bucket_idx];
				if (_chain_idx < (chain.size() - 1))
				{
					_ht_iterator me = *this;
					_chain_idx++;
					return me;
				}

				auto next_nonempty_bucket = _container->_non_empty_buckets.find_next(_bucket_idx);
				if (boost::dynamic_bitset<>::npos != next_nonempty_bucket)
				{
					_ht_iterator me = *this;
					_bucket_idx = next_nonempty_bucket;
					_chain_idx = 0;
					return me;
				}

				return _container->end();
			}

			reference_type_it_t operator*()
			{
				assert(_container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
				return _container->_buckets[_bucket_idx][_chain_idx];
			}

			pointer_type_it_t operator->()
			{
				assert(_container != nullptr && _bucket_idx < _container->_max_buckets && _chain_idx < _container->_buckets[_bucket_idx].size());
				return &(_container->_buckets[_bucket_idx][_chain_idx]);
			}

			bool operator==(const _ht_iterator &other) const
			{
				return (_container == other._container && _bucket_idx == other._bucket_idx && _chain_idx == other._chain_idx);
			}

			bool operator !=(const _ht_iterator &other) const
			{
				return (_container != other._container || _bucket_idx != other._bucket_idx || _chain_idx != other._chain_idx);
			}

			_ht_iterator operator=(const _ht_iterator &other)
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

		iterator begin()
		{
			return iterator(this, _first_nonempty_bucket, 0);
		}

		iterator end()
		{
			return iterator(this, _last_nonempty_bucket, _buckets[_last_nonempty_bucket].size());
		}

		const_iterator begin() const
		{
			return const_iterator(this, _first_nonempty_bucket, 0);
		}

		const_iterator end() const
		{
			return const_iterator(this, _last_nonempty_bucket, _buckets[_last_nonempty_bucket].size());
		}	
	public:

		iterator find(const K &key)
		{
			auto found = find_private(key);
			if (std::get<0>(found))
			{
				return iterator(this, std::get<1>(found), std::get<2>(found));
			}

			return end();
		}

		const_iterator find(const K &key) const
		{
			auto found = find_private(key);
			if (std::get<0>(found))
			{
				return const_iterator(this, std::get<1>(found), std::get<2>(found));
			}

			return end();
		}
	private:
		template<typename U>
		std::pair<iterator, bool> insert_private(U &&val)
		{
			auto found = find_private(val.first);			
			if(std::get<0>(found))
				return std::make_pair(end(), false);

			return std::make_pair(iterator(this, std::get<1>(found), std::get<2>(found)), true);
		}

	public:

		std::pair<iterator, bool> insert(const value_type& val)
		{
			return insert_private(val);
		}

		template <class P>
		std::pair<iterator, bool> insert(P&& val)
		{
			return insert_private(std::forward<P>(val));
		}

		iterator insert(const_iterator hint, const value_type& val)
		{
			return insert_private(val).first;
		}

		template <class P>
		iterator insert(const_iterator hint, P&& val)
		{
			return insert_private(std::forward<P>(val)).first;
		}

		template <class InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			while (first != last)
			{
				insert_private(*first);
				++first;
			}
		}

		void insert(std::initializer_list<value_type> il) 
		{
			for (auto &value : il)
			{
				insert_private(value);
			}
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

