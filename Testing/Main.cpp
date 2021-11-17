#include <iostream>
#include <list>
#include <set>
#include <functional>
#include <tuple>
#include <vector>

//==========================================================================
using namespace std;

template < typename, typename... >
class Invoker;

namespace FunctionTraits
{
	template < typename T >
	struct FunctionInfo;

	template < typename R, typename... A >
	struct FunctionInfo< R( A... ) >
	{
		using Signature = R( A... );
		using Return = R;
		using Arguments = tuple< A... >;
		static constexpr bool IsStatic = true;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = false;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename O, typename R, typename... A >
	struct FunctionInfo< R( O::* )( A... ) >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = false;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = true;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename O, typename R, typename... A >
	struct FunctionInfo< R( O::* )( A... ) const >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = false;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = true;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename O, typename R, typename... A >
	struct FunctionInfo< R( O::* )( A... ) volatile >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = false;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = true;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename O, typename R, typename... A >
	struct FunctionInfo< R( O::* )( A... ) const volatile >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = false;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = true;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename R, typename... A >
	struct FunctionInfo< R( * )( A... ) >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = true;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = false;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename R, typename... A >
	struct FunctionInfo< R( & )( A... ) >
		: FunctionInfo< R( A... ) >
	{
		static constexpr bool IsStatic = true;
		static constexpr bool IsLambda = false;
		static constexpr bool IsMember = false;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename T >
	struct FunctionInfo
	{
		using Signature = typename FunctionInfo< decltype( &remove_reference< T >::type::operator() ) >::Signature;
		using Return = typename FunctionInfo< Signature >::Return;
		using Arguments = typename FunctionInfo< Signature >::Arguments;
		static constexpr bool IsStatic = false;
		static constexpr bool IsLambda = true;
		static constexpr bool IsMember = false;
		static constexpr bool IsFunction = IsStatic || IsLambda || IsMember;
	};

	template < typename T >
	using GetSignature = typename FunctionInfo< T >::Signature;

	template < typename T >
	using GetReturn = typename FunctionInfo< T >::Return;

	template < typename T >
	using GetArguments = typename FunctionInfo< T >::Arguments;

	template < typename R, typename... A >
	struct ConvertToInvokerImpl
	{
		using Type = typename Invoker< R, A... >;
	};

	template < typename R, typename... A >
	struct ConvertToInvokerImpl< R, tuple< A... > >
	{
		using Type = typename Invoker< R, A... >;
	};

	template < typename T >
	using ConvertToInvoker = ConvertToInvokerImpl< GetReturn< T >, GetArguments< T > >;

	template < typename T >
	using EnableIfLambdaF = enable_if_t< FunctionInfo< T >::IsLambda, void >;

	template < typename T >
	using DisableIfLambdaF = enable_if_t< !FunctionInfo< T >::IsLambda, void >;

	template < typename T >
	using EnableIfStaticF = enable_if_t< FunctionInfo< T >::IsStatic, void >;

	template < typename T >
	using DisableIfStaticF = enable_if_t< !FunctionInfo< T >::IsStatic, void >;

	template < typename T >
	using EnableIfMemberF = enable_if_t< FunctionInfo< T >::IsMember, void >;

	template < typename T >
	using DisableIfMemberF = enable_if_t< !FunctionInfo< T >::IsMember, void >;

	template < typename T >
	using EnableIfFunction = enable_if_t< FunctionInfo< T >::IsFunction, void >;

	template < typename T >
	using DisableIfFunction = enable_if_t< !FunctionInfo< T >::IsFunction, void >;
}

//==========================================================================
//
//==========================================================================
template < typename Return = void, typename... Args >
class Invoker
{
public:

	template < typename Object >
	using MemberFunction     = Return( Object::* )( Args... );
	using StaticFunction     = Return( * )( Args... );
	using InvocationFunction = Return( * )( void*, void*, Args&... );
	using Signature          = Return( Args... );

	/// <summary>
	/// 
	/// </summary>
	Invoker()
		: m_Object( nullptr )
		, m_Function( nullptr )
		, m_Invocation( nullptr )
	{ }

	/// <summary>
	/// 
	/// </summary>
	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	Invoker( Lambda a_Lambda )
		: m_Object( reinterpret_cast< void* >( &a_Lambda ) )
		, m_Function( nullptr )
		, m_Invocation( FunctorLambda< Lambda > )
	{ }

	/// <summary>
	/// 
	/// </summary>
	template < typename Object >
	Invoker( Object* a_ObjectInstance, MemberFunction< Object > a_MemberFunction )
		: m_Object( a_ObjectInstance )
		, m_Function( reinterpret_cast< void*& >( a_MemberFunction ) )
		, m_Invocation( FunctorMember< Object > )
	{ }

	/// <summary>
	/// 
	/// </summary>
	template < typename Object >
	Invoker( Object& a_ObjectInstance, MemberFunction< Object > a_MemberFunction )
		: m_Object( &a_ObjectInstance )
		, m_Function( reinterpret_cast< void*& >( a_MemberFunction ) )
		, m_Invocation( FunctorMember< Object > )
	{ }

	/// <summary>
	/// 
	/// </summary>
	Invoker( StaticFunction a_StaticFunction )
		: m_Object( nullptr )
		, m_Function( a_StaticFunction )
		, m_Invocation( FunctorStatic )
	{ }
	
	/// <summary>
	/// 
	/// </summary>
	inline Return Invoke( Args... a_Args )
	{
		if ( !IsSet() )
		{
			return Return();
		}

		return static_cast< InvocationFunction >( m_Invocation )( m_Object, m_Function, a_Args... );
	}

	/// <summary>
	/// 
	/// </summary>
	inline Return operator()( Args... a_Args ) const
	{
		if ( !IsSet() )
		{
			return Return();
		}

		return static_cast< InvocationFunction >( m_Invocation )( m_Object, m_Function, a_Args... );
	}

	/// <summary>
	/// 
	/// </summary>
	inline bool operator==( const Invoker< Return, Args... >& a_Other ) const
	{
		return m_Invocation == a_Other.m_Invocation &&
			   m_Object     == a_Other.m_Object     &&
			   m_Function   == a_Other.m_Function;
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	inline bool operator==( Lambda a_Lambda ) const
	{
		return m_Invocation == FunctorLambda< Lambda >;
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename Object, typename = FunctionTraits::DisableIfLambdaF< Object > >
	inline bool operator==( const Object& a_Object ) const
	{
		return m_Object == &a_Object;
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename Object, typename = FunctionTraits::DisableIfLambdaF< Object > >
	inline bool operator==( const Object* a_Object ) const
	{
		return m_Object == a_Object;
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename Object >
	inline bool operator==( MemberFunction< Object > a_MemberFunction ) const
	{
		return m_Function == reinterpret_cast< void*& >( a_MemberFunction );
	}

	/// <summary>
	/// 
	/// </summary>
	inline bool operator==( StaticFunction a_StaticFunction ) const
	{
		return m_Function == a_StaticFunction;
	}

	/// <summary>
	/// 
	/// </summary>
	inline bool operator!=( const Invoker< Return, Args... >& a_Other ) const
	{
		return operator==(  a_Other );
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	inline void operator=( Lambda a_Lambda )
	{
		m_Object = reinterpret_cast< void* >( &a_Lambda );
		m_Function = nullptr;
		m_Invocation = FunctorLambda< Lambda >;
	}
	
	/// <summary>
	/// 
	/// </summary>
	inline void operator=( StaticFunction a_StaticFunction )
	{
		m_Object = nullptr;
		m_Function = a_StaticFunction;
		m_Invocation = FunctorStatic;
	}

	/// <summary>
	/// 
	/// </summary>
	inline bool IsSet() const { return m_Invocation; }

	/// <summary>
	/// 
	/// </summary>
	inline bool IsLambda() const { return !m_Function; }

	/// <summary>
	/// 
	/// </summary>
	inline bool IsMember() const { return m_Object && m_Function; }

	/// <summary>
	/// 
	/// </summary>
	inline bool IsStatic() const { return !m_Object; }

	/// <summary>
	/// 
	/// </summary>
	inline void Reset() const
	{
		m_Object = nullptr;
		m_Function = nullptr;
		m_Invocation = nullptr;
	}

private:

	/// <summary>
	/// 
	/// </summary>
	template < typename T >
	static inline Return FunctorLambda( void* a_LambdaInstance, void*, Args&... a_Args )
	{
		return ( reinterpret_cast< T* >( a_LambdaInstance )->T::operator() )( a_Args... );
	}

	/// <summary>
	/// 
	/// </summary>
	template < typename T >
	static inline Return FunctorMember( void* a_ObjectInstance, void* a_MemberFunction, Args&... a_Args )
	{
		union
		{
			void* In;
			MemberFunction< T > Out;
		} Converter;

		Converter.In = a_MemberFunction;
		return ( reinterpret_cast< T* >( a_ObjectInstance )->*Converter.Out )( a_Args... );
	}

	/// <summary>
	/// 
	/// </summary>
	static inline Return FunctorStatic( void*, void* a_StaticFunction, Args&... a_Args )
	{
		return static_cast< StaticFunction >( a_StaticFunction )( a_Args... );
	}

	//==========================================================================
	template < class, class...  > friend class Delegate;
	template < class T, class U > friend auto MakeInvoker( T*, U );
	template < class T, class U > friend auto MakeInvoker( T&, U );
	template < class T          > friend auto MakeInvoker( T     );

	//==========================================================================
	InvocationFunction m_Invocation;
	void*			   m_Object;
	void*			   m_Function;

};

//==========================================================================
template < typename T >
auto MakeInvoker( T a_Function )
{
	return FunctionTraits::ConvertToInvoker< T >::Type( a_Function );
}

//==========================================================================
template < typename T, typename U >
auto MakeInvoker( T* a_Object, U a_Member )
{
	return FunctionTraits::ConvertToInvoker< U >::Type( a_Object, a_Member );
}

//==========================================================================
template < typename T, typename U >
auto MakeInvoker( T& a_Object, U a_Member )
{
	return FunctionTraits::ConvertToInvoker< U >::Type( a_Object, a_Member );
}

typedef void* DelegateHandle;

//==========================================================================
//
//==========================================================================
template < typename Return = void, typename... Args >
class Delegate
{
public:

	using iterator       = typename list< Invoker< Return, Args... > >::iterator;
	using const_iterator = typename list< Invoker< Return, Args... > >::const_iterator;
	using InvokerType    = typename Invoker< Return, Args... >;
	using DelegateType   = typename Delegate< Return, Args... >;
	
	template < typename Object >
	using MemberFunction = Return( Object::* )( Args... );
	using StaticFunction = Return( * )( Args... );

	Delegate()
		: m_IsInvoking( false )
	{ }

	inline void Clear()
	{
		m_Invokers.clear();
		m_ToRemove.clear();
		m_IsInvoking = false;
	}

	inline size_t GetCount() const { return m_Invokers.size(); }

	inline bool IsInvoking() const { return m_IsInvoking; }

	inline const list< InvokerType >& GetInvocationList() const { return m_Invokers; }

	inline Return Invoke( size_t a_Index, Args... a_Args )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		m_IsInvoking = true;
		Return&& Result = move( ( *Iterator )( a_Args... ) );
		m_IsInvoking = false;
		CleanUp();
		return Result;
	}

	Return Invoke( DelegateHandle a_DelegateHandle, Args... a_Args )
	{
		m_IsInvoking = true;
		Return&& Result = move( ( *reinterpret_cast< InvokerType* >( a_DelegateHandle ) )( a_Args... ) );
		m_IsInvoking = false;
		CleanUp();
		return Result;
	}

	void InvokeAll( Args... a_Args )
	{
		m_IsInvoking = true;

		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			( *Iterator )( a_Args... );
		}

		m_IsInvoking = false;
		CleanUp();
	}

	void InvokeAll( vector< Return >& a_Output, Args... a_Args )
	{
		a_Output.reserve( m_Invokers.size() + a_Output.size() );

		m_IsInvoking = true;

		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			a_Output.push_back( ( *Iterator )( a_Args... ) );
		}

		m_IsInvoking = false;
		CleanUp();
	}

	inline InvokerType& operator[] ( size_t a_Index )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return *Iterator;
	}

	inline InvokerType& operator[] ( DelegateHandle a_DelegateHandle )
	{
		return *reinterpret_cast< InvokerType* >( a_DelegateHandle );
	}

	inline DelegateHandle Add( const InvokerType& a_Invoker )
	{
		m_Invokers.push_back( a_Invoker );
		return reinterpret_cast< DelegateHandle >( &m_Invokers.back() );
	}

	inline void Add( const DelegateType& a_Delegate )
	{
		m_Invokers.insert( m_Invokers.end(), a_Delegate.begin(), a_Delegate.end() );
	}

	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	inline DelegateHandle Add( Lambda a_Lambda )
	{
		m_Invokers.emplace_back( a_Lambda );
		return reinterpret_cast< DelegateHandle >( &m_Invokers.back() );
	}

	template < typename Object >
	inline DelegateHandle Add( Object* a_Object, MemberFunction< Object > a_MemberFunction )
	{
		m_Invokers.emplace_back( a_Object, a_MemberFunction );
		return reinterpret_cast< DelegateHandle >( &m_Invokers.back() );
	}

	template < typename Object >
	inline DelegateHandle Add( Object& a_Object, MemberFunction< Object > a_MemberFunction )
	{
		m_Invokers.emplace_back( a_Object, a_MemberFunction );
		return reinterpret_cast< DelegateHandle >( &m_Invokers.back() );
	}

	inline DelegateHandle Add( StaticFunction a_StaticFunction )
	{
		m_Invokers.emplace_back( a_StaticFunction );
		return reinterpret_cast< DelegateHandle >( &m_Invokers.back() );
	}

	DelegateHandle Insert( const const_iterator& a_Where, const InvokerType& a_Invoker )
	{
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.insert( a_Where, a_Invoker ) );
	}

	inline void Insert( const const_iterator& a_Where, const DelegateType& a_Delegate )
	{
		m_Invokers.insert( a_Where, a_Delegate.begin(), a_Delegate.end() );
	}

	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	inline DelegateHandle Insert( const const_iterator& a_Where, Lambda a_Lambda )
	{
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( a_Where, a_Lambda ) );
	}

	template < typename Object >
	inline DelegateHandle Insert( const const_iterator& a_Where, Object* a_Object, MemberFunction< Object > a_MemberFunction )
	{
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( a_Where, a_Object, a_MemberFunction ) );
	}

	template < typename Object >
	inline DelegateHandle Insert( const const_iterator& a_Where, Object& a_Object, MemberFunction< Object > a_MemberFunction )
	{
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( a_Where, a_Object, a_MemberFunction ) );
	}

	inline DelegateHandle Insert( const const_iterator& a_Where, StaticFunction a_StaticFunction )
	{
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( a_Where, a_StaticFunction ) );
	}

	DelegateHandle Insert( size_t a_Index, const InvokerType& a_Invoker )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( Iterator, a_Index ) );
	}

	void Insert( size_t a_Index, const DelegateType& a_Delegate )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		m_Invokers.insert( Iterator, a_Delegate.begin(), a_Delegate.end() );
	}

	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	DelegateHandle Insert( size_t a_Index, Lambda a_Lambda )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( Iterator, a_Lambda ) );
	}

	template < typename Object >
	DelegateHandle Insert( size_t a_Index, Object* a_Object, MemberFunction< Object > a_MemberFunction )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( Iterator, a_Object, a_MemberFunction ) );
	}

	template < typename Object >
	DelegateHandle Insert( size_t a_Index, Object& a_Object, MemberFunction< Object > a_MemberFunction )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( Iterator, a_Object, a_MemberFunction ) );
	}

	DelegateHandle Insert( size_t a_Index, StaticFunction a_StaticFunction )
	{
		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		return reinterpret_cast< DelegateHandle >( &*m_Invokers.emplace( Iterator, a_StaticFunction ) );
	}

	bool Remove( size_t a_Index )
	{
		if ( a_Index >= m_Invokers.size() )
		{
			return false;
		}

		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );

		if ( m_IsInvoking )
		{
			m_ToRemove.insert( Iterator );
		}
		else
		{
			m_Invokers.erase( Iterator );
		}

		return true;
	}

	bool Remove( const InvokerType& a_Invoker )
	{
		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			if ( *Iterator == a_Invoker )
			{
				if ( m_IsInvoking )
				{
					m_ToRemove.insert( Iterator );
				}
				else
				{
					m_Invokers.erase( Iterator );
				}

				return true;
			}
		}

		return false;
	}

	bool Remove( DelegateHandle a_DelegateHandle )
	{
		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			if ( &*Iterator == a_DelegateHandle )
			{
				if ( m_IsInvoking )
				{
					m_ToRemove.insert( Iterator );
				}
				else
				{
					m_Invokers.erase( Iterator );
				}
			}
		}

		return false;
	}

	bool Remove( const const_iterator& a_Where )
	{
		if ( !( a_Where >= m_Invokers.begin() && a_Where < m_Invokers.end() ) )
		{
			return false;
		}

		if ( m_IsInvoking )
		{
			m_ToRemove.insert( a_Where );
		}
		else
		{
			m_Invokers.erase( a_Where );
		}

		return true;
	}

	bool ForceRemove( size_t a_Index )
	{
		if ( a_Index >= m_Invokers.size() )
		{
			return false;
		}

		auto Iterator = m_Invokers.begin();
		advance( Iterator, a_Index );
		m_Invokers.erase( Iterator );
		return true;
	}

	bool ForceRemove( const InvokerType& a_Invoker )
	{
		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			if ( *Iterator == a_Invoker )
			{
				m_Invokers.erase( Iterator );
				return true;
			}
		}

		return false;
	}

	bool ForceRemove( DelegateHandle a_DelegateHandle )
	{
		for ( auto Iterator = m_Invokers.begin(); Iterator != m_Invokers.end(); ++Iterator )
		{
			if ( &*Iterator == a_DelegateHandle )
			{
				m_Invokers.erase( Iterator );
				return true;
			}
		}

		return false;
	}

	bool ForceRemove( const const_iterator& a_Where )
	{
		if ( !( a_Where >= m_Invokers.begin() && a_Where < m_Invokers.end() ) )
		{
			return false;
		}

		m_Invokers.erase( a_Where );
		return true;
	}

	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	bool RemoveAll( Lambda a_Lambda )
	{
		return false;
	}

	template < typename Object >
	bool RemoveAll( Object* a_Object )
	{
		return false;
	}

	template < typename Object >
	bool RemoveAll( MemberFunction< Object > a_MemberFunction )
	{
		return false;
	}

	bool RemoveAll( StaticFunction a_StaticFunction )
	{
		return false;
	}

	void operator+=( const InvokerType& a_Invoker )
	{
		m_Invokers.push_back( a_Invoker );
	}

	void operator+=( const DelegateType& a_Delegate )
	{

	}
	
	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	void operator+=( Lambda a_Lambda )
	{

	}

	void operator+=( StaticFunction a_StaticFunction )
	{

	}

	void operator-= ( size_t a_Index )
	{

	}

	void operator-= ( DelegateHandle a_DelegateHandle )
	{

	}

	void operator-= ( const const_iterator& a_Where )
	{

	}

	void operator-= ( const InvokerType& a_Invoker )
	{

	}
	
	template < typename Lambda, typename = FunctionTraits::EnableIfLambdaF< Lambda > >
	void operator-= ( Lambda a_Lambda )
	{

	}

	void operator-= ( StaticFunction a_StaticFunction )
	{

	}

	inline const_iterator begin() const
	{
		return m_Invokers.begin();
	}

	inline const_iterator end() const
	{
		return m_Invokers.end();
	}

	inline iterator begin()
	{
		return m_Invokers.begin();
	}

	inline iterator end()
	{
		return m_Invokers.end();
	}

private:

	void CleanUp()
	{
		for ( auto Iterator = m_ToRemove.begin(); Iterator != m_ToRemove.end(); ++Iterator )
		{
			m_Invokers.erase( *Iterator );
		}

		m_ToRemove.clear();
	}

	template < class... T > friend auto MakeDelegate( T... );

	list< InvokerType >   m_Invokers;
	set< const_iterator > m_ToRemove;
	bool                  m_IsInvoking;

};

struct A
{
	int num = 0;

	void foo()
	{
		num = 1;
	}

	bool foo1( int a )
	{
		return false;
	}
};

double func( int b )
{
	return b * 10.0;
}

int otherfunc()
{
	return 0;
}

int main()
{
	A a;
	A b;

	auto lambda = [&]( int a ) { return true; };
	auto lambda1 = [=]( int b ) { return true; };
	auto invoker0 = MakeInvoker( func );
	auto invoker1 = MakeInvoker( lambda1 );
	auto invoker2 = MakeInvoker( a, &A::foo1 );
	bool test = invoker0 == otherfunc;

	Delegate< bool, int > del1;
	del1 += invoker1;
	DelegateHandle handle = del1.Insert( del1.begin(), &a, &A::foo1 );
	del1.Invoke( handle, 1 );
}