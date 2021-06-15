#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <string>
#include <vector>

#include "color.h"

namespace CoreAGI {

  class Table {

    const std::string HEAD;
    const std::string VERT;
    const std::string TAIL;

    std::vector< std::vector< std::string > > table;
    std::vector< std::string >                row;
    std::vector< unsigned >                   W;
    int                                       Nc;

  public:

    Table( std::string head="", std::string vert="|", std::string tail="" ):
      HEAD{ head },VERT{ vert },TAIL{ tail },table{},row{},Nc{ 0 }{}

    void nextRow(){
      Nc = std::max< int >( Nc, row.size() );
      table.push_back( row );
      row.clear();
    }

    unsigned size () const { return table.size();  }
    unsigned empty() const { return table.empty(); }

    void clear(){ table.clear(); Nc = 0; }

    void nextCol( std::string data ){ row.push_back( data ); }

    void format( char SPACE = ' ' ){
                                                                                                                              /*
      Calculate column`s width:
                                                                                                                              */
      for( int col = 0; col < Nc; col++ ){
        int Wi{ 0 }; // :width of i`th column
//      for( auto& row: table ) if( col < row.size() ) Wi = std::max< int >( Wi, row.at( col ).length() );
        for( auto& row: table ) if( col < int( row.size() ) ) Wi = std::max< int >( Wi, actualLength( row.at( col ) ) );
        W.push_back( Wi );
      }
    }

    std::string operator[]( unsigned row ){
      if( row >= table.size() ) return "";
      std::stringstream S;
      for( int i = 0; auto cell: table[ row ] ){
        const int Wi = W.at( i );
        const int delta = cell.length() - actualLength( cell );
        S << ( ( i == 0 ) ? HEAD : VERT ) << std::setw( Wi + delta ) << std::left << cell;
        i++;
      }
      S << TAIL;
      return S.str();
    }

    auto begin(){ return table.begin(); }
    auto end  (){ return table.end();   }

  };//class Table

}

#endif // TABLE_H_INCLUDED
