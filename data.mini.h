                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

  2021.02.13 Initial version

  2021.05.31 contains(.) added

  2021.06.01 Rewritten

  2021.06.07 Added clear( Identity, bool )

  2021.06.13 Modified event processing
________________________________________________________________________________________________________________________________
                                                                                                                              */

#ifndef DATA_MINI_H_INCLUDED
#define DATA_MINI_H_INCLUDED


#include <cstring>

#include <functional>
#include <unordered_map>

#include "ancillary.h"
#include "def.h"

namespace CoreAGI::Data {

  using Key    = CoreAGI::Key;

  using Entity = Gnosis::Entity;

  constexpr unsigned CARGO_CAPACITY{ 16 };

  constexpr unsigned LAST{ CARGO_CAPACITY - 1 };

  struct Lex{ char data[ CARGO_CAPACITY ]; };

  union Cargo {

    static_assert( CARGO_CAPACITY > sizeof( int64_t ) );

    int64_t  integer;
    double   rational;
//  Identity identity;
    Lex      lex;

    Cargo(): integer{ 0 }{ lex.data[ LAST ] = 'n'; }

    explicit Cargo( int64_t  i ): integer { i }{ lex.data[ LAST ] = 'i'; }
    explicit Cargo( double   r ): rational{ r }{ lex.data[ LAST ] = 'r'; }
//  explicit Cargo( Identity e ): identity{ e }{ lex.data[ LAST ] = 'e'; }

    explicit Cargo( const char* s ){
      memset( lex.data, 0, CARGO_CAPACITY );
      unsigned i  { 0        };
      char*    out{ lex.data };
      for( const char* src = s; *src and i < LAST; src++, i++ ) *out++ = *src;
      assert( lex.data[ LAST ] == '\0' );
    }

    explicit Cargo( const std::string s ){
      memset( lex.data, 0, CARGO_CAPACITY );
      unsigned i  { 0        };
      char*    out{ lex.data };
      for( const char* src = s.c_str(); *src and i < LAST; src++, i++ ) *out++ = *src;
      assert( lex.data[ LAST ] == '\0' );
    }

    char type(){ return lex.data[ LAST ]; }

  };//Cargo
                                                                                                                              /*
  Quasy-abstract data storage base class:
                                                                                                                              */
  class Storage: public Ancillary {

  public:

    Storage( const char* title, Logger& logger, Gnosis& gnosis ): Ancillary( title, logger, gnosis ){}

    Storage            ( const Storage& ) = delete;
    Storage& operator= ( const Storage& ) = delete;

    bool contains( const Entity& obj, const Entity& atr ) const { return contains( key( obj, atr ) ); }

    virtual Key key( const Entity& obj, const Entity& atr ) const { return combination( Identity( obj ), Identity( atr ) ); }

    virtual Key key( const Identity& obj, const Identity& atr ) const { return combination( obj, atr ); }

    virtual bool   empty() const { return true; };
    virtual size_t size () const { return 0;    };

    virtual size_t clear(){ return 0; };                             // [m] 2021.06.07                         // [+] 2021.06.07

    virtual size_t change( const Identity& id, const Identity& id2, bool attribute ){ return 0; }              // [m] 2021.06.13

    virtual bool contains( const Key& key ) const { return false; }                                            // [+] 2021.05.31

    virtual void   put ( const Key& key,   const Cargo& val ){ assert( false ); }
    virtual void   excl( const Key& key                     ){ assert( false ); }

    virtual       Cargo* get( const Key& key )       { assert( false ); return nullptr; }
    virtual const Cargo* get( const Key& key ) const { assert( false ); return nullptr; }

    virtual void put ( const Entity& obj, const Entity& atr, const Cargo& val ){ put ( key( obj, atr ), val ); }
    virtual void excl( const Entity& obj, const Entity& atr, const Cargo& val ){ excl( key( obj, atr )      ); }

    virtual const Cargo* get( const Entity& obj, const Entity& atr ) const { return get( key( obj, atr ) ); }
    virtual       Cargo* get( const Entity& obj, const Entity& atr )       { return get( key( obj, atr ) ); }

    virtual bool save( const char* path ) const { log.vital( "`save()` not yet implemented" ); return false; }
    virtual bool load( const char* path )       { log.vital( "`load()` not yet implemented" ); return false; }

  };//class Storage
                                                                                                                              /*
  Minimalistic data storage:
                                                                                                                              */

  class Mini: public Storage {

    std::unordered_map< Key, Cargo > data;
    Identity                         onForget;

  public:

    Mini( const char* title, Logger& logger, Gnosis& gnosis): Storage( title, logger, gnosis ), data{}{
      log.vital( kit( "Cargo size: %lu", sizeof( Cargo ) ) ); // :just in case
      onForget = gnosis.onChangeIdIncl(
        [&]( const Identity& id, const Identity& id2, bool attribute )->void{ change( id, id2, attribute ); }
      );
    }

    virtual size_t size    (                ) const override { return data.size();          }
    virtual bool   empty   (                ) const override { return data.empty();         }
    virtual bool   contains( const Key& key ) const override { return data.contains( key ); }                  // [+] 2021.05.31

    virtual size_t clear() override {                                                                          // [m] 2021.06.07
      auto n{ data.size() };
      data.clear();
      return n;
    }

    virtual size_t change( const Identity& id, const Identity& id2, bool attribute ) override {                // [m] 2021.06.13
      std::vector< Key > K; // :list of keys to be replaced/removed
      if( id2 == CoreAGI::NIHIL ){
                                                                                                                              /*
        Entity referred by `id` will be deleted:
                                                                                                                              */
        for( const auto& [ k, v ]: data ){
          const auto& [ obj, atr ] = decombine( k );
          if( attribute ){ if( atr == id ) K.push_back( k ); }
          else           { if( obj == id ) K.push_back( k ); }
        }
        for( const auto& k: K ) data.erase( k );
      } else {
                                                                                                                              /*
        Entity referred by `id` will change ID to `id2`:
                                                                                                                              */
        for( const auto& [ k, v ]: data ){
          const auto& [ obj, atr ] = decombine( k );
          if( attribute ){ if( atr == id ) data.insert_or_assign( key( obj, id2 ), v ), K.push_back( k ); }
          else           { if( obj == id ) data.insert_or_assign( key( id2, atr ), v ), K.push_back( k ); }
        }
        for( const auto& k: K ) data.erase( k );
      }
      return K.size();
    }

    virtual Cargo* get( const Key& key ) override {
      const auto& it{ data.find( key ) };
      if( it == data.end() ) return nullptr;
      return &( it->second );
    }

    virtual const Cargo* get( const Key& key ) const override {
      const auto& it{ data.find( key ) };
      if( it == data.end() ) return nullptr;
      return &( it->second );
    }

    virtual void excl( const Key& key ) override { data.erase( key ); };

    virtual void put( const Key& key, const Cargo& val ) override {
      data.insert_or_assign( key, val );
    }

    virtual const Cargo* get( const Entity& obj, const Entity& atr ) const { return get( key( obj, atr ) ); }
    virtual       Cargo* get( const Entity& obj, const Entity& atr )       { return get( key( obj, atr ) ); }

    virtual void put ( const Entity& obj, const Entity& atr, const Cargo& val ){ put ( key( obj, atr ), val ); }
    virtual void excl( const Entity& obj, const Entity& atr, const Cargo& val ){ excl( key( obj, atr )      ); }

  };//class Mini

}//namespace CoreAGI::Mini

#endif // DATA-MINI_H_INCLUDED
