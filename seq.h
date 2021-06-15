                                                                                                                              /*
  Keep sequence of entities as sequence of identities.
  Note: not intended to keep names/strings.

  This implementation based on std::vector as undelying container,
  but ability to use alternative implementation assumed.

  2020.07.24 Added constructors and assignment for basic_string_view< Identity >

  2020.07.27 Added method `process` and removed conversion to std::string and comparizon with string

  2020.07.29 Refactored

  2020.12.21 Seq.span() added

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef SEQUENCE_H_INCLUDED
#define SEQUENCE_H_INCLUDED

#include <string>
#include <span>                                                                                                // [+] 2020.12.21
#include <string_view>                                                                                         // [+] 2020.07.24

#include "config.h"
#include "codec.h"                                                                                             // [+] 2020.07.29

namespace CoreAGI {

  class Seq {

    std::vector< Identity > seq;

  public:

    Seq(): seq{}{}

//    Seq( FILE* src ): seq{}{                                                                                 // [+] 2020.07.29
//                                                                                                                            /*
//      Read encoded identities of sequence items from file:
//                                                                                                                            */
//      constexpr char EOL{ '\0' }; // :End-Of-Line
//      char separator{ EOL };
//      for(;;){
//        const Encoded< Identity > EncodedId{ src, &separator };
//        if( not bool( EncodedId ) ) break;
//        seq.push_back( Identity( EncodedId ) );
//        if( separator == EOL ) break;
//      }
//    }

    Seq( const Seq& Q ): seq{ Q.seq }{}

    Seq& operator = ( const Seq& Q ){ seq = Q.seq; return *this; }

    auto span() const { return std::span( seq ); }                                                             // [+] 2020.12.21

    void clear(){ seq.clear(); }                                                                               // [+] 2020.07.29

    explicit operator bool() const { return seq.size() > 0; }                                                  // [+] 2020.07.29

    Seq& operator = ( const std::vector< Identity >& Q ){ seq = Q; return *this; }                             // [m] 2020.07.29

    bool operator == ( const Seq& Q ) const { return seq == Q.seq; }
    bool operator != ( const Seq& Q ) const { return seq != Q.seq; }

    Seq& operator += ( Identity i ){ seq.push_back( i ); return *this; }

    Seq& append( Identity i ){ seq.push_back( i ); return *this; }

    Identity operator[]( size_t i ) const { return seq.at( i ); }

    size_t size() const { return seq.size(); }

    void reserve( unsigned capacity ){ seq.reserve( capacity ); }

    void shrink(){ seq.shrink_to_fit(); }

    void process( std::function< bool( Identity ) > f ) const { for( auto& e: seq ){ if( not f(e) ) return; } } //[+] 2020.07.27

    auto begin() const { return seq.begin(); }
    auto end  () const { return seq.end  (); }

  };

}//namespace

#endif // SEQUENCE_H_INCLUDED
