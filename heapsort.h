#ifndef HEAPSORT_H_INCLUDED
#define HEAPSORT_H_INCLUDED

#include <functional>

namespace CoreAGI {


  //template< typename Elem > inline void heapSortSwap( Elem& u, Elem& v ){
  //  Elem t( u );
  //  u = v;
  //  v = t;
  //}

  //template< typename Elem > inline void heapSortDown( Elem* elem, const unsigned N, unsigned k, std::function< int( size_t, size_t ) > cmp ){
  template< typename Elem > inline void
  heapSortDown( Elem* elem, const unsigned N, unsigned k, std::function< int( const Elem&, const Elem& ) > cmp ){
    while( k <= N / 2 ){
      unsigned j = 2 * k;
      if( j < N && cmp( elem[ j ], elem[ j+1 ] ) < 0 ) j++;
      //if( cmp( elem[ k ], elem[ j ] ) < 0 ) heapSortSwap< Elem >( elem[ k ], elem[ j ] ); else break;
      if( cmp( elem[ k ], elem[ j ] ) < 0 ) std::swap< Elem >( elem[ k ], elem[ j ] ); else break;
      k = j;
    }
  }


  template< typename Elem > void heapSort( Elem* elem, unsigned count, std::function< int( const Elem&, const Elem& )> cmp ){
    if( count < 2 ) return;  // :no data to sort
    unsigned N = count - 1;  // :last element
    unsigned k = N / 2;
    k++;                     // :compensate the first use of 'k--'
    do {
      k--;
      heapSortDown< Elem >( elem, N, k, cmp );
    } while( k > 0 );
    while( N > 0 ){
      //if( N != 0 ) heapSortSwap< Elem >( elem[ 0 ], elem[ N ] );
      if( N != 0 ) std::swap< Elem >( elem[ 0 ], elem[ N ] );
      N--;
      heapSortDown< Elem >( elem, N, 0, cmp );
    }
  }

}//CoreAGI

#endif // HEAPSORT_H_INCLUDED
