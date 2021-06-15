                                                                                                                              /*
  Class that keeps glossary as bidirectional map:

    Entity name <-> Entity ID

  Note: It is not a part of Gnosis object rather part og AGI unith that
        may include a few glossaries (on none), but particular glossary
        associated with exacly one Gnosis object, so reference to such one
        is a component of glossary.

  2020.07.30 Initial version

  2020.11.08 operator[] added as equivalent to lex()
             operator() added as equivalent to entity()

  2021.04.24 method `lexOrId` added

  2021.06.13 Modified event processing

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef GLOSSARY_H_INCLUDED
#define GLOSSARY_H_INCLUDED

#include <cstdio>

#include <filesystem>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

#include "ancillary.h"
#include "config.h"
#include "codec.h"
#include "color.h"
#include "flat_hash.h"
#include "gnosis.h"
#include "data.mini.h"

namespace CoreAGI {

  using namespace Config::glossary;

  using Dict = std::function< std::string( Identity ) >;

  class Glossary: public Ancillary {

    Data::Storage&                                 data;
    ska::flat_hash_map< Identity,    std::string > LEX;
    ska::flat_hash_map< std::string, Identity    > IDENTITY;
    Identity                                       onChangeId;

  public:

    Glossary( const char* title, Logger& logger, Gnosis& gnosis, Data::Storage& data ):
      Ancillary { title, logger, gnosis },
      data      { data                  },
      LEX       {                       },
      IDENTITY  {                       },
      onChangeId{                       }
    {
                                                                                                                              /*
      Assign congenital entities names:
                                                                                                                              */
      let( gnosis.ABSORB,    "ABSORB"     );
      let( gnosis.AND,       "AND"        );
      let( gnosis.ADJECTIVE, "ADJECTIVE"  );  // [+] 2021.04.29
      let( gnosis.ATTRIBUTE, "ATTRIBUTE"  );
      let( gnosis.CLEAR,     "CLEAR"      );
      let( gnosis.DECR,      "DECR"       );
      let( gnosis.DIFF,      "DIFF"       );
      let( gnosis.DIV,       "DIV"        );
      let( gnosis.EXCL,      "EXCL"       );
      let( gnosis.EXPL,      "EXPL"       );
      let( gnosis.FORGET,    "FORGET"     );
      let( gnosis.FORK,      "FORK"       );
      let( gnosis.FUNCTION,  "FUNCTION"   );
      let( gnosis.HERITABLE, "HRITABLE"   );
      let( gnosis.IF,        "IF"         );
      let( gnosis.IMMORTAL,  "IMMORTAL"   );
      let( gnosis.IMMUTABLE, "IMMUTABLE"  );
      let( gnosis.INCL,      "INCL"       );
      let( gnosis.INCR,      "INCR"       );
      let( gnosis.INTEGER,   "INTEGER"    );
      let( gnosis.LET,       "LET"        );
      let( gnosis.MULT,      "MULT"       );
      let( gnosis.MUTEX,     "MUTEX"      );
      let( gnosis.NAME,      "NAME"       );  // [+] 2021.06.03
      let( gnosis.NOUN,      "NOUN"       );  // [+] 2021.04.29
      let( gnosis.OPERATOR,  "OPERATOR"   );
      let( gnosis.OR,        "OR"         );
      let( gnosis.POP,       "POP"        );
      let( gnosis.PROD,      "PROD"       );
      let( gnosis.PROPER,    "PROPER"     );  // [+] 2021.04.29
      let( gnosis.QUOT,      "QUOT"       );
      let( gnosis.RATIONAL,  "RATIONAL"   );
      let( gnosis.REF,       "REF"        );
      let( gnosis.ROUTINE,   "ROUTINE"    );
      let( gnosis.RULE,      "RULE"       );
      let( gnosis.RUN,       "RUN"        );
      let( gnosis.SEQ,       "SEQ"        );
      let( gnosis.SEQUENCE,  "SEQUENCE"   );
      let( gnosis.STRING,    "STRING"     );
      let( gnosis.SUM,       "SUM"        );
      let( gnosis.SWAP,      "SWAP"       );
      let( gnosis.SYN,       "SYN"        );
      let( gnosis.VAL,       "VAL"        );
      let( gnosis.VERB,      "VERB"       );  // [+] 2021.04.29
      log.vital();
      log.vital( "Congenital entities:" );
      log.vital();
      for( const auto& e: gnosis.CONGENITAL ) log.vital( kit( "  %8u %s", e.id, lex( e, "c" ).c_str() ) );
      log.flush();
                                                                                                                              /*
      Set onChangeId:
                                                                                                                              */
      onChangeId = gnosis.onChangeIdIncl(
        [&]( const Identity& id, const Identity& id2, bool attribute )->void{ forget( id ); }
      );
    }

    Glossary( const Glossary& ) = delete;
    Glossary& operator = ( const Glossary& ) = delete;

   ~Glossary(){ gnosis.onChangeIdExcl( onChangeId ); }

    size_t size() const { return LEX.size(); }                                                                 // [+] 2020.07.30

    bool forget( const Identity& id ){
      // log.vital( kit( "[forget] ID %u ...", id ) ); log.flush();                                                // DEBUG
      const auto it = LEX.find( id );
      if( it == LEX.end() ){
        // log.vital( kit( "[forget] not found", id ) ); log.flush();                                                // DEBUG
        return false;
      }
      auto name = it->second;
      IDENTITY.erase( name );
      LEX     .erase( id   );
      // log.vital( "[forget] [ok]" ); log.flush();                                                       // DEBUG
      return true;
    }

    Gnosis::Entity entity( const std::string& name ){
      if( name.empty() ) return gnosis.none();
                                                                                                                              /*
      Returns existing entity or creates new one:
                                                                                                                              */
      const auto& it = IDENTITY.find( name );
      if( it != IDENTITY.end() ){
        const Identity id{ it->second };
        assert( gnosis.exist( id ) );
        return gnosis.recover( id ); // :known entity identified by name
      }
                                                                                                                              /*
      Create new entity and set name:
                                                                                                                              */
      Gnosis::Entity e = gnosis.entity();
      let( e, name ); // :set name
      return e;
    }

    Gnosis::Entity entity( const char* name ){                                                           // [+] 2020.07.30
      if( not name ) return gnosis.none();
      const std::string NAME{ name };
      if( NAME.empty() ) return gnosis.none();
      return entity( NAME );
    }

    Gnosis::Entity entity( const std::string& name, std::initializer_list< Gnosis::Entity > syndrome ){
      Gnosis::Entity subj{ entity( name ) };
      if( subj.id != CoreAGI::NIHIL ) for( const auto& sign: syndrome ) subj += sign;
      return subj;
    }

    Gnosis::Entity entity( const char* name, std::initializer_list< Gnosis::Entity > syndrome ){         // [+] 2020.07.30
      Gnosis::Entity subj{ entity( name ) };
      if( subj.id != CoreAGI::NIHIL ) for( const auto& sign: syndrome ) subj += sign;
      return subj;
    }

    Gnosis::Entity operator() ( const std::string& name ){ return entity( name ); }                            // [+] 2020.11.08
    Gnosis::Entity operator() ( const char*        name ){ return entity( name ); }                            // [+] 2020.11.08

    Gnosis::Entity known( const std::string& name ) const {
      if( name.empty() ) return gnosis.none();                                                          // [+] 2020.07.30
                                                                                                                              /*
      Returns existing entity or NIHIL`id one:
                                                                                                                              */
      const auto& it = IDENTITY.find( name );
      if( it == IDENTITY.end() ) return gnosis.none();
      const Identity id{ it->second };                 assert( id != CoreAGI::NIHIL );
      return gnosis.recover( id );
    }

    Gnosis::Entity known( const char* name ) const {
      if( not name ) return gnosis.none();
      const std::string NAME{ name };
      return known( NAME );
    }

    std::string key( const Gnosis::Entity& e, const char* arg = " " ) const {
      Encoded< Identity > key( e.id );
      std::stringstream S;
      if( strchr( arg, 'c' ) ) S << xRED << "[[" << RESET << WHITE << key.c_str() << xRED << "]]" << RESET;
      else                     S << "[[" << key.c_str() << "]]";
      return S.str();
    }

    std::string lex( const Gnosis::Entity& entity, const char* arg = " ", Dict dict = nullptr ) const {
      log.sure( gnosis.ID == entity.unit, kit( "Alien entity, gnosis`ID:%u, entity.unit: %u", gnosis.ID, entity.unit ) );
      const bool decorated = strchr( arg, 'c' );
      const auto& it = LEX.find( entity.id );
      if( it == LEX.end() ){
        if( strchr( arg, 'v' ) ){
                                                                                                                              /*
          Try to find attribute `NAME`:
                                                                                                                              */
          const Data::Cargo* cargo{ data.get( data.key( entity, gnosis.NAME ) ) };
          if( cargo ){ // name attribute should not contains spaces:
            if( decorated ) return( kit( "%s%s%s", CYAN, cargo->lex.data, RESET ) );
            return std::string( cargo->lex.data );
          }
        }
                                                                                                                              /*
        Search name in (optional) dictionary:
                                                                                                                              */
        if( dict ){
          const Identity id{ Identity( entity ) };
          const std::string name = dict( id );
          if( not name.empty() ){
            if( decorated ){
              std::stringstream S;
              S << WHITE << name << RESET;
              return S.str();
            }
            return name;
          }
        }
        return NIL;
      }
      auto name = it->second;
      bool quoted{ false };
      for( char symbol: name ){
        if( isalnum( symbol ) ) continue;
        if( symbol == '_'     ) continue;
        quoted = true;
        break;
      }
      if( quoted ){
        std::stringstream S;
        if( decorated ) S << xRED << "'" << RESET << GREEN << name << xRED << "'" << RESET;
        else            S <<         "'" << name                           << "'";
        return S.str();
      } else {
        if( decorated ){
          std::stringstream S;
          S << GREEN << name << RESET;
          return S.str();
        }
        return name;
      }
    }

    std::string syn( const Gnosis::Entity& entity, const char* arg = " ", Dict dict = nullptr ) const {
      const bool decorated = strchr( arg, 'c' );
      std::stringstream S;
      if( Identity( entity ) == CoreAGI::NIHIL ){
        if( not decorated ) return Symbol::EMPTY_SET;
        S << xRED << Symbol::EMPTY_SET << RESET;
        return S.str();
      }
                                                                                                                              /*
      Compose sequence of signs ID to be able sort them:
                                                                                                                              */
      std::vector< Identity > sequence;
      entity.S().process( [&]( const Gnosis::Entity& e )->bool{ sequence.push_back( e.id ); return true; } );
                                                                                                                              /*
      Sort sequence of ID ( VERB < non-VERB ):
                                                                                                                              */
      if( not sequence.empty() ) std::ranges::sort(
        sequence,
        [&]( Identity left, Identity right )->bool{
          assert( left  );
          assert( right );
          Gnosis::Entity L{ gnosis.recover( left  ) };
          Gnosis::Entity R{ gnosis.recover( right ) };
          const bool Lv{ L.is( gnosis.VERB ) };
          const bool Rv{ R.is( gnosis.VERB ) };
          if( Lv == Rv      ) return true;
          if( Lv and not Rv ) return true;
          return false;
        }
      );
                                                                                                                              /*
      Compose output as list of ref`s in the brackets:
                                                                                                                              */
//    if( decorated ) S << xRED << '(' << RESET; else S << '(';
      bool first{ true };
      for( const auto& id: sequence ){
        if( first ) first = false; else S << ' ';
        S << ref( gnosis.recover( id ), arg, dict );                                           // :recursion
      }
//    if( decorated ) S << xRED << ')' << RESET; else S << ')';
      return S.str();
    }//syndrome

    std::string ref( const Gnosis::Entity& entity, const char* arg = " ", Dict dict = nullptr ) const {
      const bool decorated = strchr( arg, 'c' );
      std::string name = lex( entity, arg, dict );
      if( not name.empty() ) return name;
                                                                                                                              /*
      Anonymous entity; compose syndrome without IMMUTABLE & IMMORTAL:
                                                                                                                              */
      auto syndrome = entity.S();
      if( not strchr( arg, '*' ) ){
        syndrome.excl( gnosis.IMMUTABLE );
        syndrome.excl( gnosis.IMMORTAL  );
      }
                                                                                                                              /*
      Try to search for `entity` by syndrome:
                                                                                                                              */
      Identity id{ gnosis.uniqueEntityId( syndrome ) };
                                                                                                                              /*
      Return encoded ID if `entity` can't be identified by syndrome:
                                                                                                                              */
//    if( id == CoreAGI::NIHIL ) return key( entity, arg );
      std::stringstream S;
      if( id == CoreAGI::NIHIL ){
        if( decorated ) S << xRED << Symbol::EMPTY_SET << RESET; else S << Symbol::EMPTY_SET;
        return S.str();
      }
      std::string t = syn( entity, arg, dict );
      if( t.empty() ) return key( entity, arg );
      if( decorated ) S << xRED << '(' << RESET << t << xRED << ')' << RESET;
      else            S <<         '('          << t         << ')';
      return S.str();
    }

    std::string lexOrId( const Gnosis::Entity& e, const char* arg = "", Dict dict = nullptr ) const {          // [+] 2021.04.24
                                                                                                                              /*
      Returns name of named entity or encoded ID of anonyvous entity
      prefixed by provided C-string if such one provided.
                                                                                                                              */
      auto name = lex( e, arg, dict );
      return name.empty() ? key( e, arg ) : name;
    }

    std::string definition( const Gnosis::Entity& subj, const char* arg = " ", Dict dict = nullptr ) const {
      assert( gnosis.exist( subj.id ) );
      const bool decorated = strchr( arg, 'c' );
      constexpr char SPACE{ ' ' };
      std::stringstream S;
                                                                                                                              /*
      Make copy of `arg` without `v`:
                                                                                                                              */
      char ARG[ 8 ]; memset( ARG, 0, 8 );
      unsigned i{ 0 };
      for( const char* symbol = arg; *symbol; symbol++ ){
        if( *symbol != 'v' ) ARG[ i++ ] = *symbol;
        if( i >= 8 ) break;
      }
                                                                                                                              /*
      Name entity or ID:
                                                                                                                              */
      S << lexOrId( subj, ARG, dict );
      if( decorated ) S << xRED << ':' << RESET; else S << ':';
                                                                                                                              /*
      Make list of non-attribute `Y` and list of attribures `A`:
                                                                                                                              */
      std::vector< Gnosis::Entity > A;
      std::vector< Gnosis::Entity > Y;
      subj.S().process(
        [&]( const Gnosis::Entity& sign )->bool {
          if( sign.is( gnosis.ATTRIBUTE ) ) A.push_back( sign ); else Y.push_back( sign );
          return true;
        }
      );
                                                                                                                              /*
      Apend attributes from `A` to `Y`:
                                                                                                                              */
      for( const auto& atr: A ) Y.push_back( atr );
                                                                                                                              /*
      Process list of signs (including attributes):
                                                                                                                              */
      for( const auto& sign: Y ){
        const auto type{ sign.type() };
        S << SPACE << ref( sign, arg, dict );
        if( bool( type ) ){
                                                                                                                              /*
          It is an attribute, so append `:=`:
                                                                                                                              */
          if( decorated ) S << xRED << ":=" << RESET; else S << ":=";
                                                                                                                              /*
          Append value:
                                                                                                                              */
          Data::Cargo* val = data.get( subj, sign );
          if( not val ){
            if( decorated ) S << YELLOW << Symbol::EMPTY_SET << RESET; else S << Symbol::EMPTY_SET;
            continue;
          }
          if( type == gnosis.INTEGER  ){
            if( decorated ) S << YELLOW << val->integer << RESET; else S << val->integer;
            continue;
          }
          if( type == gnosis.RATIONAL ){
            if( decorated ) S << WHITE << val->rational << RESET; else S << val->rational;
            continue;
          }
          if( type == gnosis.STRING   ){
            if( decorated ) S << xRED << '"' << RESET << CYAN << val->lex.data << xRED << '"' << RESET;
            else            S <<         '"'                  << val->lex.data         << '"';
            continue;
          }
                                                                                                                              /*
          Unknown type; expose EMPTY_SET as value, but using RED color as warning:
                                                                                                                              */
          if( decorated ) S << xRED << Symbol::EMPTY_SET << RESET; else S << Symbol::EMPTY_SET;
        }
      }//for sign
                                                                                                                              /*
      Entity`s sequence (if any):
                                                                                                                              */
      auto sequence = subj.Q();
      if( not sequence.empty() ){
        if( decorated ) S << xRED << " [" << RESET; else S << " [";
        sequence.process(
          [&]( const Gnosis::Entity& e )->bool{ S << ' ' << ref( e, arg ); return true; }
        );
        if( decorated ) S << xRED << " ]" << RESET; else S << " ]";
      }
                                                                                                                              /*
      Termila point:
                                                                                                                              */
      if( decorated ) S << xRED << " ." << RESET; else S << " .";
      std::string result = S.str();
      return result;
    }//definition

    std::string definition( const char* name, const char* arg = " ", Dict dict = nullptr ){
      const bool decorated = strchr( arg, 'c' );
      Gnosis::Entity subj = entity( name );
      if( subj.id == CoreAGI::NIHIL ){
        std::stringstream S;
        if( decorated ){
          S << YELLOW << Symbol::EMPTY_SET << RESET;
          return S.str();
        }
        return Symbol::EMPTY_SET;
      }
      return definition( subj, arg, dict );
    }

    std::string definition( const std::string& subj, const char* arg = " " ) const {
      return definition( subj.c_str(), arg );
    }

    std::string operator[]( const Gnosis::Entity& entity ) const { return lex( entity ); }                     // [+] 2020.11.08

    bool let( const Gnosis::Entity& entity, const std::string& lex = NIL ){
      log.sure( gnosis.ID == entity.unit, kit( "Alien entity, gnosis`ID:%u, entity.unit: %u", gnosis.ID, entity.unit ) );
                                                                                                                              /*
      Define, change, or destroy name of entity
                                                                                                                              */
      if( lex == NIL ){
                                                                                                                              /*
        Exclude `lex` and `id` from glossary:
                                                                                                                              */
        IDENTITY.erase( lex       );
        LEX     .erase( entity.id );
        return true;
      }

      const auto& it = LEX.find( entity.id );
      if( it != LEX.end() ){
                                                                                                                              /*
        This name alrerady presented; do it refer provided identity?
                                                                                                                              */
        std::string oldName = it->second;
                                                                                                                              /*
        Reject assigning `lex` to provided identity if this `lex` already refers another identity:
                                                                                                                              */
        if( IDENTITY[ oldName ] != entity.id ) return false;
                                                                                                                              /*
        Remove oldName from glossary:
                                                                                                                              */
        IDENTITY.erase( oldName );
      }
                                                                                                                              /*
      Add mapping name <-> identity into glossary:
                                                                                                                              */
      if( lex != NIL ){
        LEX     [ entity.id ] = lex;
        IDENTITY[ lex       ] = entity.id;
      }
      return true;
    }

    bool dump( const char* path ) const {                                                                   // [+] 20-21.04.29
      log( kit( "Dump as `%s`...", path ) ); log.flush();
      std::unordered_set< Identity > congenital;
      for( const auto& e: gnosis.CONGENITAL ) congenital.insert( e.id );
      FILE* out = fopen( path, "w" );
      if( not out ){ log.vital( kit( "File `%s` not found", path ) ); return false; }
      std::vector< Identity > data;
      gnosis.process(
        data,
        [&]( std::vector< Identity >& data )->bool{
          const unsigned L = data.size(); assert( L );
          if( not congenital.contains( data.front() ) ){
            for( int i=0; const auto id: data ){
              fprintf( out, "%s%s", lexOrId( gnosis.recover( id ) ).c_str(), i>0?" ":": ");
              i++;
            }
            fprintf( out, ";\n" );
          }
          return true;
        }
      );
      fclose( out );
      return true;
    }

    bool save( const char* path ) const {                                                                      // [+] 2020.07.30
      log( kit( "Save as `%s`...", path ) ); log.flush();
      FILE* out = fopen( path, "w" );
      if( not out ){ log.vital( kit( "File `%s` not found", path ) ); return false; }
      unsigned n{ 0 };
      for( const auto& it: LEX ){
        Encoded< Identity > key( it.first );
        fprintf( out, "%s %s\n", key.c_str(), it.second.c_str() );
        n++;
      }
      fclose( out );
      log( kit( "  [ok] stored %u names", n ) ); log.flush();
      return true;
    }

    bool load( const char* path ){
      log.vital( kit( "Load from `%s`...", path ) ); log.flush();
      FILE* src = fopen( path, "r" );
      if( not src ){ log.vital( kit( "  File `%s` not found", path ) ); log.flush(); return false; }
      LEX     .clear();
      IDENTITY.clear();
      char name[ Config::glossary::CAPACITY_OF_LEX ];
      unsigned n{ 0 };
      for(;;){
        Encoded< Identity > EncodedID( src );
        if( not bool( EncodedID ) ) break;
        unsigned length{ 0 };
        for(;;){
          const int c{ fgetc( src ) };
          if( c < 0 ) break; // :end of file
          const char symbol = c;
          if( symbol == '\n' ) break; // :end of record
          if( length < Config::glossary::CAPACITY_OF_LEX - 1 ) name[ length++ ] = symbol;
        }
        name[ length ] = '\0';
        log.sure( length > 0, kit( "Empty name of `%s`", EncodedID.c_str() ) );
        const Identity id{ Identity( EncodedID ) };
        //assert( gnosis.exists( id ) );
        //log.sure( gnosis.exists( id ), kit( "Entity not exists: `%s`", name ) );                             // [-] 2020.11.27
        if( not gnosis.exists( id ) ){                                                                         // [+] 2020.11.28
          std::stringstream S;
          S << "Entity not exists: `" << name << '`';
          log.abend( S.str() );
        }
        if( not gnosis.exists( id ) ){
          std::stringstream S;
          S << "Entity not exists: `" << name << "`";
          log.abend( S.str() );
        }
        const std::string lx{ name };
        // log.vital( kit( "  %-8s %8u", lx.c_str(), id ) );                                                            // DEBUG
        LEX     [ id ] = lx;
        IDENTITY[ lx ] = id;
        n++;
      }
      fclose( src );
      log.vital( kit( "  [ok] loaded %u names", n ) ); log.flush();
      return true;
    }

  };//Glossary

}//namespace CoreAGI

#endif // GLOSSARY_H_INCLUDED
