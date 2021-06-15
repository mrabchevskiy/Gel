                                                                                                                              /*

  Wrapper over set containers, provides interface that better reflects set nature.

  Undelying set can be bitset, or std::unordered set, or std::unordered set compatible container,
  particularly Malte Skarupke flat has based version.

  2020.07.28 Fixed version ( begin() & end() fixed - dependency of underlying set was missed)

  ________________

  TODO

  [-] Remove ability to use bitset as underlying set



________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef SET_H_INCLUDED
#define SET_H_INCLUDED

#include <cstdint>
#include <cassert>
#include <sstream>
#include <cstring>

#include <string>
#include <type_traits>
#include <unordered_set>

namespace CoreAGI {

  template<
    typename Elem            = uint32_t,
    typename Underlying      = std::unordered_set< Elem >,
    unsigned CAPACITY        = 0,
    unsigned MAX_LOAD_FACTOR = 75 /* :percent */
  >
  class Set {
                                                                                                                              /*
    Represents set of integers using std::unordered_set, or some compatible,
    or integer-as-bitset as underlying storage.
                                                                                                                              */
    Underlying U;

    void prolog(){
      if constexpr( std::is_unsigned< Underlying >::value ){
        static_assert(
          std::is_same< Underlying, uint64_t >::value or
          std::is_same< Underlying, uint32_t >::value or
          std::is_same< Underlying, uint16_t >::value or
          std::is_same< Underlying, uint8_t  >::value
        );
      } else {
        static_assert( CAPACITY > 0 );
      }
      if constexpr( std::is_same< Underlying, std::unordered_set< Elem > >::value ){
        U.max_load_factor( 1.0e-2*MAX_LOAD_FACTOR );  // :set maximal load factor
        printf( "\n %i buckets, max load factor %.3f", int( U.bucket_count() ), U.max_load_factor() );
      }
    }

  public:

    Set(): U{
      ( not std::is_unsigned< Underlying >::value ) and ( CAPACITY > 0 ) ? Underlying( CAPACITY ) : Underlying()
    }{
      prolog();
    }

    Set( std::initializer_list< Elem > L ): U{
      ( not std::is_unsigned< Underlying >::value ) and ( CAPACITY > 0 ) ? Underlying( CAPACITY ) : Underlying()
    }{
      prolog();
      for( auto& e:L ) incl( e );
    }

    Set( const Set& M ): U{ M.U }{ prolog(); }

    Set& operator = ( const Set& M ){ U = M.U; return *this; }

    Set& operator = ( std::initializer_list< Elem > L ){ for( auto& e:L ) U.insert( e ); }

    const Underlying& underlying() const { return U; }

    void clear(){
      if constexpr( std::is_unsigned< Underlying >::value ) U = 0;
      else U.clear();
    }

    bool empty() const {
      if constexpr( std::is_unsigned< Underlying >::value ) return not bool( U );
      else return U.empty();
    }

    bool contains( const Elem e ) const {
      if constexpr( not std::is_unsigned< Underlying >::value ){
        return U.find( e ) != U.end();
      } else {
        if( e >= 8*sizeof( Underlying ) ) return false;
        Underlying ONE = 1u << e;
        return U & ONE;
      }
    };

    bool incl( const Elem e ){
                                                                                                                              /*
      Returns `false` only if operation failed(for example because
      undelying container capacity exceeded), so if `e` already
      contained in the set, returns `true`:
                                                                                                                              */
      if constexpr( std::is_unsigned< Underlying >::value ){
        if( e > 8*sizeof( Underlying ) ) return false;
        const Underlying ONE{ 1 };
        U |= ONE << e;
      } else {
        U.insert( e ).second;
      }
      return true;
    };

    void excl( const Elem e ){
      if constexpr( std::is_unsigned< Underlying >::value ){
        if( e < 8*sizeof( Underlying ) ){
          const Underlying ONE{ 1 };
          U &= ~( ONE << e );
        }
      } else {
        U.erase( e );
      }
    };

    explicit operator std::string() const {
      if( empty() ) return "{}";
      std::stringstream S;
      S << "{"; for( const auto& e: (*this) ) S << ' ' << e; S << " }";
      return S.str();
    }

    explicit operator bool() const {
      if constexpr( std::is_unsigned< Underlying >::value ) return bool( U );
      return U.size() > 0;
    }

    unsigned size() const {
      if constexpr( not std::is_unsigned< Underlying >::value ) return unsigned( U.size() );
      unsigned n{ 0 };
      for( unsigned e = 0; e < 8*sizeof( Underlying ); e++ ) if( contains( e ) ) n++;
      return n;
    }

    static unsigned capacity(){
      if constexpr( std::is_unsigned< Underlying >::value ) return 8*sizeof( Underlying );
      return unsigned( CAPACITY * MAX_LOAD_FACTOR );
    }

    Set& operator += ( const Elem e ){ incl( e ); return *this; }
    Set& operator -= ( const Elem e ){ excl( e ); return *this; }

    Set& operator += ( const Set& M ){
      if constexpr( std::is_unsigned< Underlying >::value ) U &= M.U;
      else for( const auto& e: M ) incl( e );
      return *this;
    }

    Set& operator -= ( const Set& M ){
      if constexpr( std::is_unsigned< Underlying >::value ) U &= ~M.U;
      else for( const auto& e: M ) excl( e );
      return *this;
    }

    Set operator + ( const Elem e ) const { Set M( *this ); M.incl( e ); return M; }
    Set operator - ( const Elem e ) const { Set M( *this ); M.excl( e ); return M; }
    Set operator + ( const Set& M ) const { Set S{ *this }; S += M;      return S; }
    Set operator - ( const Set& M ) const { Set S{ *this }; S -= M;      return S; }

    Set operator ^ ( const Set& M ) const { // Symmetrical difference of two sets:
      Set S;
      for( const auto& e: (*this ) ) if( not M.contains( e ) ) S.incl( e );
      for( const auto& e: M        ) if( not   contains( e ) ) S.incl( e );
      return S;
    };

    bool operator [] ( const Elem e ) const { return contains( e ); };

    bool operator == ( const Set& M ) const {
      if constexpr( std::is_unsigned< Underlying >::value ) return U == M.U;
      if( size() != M.size() ) return false;
      for( const auto& e: M ) if( not contains( e ) ) return false;
      return true;
    };

    bool operator != ( const Set& M ) const {
      if constexpr( std::is_unsigned< Underlying >::value ) return U != M.U;
      if( size() != M.size() ) return true;
      for( const auto& e: M ) if( not contains( e ) ) return true;
      return false;
    };

    bool operator <= ( const Set& M ) const {
                                                                                                                              /*
      Returns `true` if this is a subset of M ot equal M; if M or this is empty, return `false`:
                                                                                                                              */
      if( empty() or M.empty() ) return false;
      if constexpr( std::is_unsigned< Underlying >::value ) return ( U == M.U ) or ( bool( M - (*this ) ) );
      if( size() > M.size() ) return false;
      for( const auto& e: (*this) ) if( not M.contains( e ) ) return false;
      return true;
    };

    bool operator >= ( const Set& M ) const {
                                                                                                                              /*
      Returns `true` if M is a subset of this or equal to this; if M or this is empty, return `false`:
                                                                                                                              */
      if( empty() or M.empty() ) return false;
      if constexpr( std::is_unsigned< Underlying >::value ) return ( U == M.U ) or ( bool( (*this ) - M ) );
      if( size() < M.size() ) return false;
      for( const auto& e: M ) if( not contains( e ) ) return false;
      return true;
    };

    struct Sentinel{};

    struct Iter {
      const Set& S;
      unsigned e;

      Iter( const Set& X ): S{ X }, e{ 0 }{
        for( ; e <= 8*sizeof( Underlying ); e++ ) if( S.contains( e ) ) break;
      }

      bool operator != ( const Sentinel& S ) const { return e < 8*sizeof( Underlying ); } // DUMMY

      Iter& operator ++(){
        for(;;){
          e++;
          if( e >= 8*sizeof( Underlying ) ) break;
          if( S.contains( e )             ) break;
        }
        return *this;
      }

      Elem operator*(){ return e; }
    };

    auto begin() const {
      if constexpr( std::is_unsigned< Underlying >::value ) return Iter( *this );                              // [m] 2020.07.28
      return U.begin();                                                                                        // [+] 2020.07.28
    }

    auto end() const {
      if constexpr( std::is_unsigned< Underlying >::value ) return Sentinel();                                 // [m] 2020.07.28
      return U.end();                                                                                          // [+] 2020.07.28
    }

  };// class Set

}//namespace CoreAGI

#endif // SET_H_INCLUDED
