                                                                                                                              /*
  Stack implemented as wrapper over std::vector

  2020.05.06  Initial version

  2020.05.29  Rewritten using fixed size array instead of vector;
              string() method modified to use string( T );
              access by index from top added.

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

#include <array>
#include <vector>
#include <sstream>
#include <string>

namespace CoreAGI {

  template< typename T, unsigned CAPACITY > class Stack {

    T        data[ CAPACITY ];
    unsigned len;

  public:

    Stack(): data{}, len{ 0 }{};

    bool empty() const { return len == 0; }

    const std::vector< T > operator()() const {
      std::vector< T > V;
      for( unsigned i = 0; i < len; i++ ) V.push_back( data[i] );
      return V;
    }

    void clear(){ len = 0; }

    void push( T t ){ assert( len < CAPACITY ); data[ len++ ] = t; }

    unsigned size() const { return len; }

    T& top(){ assert( len > 0 ); return data[ len - 1 ]; }

    T& operator[]( unsigned depth ){
      int i = int( len ) - depth - 1;
      assert( i >= 0 );
      return data[ i ];
    }

    T pop(){ assert( len > 0 ); auto t = data[ len - 1 ]; len--; return t; }

    operator std::string() const {
      std::stringstream S;
      S << " [";
      for( unsigned i = 0; i < len; i++ ) S << " " << std::string( data[i] );
      S << " ]";
      return S.str();
    }

  };// class Stack

}//namespace CoreAGI

#endif // STACK_H_INCLUDED
