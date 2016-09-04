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
			// TODO
			return 0;
		}
	};

    template<typename K, typename V>
    struct HashNode
    {
    private:
        std::pair<K,V> _key_value;
        HashNode<K,V> *_next;
        bool _initialized;

    public:
        template<typename U, typename W>
        HashNode(U&& u, W &&w) :
          _key_value( std::make_pair( std::forward<U>(u), std::forward<W>(w) ) )
        , _next( nullptr )
        , _initialized( true )
        {}

        template<typename U>
        HashNode(U&& u) :
        , _next( nullptr )
        , _initialized( false )
        {
            _key_value.first = std::forward<U>( u );
        }



        HashNode() : _next( nullptr ), _initialized( false ) {}

        const HashNode<K,V> *next() const noexcept
        {
            return _next;
        }

		HashNode<K, V> *next() noexcept
		{
			return _next;
		}


        const std::pair<K,V> &get_key_value() const noexcept
        {
            return _key_value;
        }

		std::pair<K, V> &get_key_value() noexcept
		{
			return _key_value;
		}


        HashNode<K,V>* find(const std::function<bool( const HashNode<K,V>& node )> &predicate)
        {
            auto *it = this;
            while( it != nullptr )
            {
                if ( predicate( *it ) )
                {
                    return it;
                }
                it = _next->next();
            }
            return nullptr;
        }

        size_t size() const noexcept
        {
            size_t sz = 0;
            const auto *it = _next;
            while( it != nullptr )
            {
                ++sz;
                it = _next->next();
            }
            return sz;
        }

        bool initialized() const noexcept
        {
            return _initialized;
        }

        HashNode *get_last() noexcept
        {
            if( !initialized )
            {
                return this;
            }

            auto *it = _next;
            while(it != nullptr)
            {
                it = it->next();
            }
            return it;
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
        , _buckets( max_size, nullptr )
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
        reference operator[](U &&k)
        {
            return const_cast<mapped_type&>( at_private( std::forward<U>(k), NOT_FOUND_POLICY::INSERT_KEY_VALUE) );
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
        const mapped_type& at_private( U &&k, NOT_FOUND_POLICY not_found_policy ) const
        {
            size_type bucket_idx = bucket_private( k );
            auto *node = _buckets[bucket_idx];
            if( node != nullptr )
            {
                auto predicate = [&k]( const HashNode<K,V> &node )
                {
                    return std::forward<U>( k ) == node.get_key_value().first;
                };

                node->find( predicate );
            }

            if( node == nullptr )
            {
                if ( not_found_policy == NOT_FOUND_POLICY::THROW_EXCEPTION )
                {
                    throw std::out_of_range( "key element does not exist in hash table" );
                }
                else if ( not_found_policy == NOT_FOUND_POLICY::INSERT_KEY_VALUE )
                {
                    node = new HashNode<K,V>( std::forward<U>( k ) );
                }
            }

            return node->get_key_value().second;
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
		std::vector<HashNode<K,V>*> _buckets;
        HashFunc _hash_func;
	};
}