                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________


 2021.03.27 Inutial version

 2021.04.18 Updated
 2021.06.05 Updated
 2021.06.14 Updated

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef HMI_H_INCLUDED
#define HMI_H_INCLUDED

#include <cassert>
#include <cctype>

#include <algorithm>
#include <atomic>
#include <atomic>
#include <codecvt>
#include <iomanip>
#include <map>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "color.h"
#include "channel.h"
#include "data.mini.h"
#include "def.h"
#include "gnosis.h"
#include "glossary.h"
#include "logger.h"
#include "random.h"
#include "stack.h"
#include "codec.h"
#include "table.h"

//#include "scanner1.h" // :renamed hmi/Scanner.h
//#include "scanner2.h" // :renamed hmi/Scanner.cpp
#include "scanner.h" // :concatenated files hmi/Scanner.h + hmi/Scanner.cpp

namespace CoreAGI {

  std::string CONV( const wchar_t* lex ){
    char C[ 256 ]{ '\0' };
    const wchar_t* symbol{ lex };
    unsigned       i     { 0   };
    while( symbol and i < 255 ){ C[i] = *symbol; i++; symbol++; }
    assert( i <= 256 );
    C[i] = '\0';
    return std::string( C );
  };

  using Token = Gel::Token;

  struct Term {

    enum Tag: uint8_t {
      NOPE =  0, // :return empty set
      ANLG =  1, // :find analogical
      ANON =  2, // :create new anonimous entity
      BSRB =  3, // :absorb entity
      COND =  4, // :process list of variables in the `analogy`
      CNST =  5, // :name of the constant in the Claim
      CR_N =  6, // :switch to disabled entity creation mode
      CR_Y =  7, // :switch to anabled  entity creation mode
      DENY =  8, // :add pair ( subj, sign ) to set of `tabu` conditions
      DFNT =  9, // :compose definition
      ENTT = 10, // :entity
      EXCL = 11, // :exclude sign
      EXPL = 12, // :get explication
      E_ID = 13, // :expose entity ID
      GETA = 14, // :get attribute value
      IDNT = 15, // :encoded identity
      INCL = 16, // :include sign
      INFO = 17, // :expose general info
      KILL = 18, // :delete entity
      LIST = 19, // :push empty syndrome
      NAME = 20, // :set entity name using token.lex as a new name
      QUOT = 21, // :single quoted name
      SHOW = 22, // :show patterm item
      SKIP = 23, // :skip pattern item
      SLCT = 24, // :find using condition pattern
      SMLR = 25, // :get similar entities entity.A()
      SQNC = 26, // :assign sequence
      STOP = 27, // :finish execution and reset stack
      SYND = 28, // :get sydrome entity.S()
      TABU = 29, // :switch sign ID to -ID
      UNIQ = 30, // :find unique entity by syndrome
      VALA = 31, // :set attributes value
      VALI = 32, // :set integer    value
      VALN = 33, // :set integer    value
      VALR = 34, // :set empty      value
      VALS = 35, // :set string     value
      VRBL = 36  // :name of the variable in the Claim
    };

    static const char* LEX( Tag tag ){
      switch( tag ){
        case ANLG: return "ANLG";
        case ANON: return "ANON";
        case BSRB: return "BSRB";
        case CNST: return "CNST";
        case COND: return "COND";
        case CR_N: return "CR_N";
        case CR_Y: return "CR_Y";
        case DFNT: return "DFNT";
        case DENY: return "DENY";
        case ENTT: return "ENTT";
        case EXCL: return "EXCL";
        case EXPL: return "EXPL";
        case E_ID: return "E_ID";
        case GETA: return "GETA";
        case IDNT: return "IDNT";
        case INCL: return "INCL";
        case INFO: return "INFO";
        case KILL: return "KILL";
        case LIST: return "LIST";
        case NAME: return "NAME";
        case NOPE: return "NOPE";
        case QUOT: return "QUOT";
        case SHOW: return "SHOW";
        case SKIP: return "SKIP";
        case SLCT: return "SLCT";
        case SMLR: return "SMLR";
        case SQNC: return "SQNC";
        case STOP: return "STOP";
        case SYND: return "SYND";
        case TABU: return "TABU";
        case UNIQ: return "UNIQ";
        case VALA: return "VALA";
        case VALI: return "VALI";
        case VALN: return "VALN";
        case VALR: return "VALR";
        case VALS: return "VALS";
        case VRBL: return "VRBL";
        default  : break;
      }
      assert( false );
    };

    Tag          tag;
    Identity     id;   // :id entity (including operator`Id if it is reperesented by the entity)
    const Token* t;
    std::string  lex;  // :keep name of the entity or comment to operator

    Term(                               ): tag{ NOPE }, id{ CoreAGI::NIHIL }, t{ nullptr }, lex{                  } {}
    Term( Tag tag                       ): tag{ tag  }, id{ CoreAGI::NIHIL }, t{ nullptr }, lex{                  } {}
    Term( Tag tag, const Token* t       ): tag{ tag  }, id{ CoreAGI::NIHIL }, t{ t       }, lex{ CONV( t->val )   } {}
    Term( Tag tag, const std::wstring s ): tag{ tag  }, id{ CoreAGI::NIHIL }, t{ nullptr }, lex{ CONV( s.data() ) } {}

    Term( const Term& T ): tag{ T.tag }, id{ T.id }, t{ T.t }, lex{ T.lex }{}

    Term& operator= ( const Term& T ){
      tag  = T.tag;
      id   = T.id;
      t    = T.t;
      lex  = T.lex;
      return *this;
    }

    operator std::string() const {
      std::stringstream S;
      S << std::setw( 5 );
      if( id != CoreAGI::NIHIL  ){ Encoded< Identity > encodedId( id ); S << encodedId.c_str(); } else { S << ""; }
      S << " " << LEX( tag );
      if( not lex.empty() ) S << "  `" << lex << '`';
      return S.str();
    }//operator string

  };// struct Term

  std::vector       < std::string > MSG; // :errors/warning
  std::unordered_set< unsigned    > LOC; // :error symbol location

}//namespace CoreAGI::HMI


//#include "parser1.h" // :renamed hmi/Parser.h
//#include "parser2.h" // :renamed hmi/Parser.cpp
#include "parser.h"    // :concatenated hmi/Parswer.h + hmi/Parser.cpp

namespace CoreAGI {

  class Hmi {

    static constexpr unsigned CONFIRMATION_TIMEOUT{ 60 }; // :seconds
    static constexpr unsigned SEND_TIMEOUT        {  2 }; // :seconds

    using Channel = CoreAGI::Communication::Channel;

    Logger::Log                                   log;
    Gnosis&                                       gnosis;
    Glossary&                                     glossary;
    Data::Mini&                                   data;
    Gel::Scanner                                  scanner;
    Gel::Parser                                   parser;
    Channel                                       channel;
    std::atomic< bool >                           active;
    std::map< std::string, Identity >             unknown;   // :unknown named entitities
    std::set< std::string >                       expected;  // :expected to be known
    std::map< std::string, Identity >             variable;  // :set of variables
    std::vector< Gnosis::Entity >                 created;
    std::unordered_map< Identity, std::string >   DICT;
    std::vector< bool >                           SHOW;      // :show/skip pattern item in the results
    std::map< Identity, unsigned >                ORDV;      // :maps variable id -> variable index
    std::vector< std::vector< Gnosis::Entity > >  DENY;      // :set of pair that defines what must be rejected in `analogy`
    std::vector< std::string >                    result;
    std::string                                   messageId; // :part of datagram
                                                                                                                              /*
    Note: identical to one defined in the terminal.h
                                                                                                                              */
    enum Prefix: char {
      ORIGINAL = 'o',
      CORRECT  = 'c',
      ERROR    = 'e',
      FACTS    = 'f',
      PRIOR    = 'p',
      REPLY    = 'r',
      INFO     = 'i',
      END      = '.',
      QUIT     = '#',
    };

    bool send( char prefix, std::string message, bool reply = false ){
                                                                                                                              /*
      Data format:

        0123  4       56...      <- byte
          ID  prefix  statement  <- meaning
                                                                                                                              */
      char data[ Communication::DATA_CAPACITY ];
      memset( data, 0, Communication::DATA_CAPACITY );
      if( messageId.empty() ) messageId = "    ";
      for( unsigned i = 0; i < 4; i++ ) data[i] = reply ? messageId[i] : ' ';
      data[4] = prefix;
      const char* symbol = message.c_str();
      while( *symbol == ' ' ) symbol++;
      unsigned i = 4;
      while( *symbol ){
        data[++i] = *symbol;
        if( i+2 >= Communication::DATA_CAPACITY ) break;
        symbol++;
      }
      std::string msg( data );
      assert( not msg.empty() );
      Timer timer;
      while( timer.elapsed( Timer::SEC ) < SEND_TIMEOUT ){
        if( channel.push( msg ) ){
          if( prefix == END ){ log.vital( "Sent `end`" ); log.flush(); }
          return true;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
      }
      return false;
    }

  public:

    void reset(){
      expected.clear();
      unknown .clear();
      variable.clear();
      created .clear();
      DICT    .clear();
      SHOW    .clear();
      DENY    .clear();
      ORDV    .clear();
    }

    Hmi( Logger& logger, Gnosis& gnosis, Glossary& glossary, Data::Mini& data, int thisPort, const char* peerAddr, int peerPort ):
      log      { logger.log( "Hmi" )          },
      gnosis   { gnosis                       },
      glossary { glossary                     },
      data     { data                         },
      scanner  {                              },
      parser   { &scanner                     },
      channel  { thisPort, peerAddr, peerPort },
      active   {                              },
      unknown  {                              },
      created  {                              },
      DICT     {                              },
      result   {                              },
      messageId{ 0                            }
    {
      Timer timer;
      while( not channel.live() ){
        if( timer.elapsed() > 2000 ) log.sure( channel.live(), kit( "%s", channel.error().c_str() ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      }
      active = true;
      log.vital( "Active" );
    }

    bool live() const { return active.load(); }

    void notify(){ log.vital( "WARNING: Can't send data" ); log.flush(); }

    std::tuple< bool, std::string > forget( Identity id, std::string name ){
      // log.vital();                                                                                                   // DEBUG
      // log.vital( kit( "Forget `%s`, ID %u", name.c_str(), id ) ); log.flush();                                       // DEBUG
      if( not gnosis.exists( id ) ){
        std::string msg{ kit( "%sEntity %s %u not exists%s", YELLOW, name.c_str(), id, RESET ) };
        log.vital( msg ); log.flush(); // DEBUG
        return{ true, msg };
      }
      assert( gnosis.exists( id ) );                                                                                    // DEBUG
      auto e = gnosis.recover( id );
      const std::string msg {
        e.forget() ? kit( "%sDeleted %s%s",                   YELLOW, name.c_str(), RESET )
                   : kit( "%sEntity %s can not be deleted%s", RED,    name.c_str(), RESET )
      };
      log.vital( msg ); log.flush(); // DEBUG
      return std::make_tuple( false, msg );
    }

    std::string decorated() const {

      using namespace Symbol;

      auto color = [&]( const std::string& name )->const char* {
        if( expected.contains( name ) ) return xMAGENTA;
        if( unknown .contains( name ) ) return YELLOW;
        return GREEN;
      };

      constexpr const char SPACE{ ' ' };

      int               i   { 0 };
      std::string       prev{" "};
      std::stringstream S; S << RESET;
      for( const auto& lex: parser.SRC ){
        const char  o{ lex.front() };
        bool space{ false };
        if( i++ and not strchr( ":;,{()?]`", o ) ) space = true;
        if( lex  == ":=" or prev == ":="         ) space = false;
        if( lex  == "`"  or prev == "`"          ) space = false;
        if(                 prev == "~"          ) space = false;
        if( lex  == DOWNARROW                    ) space = false;
        if( prev == NOT                          ) space = false;
        if( space ) S << SPACE;
        if( o == '\'' ){
                                                                                                                              /*
          Quoted name:
                                                                                                                              */
          std::string body = lex.substr( 1, lex.length() - 2 );
          S << xRED << lex[0] << RESET << color( body ) << body << xRED << lex.back() << RESET;
        } else if( o == '"' ){
                                                                                                                              /*
          String value:
                                                                                                                              */
          std::string body = lex.substr( 1, lex.length() - 2 );
          S << xRED << lex[0] << RESET << WHITE << body << xRED << lex.back() << RESET;
        } else if( ( o == '-' or o == '+' ) and lex.length() > 1 ){
                                                                                                                              /*
          Signed number:
                                                                                                                              */
          S << WHITE << lex << RESET;
        } else if( isdigit( o ) ){
                                                                                                                              /*
          Unsigned number:
                                                                                                                              */
          S << WHITE << lex << RESET;
        } else if( lex.starts_with( "[[" ) and lex.ends_with( "]]" ) ){
                                                                                                                              /*
          Encoded ID:
                                                                                                                              */
          std::string body = lex.substr( 2, lex.length() - 4 );
          S << xRED << "[[" << RESET << WHITE << body << xRED << "]]" << RESET;

        } else if( lex == EMPTY_SET or lex == NOT or lex == EXCESS or lex == DOWNARROW or lex == RING ){
                                                                                                                              /*
          Special utf-8 symbols:
                                                                                                                              */
          S << xRED << lex << RESET;
        } else {
                                                                                                                              /*
          Rest cases:
                                                                                                                              */
          if( o == '`' or o == '?' or std::ispunct( o ) ) S << xRED << lex << RESET; // :special operators
          else S << color( lex ) << lex;
        }
        prev = lex;
      }
      S << RESET;
      return S.str();
    }

    void stop(){
      log.vital();
      log.vital( "Stopping communication channel.." );
      log.flush();
      channel.stop();
      log.vital( "Communication channel stopped" );
      log.flush();
      active.store( false );
    }

    bool interpret(){

      using Cargo = Data::Cargo;
                                                                                                                              /*
      Interpret RPN:
                                                                                                                              */
      auto note = [&]( bool success, const char* err ){
        assert( err );
        if( success ) return;
        std::stringstream S;
        S << xRED << err << RESET;
        result.push_back( S.str() );
      };
                                                                                                                              /*
      Stack elements are ID, negated ID ot zero that plays role of `stopper`
      that designates start of ID list.
                                                                                                                              */
      constexpr unsigned STACK_CAPACITY{ 1024 };
      Stack< int, STACK_CAPACITY > stack;

      const bool SELECT{ hasSLCT() };

      bool     rollback{ false };
      unsigned pos     { 0     }; // :current RPN position
      bool     BREAK;

      auto dict = [&]( Identity id )->std::string{ return DICT.contains( id ) ? DICT[ id ] : ""; };

      auto sure = [&]( bool condition, const char* note ){
                                                                                                                              /*
        Check condition:
                                                                                                                              */
        if( condition ) return;

        log.vital();
        log.vital( "[interpret::sure] trigged" ); log.flush();   // DEBUG
                                                                                                                              /*
        Expose RPN:
                                                                                                                              */
        {
          std::stringstream S;
          S << RED << "Interpretation failure: " << note << RESET;
          log.vital( S.str() );
        }
        log.vital( "| RPN:" );
        unsigned i{ 0 };
        for( const auto& term: parser.RPN ){
          std::stringstream S;
          S << "| " << std::string( term );
          if( i == pos ) S << RED << " <-=" << RESET;
          log.vital( S.str() );
          i++;
        }
        log.vital( "| Stack:" );
        for( const auto& id: stack() ){
          std::stringstream S;
          S << "| ";
          if ( id >  0 ){
             assert( gnosis.exists( id ) );                                                                                    // DEBUG
             S << "Y " << glossary.lexOrId( gnosis.recover(      id   ) );
          } else if( id <  0 ){
             assert( gnosis.exists( abs( id ) ) );                                                                                    // DEBUG
             S << "N " << glossary.lexOrId( gnosis.recover( abs( id ) ) );
          }
          else S << "----------------";
          log.vital( S.str() );
        }
        rollback = true;
        BREAK    = true;
      };

      auto push = [&]( Term term, bool invert = false ){
        sure( term.id != CoreAGI::NIHIL, kit( "push() NIHIL ID `%s`", std::string( term ).c_str() ).c_str() );
        stack.push( invert ? -int( term.id ) : int( term.id ) );
      };

      auto top = [&]()->Gnosis::Entity{
        sure( not stack.empty(), "top(): empty stack" );
        if( stack.empty() ){ log.flush(); std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); assert( false ); }
        auto id = stack.top();
        assert( gnosis.exists( id ) );                                                                                  // DEBUG
        return gnosis.recover( id );
      };

      auto pop = [&]()->Gnosis::Entity{
        sure( not stack.empty(), "pop(): empty stack" );
        if( stack.empty() ){ log.flush(); std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); assert( false ); }
        auto id = stack.pop();
        assert( gnosis.exists( id ) );                                                                                  // DEBUG
        return gnosis.recover( id );
      };

      auto stackContent = [&]()->std::string{
        std::stringstream S; S << '[';
        for( int i{ 0 }; int id: stack() ){
          if( i++ ) S << '|';
          if( id > 0 and gnosis.exists( id ) ){
            S << glossary.lexOrId( gnosis.recover( id ) );
          } else if( id < 0 and gnosis.exists( std::abs( id ) ) ){
            S << Symbol::NOT << glossary.lexOrId( gnosis.recover( std::abs( id ) ) );
          } else {
            S << Symbol::EMPTY_SET;
          }
        }
        S << "]";
        return S.str();
      };
                                                                                                                              /*
      _________________________________________________________________________
                                                                                                                              */
      auto synd = [&](){
        auto syndrome = pop().S();
        if( not syndrome.empty() ){
          for( const Identity id: syndrome.id() ){
            assert( gnosis.exists( id ) );                                                                              // DEBUG
            result.push_back( glossary.ref( gnosis.recover( id ), "c", dict ) );
          }
        }
      };

      auto expl = [&](){
        int n{ 0 };
        for( const auto& e: pop().E() ){
          result.push_back( glossary.ref( e, "c", dict ) );
          if( ++n > 64 ){ result.push_back( ".." ); break; }
        }
      };

      auto dfnt = [&](                  ){ result.push_back( glossary.definition( pop(), "c" ) );   };
      auto anon = [&]( const Term& term ){ push( term );                                            };
      auto incl = [&](                  ){ top() += pop();                                          };
      auto excl = [&](                  ){ top() -= pop();                                          };
      auto list = [&](                  ){ stack.push( 0      );                                    };
      auto tabu = [&](                  ){ stack.top() *= -1;                                       };

      auto setv = [&]( const Term& term ){
        auto atr = pop(); // :attribute entity
        auto obj = top(); // :subject entity
        if( not obj.is( atr ) ){
          obj += atr;
          if( not obj.is( atr ) ){
            result.push_back(
              kit( "Can't add attribute `%s` to `%s`", glossary.ref( atr ).c_str(), glossary.ref( obj ).c_str() )
            );
            return;
          }
        }
                                                                                                                              /*
        Deletion not require to know data type:
                                                                                                                              */
        if( term.tag == Term::VALN ){
          data.excl( data.key( obj, atr ) );
          return;
        }
        auto type = atr.type();
        if( not type ){
          if( atr.is( gnosis.ATTRIBUTE ) )
            result.push_back( kit( "Undefined type of the attribute `%s`", glossary.ref( atr, "c" ).c_str() ) );
          else
            result.push_back( kit( "`%s` is not an attribute", glossary.ref( atr, "c" ).c_str() ) );
          return;
        }
        const auto key{ data.key( obj, atr ) };
        switch( term.tag ){
          case Term::VALI:
            if( type == gnosis.INTEGER  ){ data.put( key, Cargo{ int64_t( std::stoll( term.lex ) ) } ); break; }
            if( type == gnosis.RATIONAL ){ data.put( key, Cargo{           std::stod( term.lex )   } ); break; }
            if( type == gnosis.STRING   ){ data.put( key, Cargo{                      term.lex     } ); break; }
            break;
          case Term::VALR:
            if( type == gnosis.RATIONAL ){ data.put( key, Cargo{          std::stod ( term.lex )   } ); break; }
            if( type == gnosis.INTEGER  ){ data.put( key, Cargo{ int64_t( std::stoll( term.lex ) ) } ); break; }
            if( type == gnosis.STRING   ){ data.put( key, Cargo{                      term.lex     } ); break; }
            break;
          case Term::VALS:
            if( type == gnosis.STRING   ){ data.put( key, Cargo{                      term.lex     } ); break; }
            if( type == gnosis.INTEGER  ){
              result.push_back(
                kit( "Invalid value `%s` of integer attribute `%`", term.lex.c_str(), glossary.ref( atr ).c_str() )
              );
              return;
            }
            if( type == gnosis.RATIONAL ){
              result.push_back(
                kit( "Invalid value `%s` of rational attribute `%`", term.lex.c_str(), glossary.ref( atr ).c_str() )
              );
              return;
            }
          default: assert( false ); // :invalid Term`s tag
        }//switch
        // assert( data.contains( data.key( obj, atr ) ) );                                                             // DEBUG                                                                        // DEBUG
      };//setv

      auto seta = [&]( const Term& term ){
        auto srcAtr = pop();
        auto srcObj = pop();
        auto atr    = pop();
        auto obj    = top();
        Cargo* val{ data.get( data.key( srcObj, srcAtr ) ) };
        if( not val ){
          result.push_back( kit( "There are no %s`%s", glossary.ref( srcObj ).c_str(), glossary.ref( srcAtr ).c_str() ) );
          return;
        }
        auto key  = data.key( obj, atr );
        auto type = atr.type(); // :target type
        if( not obj.is( atr ) ){
          obj += atr;
          if( not obj.is( atr ) ){
            result.push_back(
              kit( "Can't add attribute `%s` to `%s`", glossary.ref( atr ).c_str(), glossary.ref( obj ).c_str() )
            );
            return;
          }
        }
        switch( val->type() ){  // :value  type, char-encoded
          case 'i' :
            if( type == gnosis.INTEGER  ){ data.put( key, Cargo{                 val->integer    } ); return; }
            if( type == gnosis.RATIONAL ){ data.put( key, Cargo{ double        ( val->integer  ) } ); return; }
            if( type == gnosis.STRING   ){ data.put( key, Cargo{ std::to_string( val->integer  ) } ); return; }
            break;
          case 'r' :
            if( type == gnosis.INTEGER  ){ data.put( key, Cargo{        int64_t( val->rational ) } ); return; }
            if( type == gnosis.RATIONAL ){ data.put( key, Cargo{                 val->rational   } ); return; }
            if( type == gnosis.STRING   ){ data.put( key, Cargo{ std::to_string( val->rational ) } ); return; }
            break;
          case '\0' :
            if( type == gnosis.STRING   ){ data.put( key, Cargo{                 val->lex.data   } ); return; }
            if( type == gnosis.INTEGER  ){ result.push_back( "Can't convert string to integer"     ); return; }
            if( type == gnosis.RATIONAL ){ result.push_back( "Can't convert string to rational"    ); return; }
            break;
        }//switch src type
        result.push_back( kit( "%sInvalid attribute type:%s `%s`", xRED, RESET, glossary.ref( type ) ) );
      };

      auto geta = [&](){
        auto atr = pop();
        auto obj = top();
        if( not obj.is( atr ) ){
          result.push_back( kit(
            "%s%s is not an attribute of %s%s", YELLOW, glossary.ref( atr ).c_str(), glossary.ref( obj ).c_str(), RESET )
          );
          return;
        }
        Cargo* val{ data.get( data.key( obj, atr ) ) };
        if( not val ){
          result.push_back( Symbol::EMPTY_SET );
          return;
        }
        switch( val->type() ){
          case 'i' : result.push_back( kit( "%s%i%s",     YELLOW, val->integer,  RESET ) ); return;
          case 'r' : result.push_back( kit( "%s%f%s",     YELLOW, val->rational, RESET ) ); return;
          case '\0': result.push_back( kit( "%s\"%s\"%s", YELLOW, val->lex.data, RESET ) ); return;
          default  : break;
        }
        result.push_back( kit( "%s%s%s", xRED, Symbol::EMPTY_SET, RESET ) );
      };

      auto uniq = [&](){
        auto Y = gnosis.syndrome();
        auto N = gnosis.syndrome();
        for(;;){
          sure( not stack.empty(), "uniq: empty stack" );
          if( stack.empty() ){ log.flush(); std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); assert( false ); }
          int i = stack.pop();
          Encoded< Identity > encoded{ Identity( i ) };
          log.vital( kit( "uniq(): id: %5s, stack size %u", encoded.c_str(), stack.size() ) ); // DEBUG
          if( i == 0 ) break;
          if( i > 0  ){
            Identity id = i;
            assert( gnosis.exists( id ) );                                                                                    // DEBUG
            auto e = gnosis.recover( id );  assert( bool( e ) );
            Y.incl( e );
          } else {
            Identity id = abs( i );
            assert( gnosis.exists( id ) );                                                                                    // DEBUG
            auto e = gnosis.recover( id ); assert( bool( e ) );
            N.incl( e );
          }
        }
        //log.vital( kit( "uniq(): Y.size = %u, N.size = %u", Y.size(), N.size() ) ); // DEBUG
        auto e = gnosis.uniqueEntity( Y, N );
        if( bool( e ) ){
          Encoded< Identity > encoded{ Identity( e ) };
          //log.vital( kit( "uniq(): Found `%s`", encoded.c_str() ) ); log.flush(); // DEBUG
          stack.push( Identity( e ) );
        } else {
          auto e = gnosis.entity( Y );
          if( SELECT ){
            std::string name{ glossary.ref( e, "c", dict ) };
            variable[ name          ] = Identity( e );
            DICT    [ Identity( e ) ] = name;
            //log.vital( kit( "uniq(): Anonymous variable `%s` created", name.c_str() ) ); log.flush(); // DEBUG
          } else {
            Encoded< Identity > encoded{ Identity( e ) };
            //log.vital( kit( "uniq(): Anonymous entity [[%s]] created", encoded.c_str() ) ); log.flush(); // DEBUG
            created.push_back( e );
          }
          Y.process( [&]( const Gnosis::Entity& sign )->bool{ e += sign; return true; } );
          stack.push( Identity( e ) );
        }
      };

      auto kill = [&](){
        auto e{ pop() };
        auto [ done, msg ] = forget( Identity( e ), glossary.ref( e ) );
        result.push_back( msg );
      };

      auto smlr = [&](){
        const auto subj{ pop() };
        for( int n = 0; const auto& e: subj.A() ){
          const std::string rec = kit( "%s%s%s", YELLOW, glossary.ref( e ).c_str(), RESET );
          log.vital( rec );
          result.push_back( rec );
          if( ++n > 32 ){
            log.vital( rec );
            result.push_back( rec );
            break;
          }
        }
      };

      auto vrbl = [&]( const Term& term ){
        assert( term.id );
        push( term );
      };

      auto show = [&](){ SHOW.push_back( true  ); };
      auto skip = [&](){ SHOW.push_back( false ); };

      auto cond = [&](){
        for( int i = 0; const auto& w: parser.ORD ){
          auto name = CONV( w.data() );
          assert( variable.contains( name ) );
          auto id = variable[ name ];
          ORDV[ id ] = i;
          DENY.push_back( std::vector< Gnosis::Entity >() ); // :empty list that can be filled later
          // log.vital( kit( "[cond] %2u %s", i, glossary.def( gnosis.recover( id ), "cv" ).c_str() ) );                // DEBUG
          i++;
        }
      };

      auto deny = [&](){
        auto sign = pop();
        auto subj = stack.top();
        assert( ORDV.contains( subj ) );
        unsigned i = ORDV[ subj ];
        DENY.at( i ).push_back( sign );
      };

      auto analogy = [&]( const Gnosis::Sequence& pattern, const char* tracePath = nullptr ){

        log.vital( "[analogy] Pattern:" );
        unsigned i{ 0 };
        pattern.process(
          [&]( const Gnosis::Entity& e )->bool{
            log.vital( std::to_string( ++i ) + " " + glossary.definition( e, "cv", dict ) );
            return true;
          }
        );
        i = 0;
        log.vital( "[analogy] Deny:" );
        for( const auto& D: DENY ){
          i++;
          for( const auto& sign: D ) log.vital( kit( "%4u %s", i, glossary.ref( sign, "cv" ).c_str() ) );
        }
        Gnosis::Syndrome mask = gnosis.syndrome();    // :empty mask
//      std::vector< std::vector< Identity > > table; // :storage for results
        std::set< std::vector< Identity > > table;    // :storage for results
        Config::gnosis::spurt = true;
        Timer timer;
        bool done = gnosis.analogic(
          pattern,
          mask,
          [&]( const Gnosis::Sequence& group )->bool {
            std::vector< Identity > row;
            bool accept{ true };
            for( unsigned i = 0; i < group.size(); i++ ){
              auto subj = group[i];
              for( const auto& sign: DENY.at( i ) ) if( subj.is( sign ) ) accept = false;
              if( SHOW[i] ) row.push_back( Identity( subj ) );
            }
            if( accept ) table.insert( row );
            constexpr unsigned LIMIT{ 32 };
            return table.size() < LIMIT;
          },
          [&]( Identity id )->std::string {
            if( id == 0 ) return "NIHIL";
            if( DICT.contains( id ) ) return DICT[ id ];
            assert( gnosis.exists( id ) );                                                                                    // DEBUG
            return glossary.lexOrId( gnosis.recover( id ), "cv", dict );
          },
          tracePath
        );
        Config::gnosis::spurt = false;
        double t{ timer.elapsed() };
        const unsigned n = table.size();                              // :number of selected analogs
        const std::string msg{
          done ? kit( "%sDone in %.2f msec, %u analogies found%s", YELLOW, t, n, RESET )
               : kit( "%sFailed, elapsed %.2f msec%s",             RED,    t,    RESET )
        };
        log.vital( msg );
        result.push_back( msg );
        log.flush(); // DEBUG
        if( n > 0 ){
          log.vital( "Compose table.." ); log.flush();
          Table T( "| ", " | ", " |" );
                                                                                                                              /*
          Table header:
                                                                                                                              */
          int i{ 0 };
          pattern.process(
            [&]( const Gnosis::Entity& e )->bool{ if( SHOW[ i++ ] ) T.nextCol( DICT[ Identity( e ) ] ); return true; }
          );
          T.nextRow();
                                                                                                                              /*
          Table body:
                                                                                                                              */
          for( const auto& row: table ){
            for( auto id: row ){
              assert( gnosis.exists( id ) );                                                                                    // DEBUG
              T.nextCol( glossary.ref( gnosis.recover( id ), "cv", dict ) );
            }
            T.nextRow();
          }
          log.vital( "Format table and print it out.." ); log.flush();
          T.format();
          for( int i= 0; i < int( T.size() ); i++ ){
            std::string row{ T[i] };
            log.vital( row );
            result.push_back( row );
          }
        } else {
          result.push_back( std::string( YELLOW ) + Symbol::EMPTY_SET + RESET );
        }
        log.vital();
        log.vital( "Remove temporal entities:" );
        for( const auto& [ name, id ]: variable ) log.vital( kit( " %10u %s", id, name.c_str() ) );
        log.flush();
        for( const auto& [ name, id ]: variable ){ forget( id, name ); log.flush(); }
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );                                                    // NB DEBUG
        log.vital( "OK, temporary entities removed" );
        log.flush();

      };// analogy

      auto anlg = [&](){
        std::vector< Gnosis::Entity > P;
        while( not stack.empty() ) P.push_back( pop() );
        int n = P.size();
        log.vital( kit( "[anlg] Pattern consists of %u entities:", n ) );
        auto pattern = gnosis.sequence();
        //DICT.clear();
        for( int i = 0; i < n; i++ ){
          const auto& e{ P[ 2 - i ] };
          pattern += e;
          //std::string t{ CONV( parser.COL[i].data() ) };
          std::string name{ glossary.ref( e, "cv" ) };
          DICT.insert_or_assign( Identity( e ), name );
          log.vital( kit( " %8s", name.c_str() ) );
        }
        log.flush();
        // analogy( pattern, ( parser.src( '_' ) + ".trace" ).c_str() );                                                // DEBUG
        analogy( pattern );
      };

      auto slct = [&](){
        auto pattern = gnosis.sequence();
        // log.vital( "[slct] Pattern:" );
        for( const auto& [ name, id ]: variable ){
          assert( gnosis.exists( id ) );                                                                                // DEBUG
          pattern += gnosis.recover( id );
          // log.vital( kit( "  %s", glossary.definition( gnosis.recover( id ), "c", dict ).c_str() ) );
        }
        // analogy( pattern, ( parser.src( '_' ) + ".trace" ).c_str() );                                                // DEBUG
        analogy( pattern );
        // log.vital( "[slct] finished" ); log.flush();                                                                 // DEBUG
      };//slct

      auto name = [&]( const Term& T ){
        if( glossary.let( pop(), T.lex ) ) result.push_back( "Done" ); else note( false, "Name refers some entity" );
      };

      auto bsrb = [&]( const Term& T ){
        auto e    = pop();
        auto subj = top();
        if( not subj.absorb( e ) ){
          std::string msg( kit( "%s%s can not be absorbed%s", RED, glossary.ref( e ).c_str(), RESET ) );
          log.vital( msg );
          result.push_back( msg );
        }
      };

      auto info = [&](){
        std::stringstream S;
        S << "Total " << gnosis.size() << " entities, " << glossary.size() << " of them are named"
          << " and " << data.size() << " data values.";
        result.push_back( S.str() );
      };

      auto e_id = [&](){
        auto     e  = pop();
        Identity id = Identity( e );
        Encoded< Identity > encoded{ id };
        std::string msg{ kit( "%s%u %s[[%s%s%s]]%s", YELLOW, id, RED, WHITE, encoded.c_str(), RED, RESET ) };
        log.vital( msg ); log.flush();
        result.push_back( msg );
      };

      auto sqnc = [&](){
        std::vector< Gnosis::Entity > Q;
        while( stack.size() > 1 ) Q.push_back( pop() ); // :reversed sequence
        auto subj{ top() };
        auto sequence{ gnosis.sequence() };
        for( auto& e: std::ranges::reverse_view{ Q } ) sequence += e; // :iterate in reverse order
        subj.seq( sequence );
      };

      auto nope = [&](){  };

      log.vital( "Interpret RPN.." ); log.flush();                                                                      // DEBUG
      result.clear();
      BREAK = false;
      for( const auto& term: parser.RPN ){
        if( BREAK ) break;
        std::stringstream S; S << kit( "| %-30s", std::string( term ).c_str() ) << stackContent();                      // DEBUG
        switch( term.tag ){
//        case Term::NOPE: break;
          case Term::NOPE: nope(      ); break;
          case Term::CR_Y:               break;
          case Term::CR_N:               break;
          case Term::VRBL: vrbl( term ); break;
          case Term::CNST:
          case Term::QUOT:
          case Term::IDNT: push( term ); break;
          case Term::ANON: anon( term ); break;
          case Term::BSRB: bsrb( term ); break;
          case Term::COND: cond(      ); break;
          case Term::DENY: deny(      ); break;
          case Term::DFNT: dfnt(      ); break;
          case Term::EXCL: excl(      ); break;
          case Term::EXPL: expl(      ); break;
          case Term::E_ID: e_id(      ); break;
          case Term::SLCT: slct(      ); break;
          case Term::ANLG: anlg(      ); break;
          case Term::INCL: incl(      ); break;
          case Term::INFO: info(      ); break;
          case Term::KILL: kill(      ); break;
          case Term::LIST: list(      ); break;
          case Term::NAME: name( term ); break;
          case Term::STOP: BREAK = true; log.vital( "[stop]" ); log.flush(); break;
          case Term::SHOW: show(      ); break;
          case Term::SKIP: skip(      ); break;
          case Term::SQNC: sqnc(      ); break;
          case Term::SYND: synd(      ); break;
          case Term::SMLR: smlr(      ); break;
          case Term::TABU: tabu(      ); break;
          case Term::UNIQ: uniq(      ); break;
          case Term::GETA: geta(      ); break;
          case Term::VALA: seta( term ); break;
          case Term::VALI: setv( term ); break;
          case Term::VALN: setv( term ); break;
          case Term::VALR: setv( term ); break;
          case Term::VALS: setv( term ); break;
          default        : log.abend( kit( "[interpret] unexpected tag `%s`", Term::LEX( term.tag ) ) );
        }
        S << " => " << stackContent(); log.vital( S.str() ); log.flush();                                               // DEBUG
        pos++;
      }//for term
      if( rollback ){
        log.vital( "Failure - forgot created entities.." ); log.flush();
        std::map< Identity, std::string > M;
        for( auto& e: created ) M[ Identity( e ) ] = glossary.ref( e );
        for( const auto& [ id, name ]: M ) forget( id, name );
      }
      if( result.empty() ) result.push_back( kit( "%s%s%s",  YELLOW, Symbol::EMPTY_SET, RESET ) );
      return true;
    }//iterpret

    void parse( const unsigned char* source, unsigned length ){
      scanner.reset( source, length );
      parser.reset();
      MSG.clear();
      LOC.clear();
      parser.Parse();
    }

    bool hasSLCT() const {
      for( auto& term: parser.RPN ) if( term.tag == Term::SLCT ) return true;
      return false;
    }

    void assignKnownId(){
      bool allowCreateEntity{ true };
      for( auto& term: parser.RPN ){
                                                                                                                              /*
        Process names; at the end `unknown` contains unknown constant names,
        and `variables` contains variable names:
                                                                                                                              */
        switch( term.tag ){
          case Term::STOP:
            goto OUT;
          case Term::CR_Y:
            allowCreateEntity = true;
            break;
          case Term::CR_N:
            allowCreateEntity = false;
            break;
          case Term::QUOT:
                                                                                                                              /*
            NOTE: fall through (no `break`):
                                                                                                                              */
            term.lex = term.lex.substr( 1, term.lex.length() - 2 );
          case Term::NAME:
            break;
          case Term::CNST:
                                                                                                                              /*
            Try to identify; if not known and creation is prohibited, add to `expected` set:
                                                                                                                              */
            term.id = Identity( glossary.known( term.lex ) );
            if( term.id == CoreAGI::NIHIL ){
              unknown[ term.lex ] = CoreAGI::NIHIL; // :unknown entity
              if( not allowCreateEntity ) expected.insert( term.lex );
            }
            break;
          case Term::VRBL:
                                                                                                                              /*
            Variables a priory not presented in storage,
            so create one if it not yet presented in `variables`:
                                                                                                                              */
            term.id = CoreAGI::NIHIL;
            if( not variable.contains( term.lex ) ){
              variable[ term.lex ] = term.id;
              DICT    [ term.id  ] = term.lex;
            }
            break;
          case Term::IDNT:
            {
              const std::string root = term.lex.substr( 2, term.lex.length() - 4 ); // :NB fall through
              Encoded< Identity > encodedId( root.c_str() );
              Identity id = Identity( encodedId );
              if( gnosis.exist( id ) ) term.id = id;
            }
            break;
          default  : break;
        }
      }
      OUT:;
    }

    void createEntities(){
                                                                                                                              /*
      Create variable entities (if any):
                                                                                                                              */
      for( auto& [ name, id ]: variable ){
        auto e = gnosis.entity();
        created.push_back( e );
        variable[ name          ] = Identity( e );
        DICT    [ Identity( e ) ] = name;
        Encoded< Identity > encoded{ Identity( e ) };
        log.vital( kit( "Created variable [[%s]] %s%s%s", encoded.c_str(), WHITE, name.c_str(), RESET ) );
      }
                                                                                                                              /*
      Create unknown constant entities and assign missed ID:
                                                                                                                              */
      for( auto& term: parser.RPN ){
        switch( term.tag ){
          case Term::VRBL:
            log.sure(
              variable.contains( term.lex ),
              kit( "Term `%s`: `variable` don`t contains `%s`", std::string( term ).c_str(), term.lex.c_str() )
            );
            term.id = variable[ term.lex ];
            break;
          case Term::QUOT:
          case Term::CNST:
            if( term.id == CoreAGI::NIHIL ){
              assert( unknown.contains( term.lex ) );
              term.id = unknown[ term.lex ];
              if( term.id == CoreAGI::NIHIL ){
                auto   e = glossary.entity( term.lex );
                created.push_back( e );
                term.id  = Identity( e );
                unknown[ term.lex ] = term.id;
                Encoded< Identity > encoded{ Identity( e ) };
                log.vital( kit( "Created constant %5s `%s`", encoded.c_str(), term.lex.c_str() ) );
              }//if
            }//if
            break;
          default:
            break;
        }//switch tag
      }//for term
    }//createEntities

    bool authorized(){
      std::stringstream S;
      if( unknown.empty() and variable.empty() ){
        S << YELLOW << "Execute? Press " << RESET;
      } else {
        S << YELLOW << "Create " << ( unknown.size() + variable.size() ) << " entities" << xBLUE;
        bool first{ true };
        for( const auto& [ lex, id ]: unknown  ){ if( first ) first = false; else S << ','; S << ' ' << lex; }
        for( const auto& [ lex, id ]: variable ){ if( first ) first = false; else S << ','; S << ' ' << lex; }
        S << RESET << YELLOW << " and execute? Press " << RESET;
      }
      S << GREEN << 'y' << RESET << YELLOW << " or " << xRED << 'N' << RESET;
      std::string request{ S.str() };
      log.vital( kit( "%s AGI%s: %s", YELLOW, RESET, request.c_str() ) );
                                                                                                                              /*
      Send request, wait for feedback, parse until 'y' or 'n' detected or time expired:
                                                                                                                              */
      char decision{ '?' };
      do{
        if( not send( FACTS, request ) ){ decision = '?'; break; }
                                                                                                                              /*
        Get feedbaack:
                                                                                                                              */
        Timer timer;
        constexpr unsigned PAUSE{ 50 }; // :millisec
        while( timer.elapsed( Timer::SEC ) < 60.0 ){
          if( not channel.empty() ){
            auto feedback = channel.pull();
            // log.vital( kit( "Received `%s`", feedback.c_str() ) ); log.flush();                                      // DEBUG
            const char symbol = toupper( feedback.back() );
            if( symbol == 'Y' or symbol == 'N' ){ decision = symbol; break; }
          }
          std::this_thread::sleep_for( std::chrono::milliseconds( PAUSE ) );
        }
      } while( decision == '?' );
      switch( decision ){
        case 'Y': log.vital( kit( "%sUser%s: %sY%s", GREEN, RESET, GREEN, RESET ) ); return true;
        case 'N': log.vital( kit( "%sUser%s: %sN%s", GREEN, RESET, xRED,  RESET ) ); return false;
        case '?': log.vital( kit( "Can't get authorization"                     ) ); return false;
        default : assert( false );
      }
      return false; // DUMMY
    }

    void GelError( std::string_view source, bool peer = false ){
                                                                                                                              /*
      Colorize user input with highlited error point:
                                                                                                                              */
      std::stringstream S;
      S << CYAN;
      const unsigned len{ unsigned( source.size() ) };
      for( unsigned i = 0; i < len; i++ ){
        if( LOC.contains( i ) ) S << xRED << source[i];
        else                    S << source[i];
      }
      std::string incorrect{ S.str() };
      log.vital      ( kit( "%sUser: %s%s", YELLOW, incorrect.c_str(), RESET ) );
      if( peer ) send( ERROR, kit( "%s%s", incorrect.c_str(), RESET ), true );
                                                                                                                              /*
      Send error explanation header:
                                                                                                                              */
      if( parser.errors->count == 1 ){
        log.vital      (        kit( "%s AGI: %sError:%s", GREEN, RED, RESET ) );
        if( peer ) send( ERROR, kit( "%sError:%s",                RED, RESET ) );
      } else {
        log.vital      (        kit( "%s AGI: %s%u errors:%s", GREEN, RED, parser.errors->count, RESET ) );
        if( peer ) send( ERROR, kit( "%s AGI: %s%u errors:%s", GREEN, RED, parser.errors->count, RESET ) );
      }
                                                                                                                              /*
      Send error(s) explanation(s):
                                                                                                                              */
      const std::string GEL{ "Gel" };
      const std::string GEL_STATEMENT { GREEN + GEL + YELLOW + " statement" };
      for( const auto msg: MSG ){
        std::string note{ msg };
        auto i = note.find( GEL );
        if( i != std::string::npos ) note.replace( i, GEL.length(), GEL_STATEMENT );
        const char* NOTE{ note.c_str() + 2 };
        log.vital      (        kit( "     %s%s%s", YELLOW, NOTE, RESET ) );
        if( peer ) send( ERROR, kit( "     %s%s%s", YELLOW, NOTE, RESET ) );
      }
      log.flush();
    }//GelError

    bool operator()( std::string_view source ){
                                                                                                                              /*
      Process statement presented by string:
                                                                                                                              */
      reset();
      parse( (unsigned char*)source.data(), unsigned( source.size() ) );
      if( parser.errors->count ){
        GelError( source );
        return false;
      }
      assignKnownId();
                                                                                                                              /*
      Expose colored sources:
                                                                                                                              */
      const auto colored = decorated();
      log.vital( kit( "%sSource: %s", CYAN, colored.c_str() ) );
      created.clear();
      createEntities();
      log.vital( "RPN:" );
      for( const auto& term: parser.RPN ) log.vital( std::string( term ).c_str() );
                                                                                                                              /*
      Interpret RPN:
                                                                                                                              */
      const bool ok{ interpret() };
      for( const auto& line: result ) log.vital( line );
      log.flush();
      return ok;
    }//bool operator( string )

    bool run(){
                                                                                                                              /*
      Try to use remote input as source.
      Returns user input if first symbol is `#`
                                                                                                                              */
      int Io{ 0 }; // :min     history index
      int It{ 0 }; // :max     history index
      int I_{ 0 }; // :current history index
      std::unordered_map< std::string, int > history;
      constexpr unsigned HISTORY_CAPACITY{ 16 }; // :can be increased

      auto H = [&]( int i )->std::string{
        for( const auto& [ src, ord ]: history ) if( i == ord ) return src;
        return "";
      };

      auto UP = [&]()->std::string{
        if( It > Io ){ if( --I_ < Io ) I_ = It; }
        return H( I_ );
      };

      auto DOWN = [&]()->std::string{
        if( It > Io ){ if( ++I_ > It ) I_ = Io; }
        return H( I_ );
      };

      using Message = std::tuple< std::string, char, std::string >;

      auto receive = [&]()->Message{
        const std::string data = channel.pull(); assert( not data.empty() );
        messageId = data.substr( 0, 4 );
        return std::make_tuple( messageId, data.at( 4 ), data.substr( 5 ) );
      };

      auto VERS = [&]()->std::string{
         log.vital( "Got language version request" );
         std::stringstream S;
         S << YELLOW << "Communication language " << CYAN << "Gel "
           << RED    << Symbol::OMEGA << YELLOW << " [ 21.06 ]" << RESET;
         return S.str();
      };

      auto STAT = [&](){
        send( FACTS, kit( "%sGnosis   %6u%s", YELLOW, gnosis  .size(), RESET ) );
        send( FACTS, kit( "%sGlossary %6u%s", YELLOW, glossary.size(), RESET ) );
        send( FACTS, kit( "%sData     %6u%s", YELLOW, data    .size(), RESET ) );
      };

      for(;;){
        if( not channel.live() ){
          log.vital( kit( "Communication channel error: %s", channel.error().c_str() ) );
          break;
        }
        log.flush();
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        if( channel.empty() ) continue;
        auto[ id, prefix, source ] = receive();
        log.vital( source );
        if( source.front() == '#' ){
          if( source.length() > 1 ){
            switch( toupper( source[1] ) ){
              case 'U' : send( PRIOR, UP  () ); break;
              case 'D' : send( PRIOR, DOWN() ); break;
              case 'S' :              STAT();   break;
              case '@' : send( FACTS, VERS() ); break; // :language info
              default  :                        break; // :just ignore
            }
            continue;
          }
                                                                                                                              /*
          Terminate:
                                                                                                                              */
          std::stringstream S;
          S << YELLOW << "Buy!" << RESET;
          send( QUIT, S.str() );
          while( not channel.done() ) std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                                                                                                                              /*
          Wait for sending all pushed:
                                                                                                                              */
          Timer timer;
          while( not channel.done() ){
            if( timer.elapsed( Timer::SEC ) > 8 ) break;
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
          }
          return true; // :actual termination point
        }//if source `#`
                                                                                                                              /*
        Store input into history:
                                                                                                                              */
        if( history.empty() ){
          history[ source ] = I_ = Io = It = 1;
        } else {
          history[ source ] = I_ = ++It;
          if( history.size() > HISTORY_CAPACITY ){
            std::string oldest;
            for( const auto& [ src, ord ]: history ) if( ord == Io ){ oldest = src; break; }
            assert( not oldest.empty() );
            history.erase( oldest );
            Io++;
          }
        }
                                                                                                                              /*
        Parse sources:
                                                                                                                              */
        reset();
        parse( (unsigned char*)source.data(), unsigned( source.size() ) );
        if( parser.errors->count ){
          std::string_view src( source );
          GelError( src, true ); // :`true` means sending response to remote user
          continue;
        }
                                                                                                                              /*
        Collect unknown entities:
                                                                                                                              */
        assignKnownId();
                                                                                                                              /*
        Send colored sources back:
                                                                                                                              */
        std::string colored = decorated();
        log.vital();
        log.vital( kit( "%sUser%s: %s", GREEN, RESET, colored.c_str() ) );
        log.flush();
        send( CORRECT, colored, true );
        if( not expected.empty() ){
                                                                                                                              /*
          Statement contains unknown entities that expected to be known:
                                                                                                                              */
          std::stringstream error;
          error << RED << "Statement contains unknown entities that expected to be known." << RESET;
          log.vital( error.str() );
          send( ERROR, error.str() );
        } else if( authorized() ){
          log.vital( "Authorized" ); log.flush(); // DEBUG
                                                                                                                              /*
          Create entities if need:
                                                                                                                              */
          created.clear();
          createEntities();
                                                                                                                              /*
          Print RPN:
                                                                                                                              */
          log.vital( "RPN:" );                                                                                          // DEBUG
          log.flush();                                                                                                  // DEBUG
          for( const auto& term: parser.RPN ) log.vital( std::string( term ).c_str() );                                 // DEBUG
          log.flush();                                                                                                  // DEBUG
                                                                                                                              /*
          Interpret RPN:
                                                                                                                              */
          interpret();
          if( result.empty() ) result.push_back( kit( "%s%s%s", YELLOW, Symbol::EMPTY_SET, RESET ) );
          for( const auto& line: result ) log.vital( line ), send( INFO, line );
          assert( send( END, "." ) );
          log.flush();
        } else {
          constexpr const char* CANCELED{ "Not authorized by user" };
          log.vital( CANCELED );
          send( INFO, kit( "%s%s%s", YELLOW, CANCELED, RESET ) );
          log.flush();
        }//if authorized creation
      }//forever
      return true;
    }//bool operator()

  };//class Hmi

}//namespace CoreAGI

#endif // HMI_H_INCLUDED
