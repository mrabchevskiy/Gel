                                                                                                                              /*
  Class that keeps knowledge graph.

  Whole graph splitted into a few segments; requests to for selection
  by syndrome (set of signs) dispatched to permanently active segment
  thread.

  Structure:

    namespace CoreAGI

      template class Shard      - active segment of knowledge graph, use Sequence
        private class Node      - information about single entity of particular unit

      template class Gnosis     - knowledge graph
        public  class Entity    - refers entity in a particular unit
        public  class Syndrome  - represents set of entities (belonged to particular unit)
          public class Iter     - range iterator of goup`s members (entities)

________________________________________________________________________________________________________________________________

  2019.02.17 Initial version

  2020.04.15 Bug fixed
             Removed `using namespace std`
             Replaced `printf` by Logger-based logging

  2020.04.30 Code cleaning; constant moved to config.h

  2020.07.11 NUMBER_OF_SEGMENTS become Config`s constant;
             `segment` become fixed-size array

  2020.07.24 Method `SetOfEntities.process(.)` added

  2020.07.27 `glossary` component removed from `Gnosis` because AGI unit may use
             a few glossaries for conversation with a few channels.
             Removed related methods `entity( const std::string& lex )` and `existingEntity( const std::string& lex )`
             that should be part of the AGI `Unit` class

  2020.07.29 Gnosis inner class `Sequence` added (similar to Entity and Syndrome);
             Low level sequence of ID renamed from Sequence to Seq (file sequence.h -> seq.h)

  2020.07.31 A few methods added; some methods modified

  2020.11.10 Added Gnosis::Entity.forget() and Gnosis::Entity.E()

  2020.11.11 Gnosis::Entity.absorb() and Gnosis::Entity.opeator << added

  2020.12.11 Refactoring after `Segment` refactored

  2020.12.21 Added private methods Gnosis.is(..), Gnosis.S_(.), Gnosis.Q_(.)

  2020.12.23 Gnosis.entity( Identity ) renamed to Gnosis.recover( Identity )

  2020.12.25 Added (non-methods) functions:
               S( Entity )-> Syndrome
               E( Entity )-> std::vector< Entity >
               Q( Entity )-> Sequence
             Added congenital concept HERITABLE

  2020.12.26 Entity.incl() updated: added processing of MUTEX and HERITABLE signs
             Added Entity.A() and Gnosis.A( Entity )

  2021.01.01 Fixed bug in Entity.forget()
             Added Gonosis.analogic(...)

  2021.01.02 Improved candidates selection in Gnosis.analogic (do not include entities with empty syndrome)

  2021.04.13 Added Gnosis::Syndrome opersator* (intercestion of synfromes)
             Added Gnosis.setOfEntities( const Syndrome& syndrome, const Syndrome& tabu )
             Aded  Entity Gnosis::uniqueEntity( const Syndrome& syndrome )
             Aded  Entity Gnosis::uniqueEntity( const Syndrome& syndrome, const Syndrome& tabu )

  2021.05.31 Added Gnosis::Entity.type()       method

  2021.06.06 Added Gnosis::Entity.attributes() method

  __________________________________________________________

  TODO:

  [+] 2020.12.26 Process assigning sign that is an mutually exclusive one
                 Process assigning sign that has heritable sign

  [+] 2020.12.26 Add register of function for external processing of the entity forgetting events

  [+] 2021.01.01 Implement Gnosis.analogic()

  [+] 2021.01.02 Implement multithreding version of the into Gnosis.analogic()

  [-]            Add endianness coversion to `codec.h` (or check it)

  [-]            Add header into file with Gnosis` data dump that refers version and andianness

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef GNOSIS_H_INCLUDED
#define GNOSIS_H_INCLUDED

#include <stdio.h>
#include <cmath>           // :log

#include <atomic>
#include <bit>             // :endianness
#include <bitset>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <span>
#include <thread>
#include <unordered_set>
#include <vector>

#include "codec.h"
#include "logger.h"
#include "set.h"
#include "random.h"
#include "timer.h"
#include "segment.h"
#include "arena.h"
#include "heapsort.h"

namespace CoreAGI {

  using namespace Config::gnosis;

  class Gnosis {
  public:

    using Shard = Segment< CAPACITY_OF_SEGMENT >;

    class Local {
                                                                                                                              /*
      Base class for Entity, Syndrome and SetOfEntities.
      Just keep information about what is parent Gnosis instange.

      Two way ono-to-one mapping Gnosis instance pointer <-> ID:
                                                                                                                              */
    public:

      const unsigned unit;

      Local( const unsigned unit ): unit{ unit }{}

      bool mate( const Local& L ) const {
                                                                                                                              /*
        Check belonging to same unit == Gnosis instance:
                                                                                                                              */
        const bool result{ unit == L.unit };
        if( not result ){ printf( "\n\n Local.mate: %i != %i", unit, L.unit ); fflush( stdout ); } // DEBUG
        return result;
      }

      Gnosis& gnosis() const { return Gnosis::instance.at( unit ); }

    };//calss Local


    friend class Local;
    friend class SetOfEntities;
    friend class Segment< Config::gnosis::CAPACITY_OF_SEGMENT >; //Shard;
    friend class Glossary;
    friend class Processor;

    class Syndrome;
    class Sequence;


    class Entity: public Local {

      friend class Gnosis;
      friend class Glossary;
      friend class Processor;

      Identity id;
                                                                                                                              /*
      Private constructors acessible from Gnosis.entity() and Glossary;

      Normal constructor called from Gnosis.entity():
                                                                                                                              */
      Entity( unsigned unit, const Identity id ): Local( unit ), id( id ){
        Shard& segment = gnosis().segment( id );
//        gnosis().log.sure(
//          not segment.contains( id ),
//          "Constructor `Entity( unit, id )` called with `id` of existing entity, maybe instead of call `Gnosis.entity( id )`"
//        );
        segment.incl( id ); // :include entity into segment with empty syndrome
      }
                                                                                                                              /*
      Special constructor used for congenital concepts initialization
      in the Gnosis` constructor before Shards are initialized:
                                                                                                                              */
      Entity( unsigned unit ): Local{ unit }, id{ 0 }{}

    private:
                                                                                                                              /*
      Methods used exclusively in the Gnosis constructor for congenital concepts definition:
                                                                                                                              */
      Entity& operator = ( Identity ID ){
                                                                                                                              /*
        Assign congenital entity ID and make graph node:
                                                                                                                              */
        id = ID;
        Shard& segment = gnosis().segment( id );
        assert( segment.incl( id ) );  // :include entity into segment with empty syndrome
        gnosis().log.vital( kit( "  Congenital concept %8u created", id ) ); gnosis().log.flush();                      // DEBUG
        return *this;
      }

      Entity& operator = ( std::initializer_list< Entity > syndrome ){
                                                                                                                              /*
        Assign syndrome to congenital entity and place into adequate segment;
        must be called from Gnosis constructor only:
                                                                                                                              */
        for( const auto& sign: syndrome ){
          assert( sign.id != CoreAGI::NIHIL );
          Shard& segment = gnosis().segment( id );
          Signs* Y{ segment[id] }; assert( Y );
          assert( Y->incl( sign.id ) );
          // gnosis().log.vital( kit( "  Congenital syndrome: %8u --> %8u", id, sign.id ) ); gnosis().log.flush();      // DEBUG
        }
        return *this;
      }

    public:

      Entity(                 ): Local{ 0      }, id{ 0    }{}  // :produces NIHIL entity
      Entity( const Entity& e ): Local{ e.unit }, id{ e.id }{}  // :copy constructor

      explicit operator bool    () const { return id != CoreAGI::NIHIL; }
      explicit operator Identity() const { return id;          }

      const Gnosis::Syndrome S() const; // :forward declaration
      const Gnosis::Sequence Q() const; // :forward declaration
                                                                                                                              /*
      Explication:
                                                                                                                              */
      const std::vector< Entity > E() const {
                                                                                                                              /*
        Represent `id` of this `Entity` as `Array` of `Identity` with legth==1:
                                                                                                                              */
        Syndrome S = gnosis().syndrome();
        S.incl( *this );

        std::vector< Entity > expl;
                                                                                                                              /*
        Make selection by the single signle-sign-syndrome:
                                                                                                                              */
        gnosis().select(
          std::span< Syndrome >( &S, 1 ),
          [&]( unsigned ix, Entity e )->bool{ assert( ix == 0 ); expl.push_back( e ); return true; }
        );
        return expl;
      }
                                                                                                                              /*
      Array of entity` attributes:
                                                                                                                              */
      const std::vector< Entity > attributes() const {                                                         // [+] 2021.06.06
        std::vector< Entity > result;
        const auto& ATTRIBUTE{ gnosis().ATTRIBUTE };
        S().process( [&]( const Entity& sign )->bool{ if( sign.is( ATTRIBUTE ) ) result.push_back( sign ); return true; } );
        return result;
      }
                                                                                                                              /*
      Array of `affined`, or similar, entities that has same syndrome as this entity;
                                                                                                                              */
      const std::vector< Entity > A( std::span< Entity > mask = std::span< Entity >{} ) const {                // [+] 2020.12.26
                                                                                                                              /*
        Compose search syndrome as syndrome of *this excluding `mask` sings:
                                                                                                                              */
        Syndrome syndrome{ S() };
        for( const auto& sign: mask ) syndrome.excl( sign );                                 // :exclude masked signs
        std::vector< Entity > affined;                                                       // :storage for selected entities
                                                                                                                              /*
        Make selection by the single syndrome:
                                                                                                                              */
        gnosis().select(
          std::span< Syndrome >( &syndrome, 1 ),
          [&]( unsigned ix, Entity e )->bool{ assert( ix == 0 ); affined.push_back( e ); return true; }
        );
        return affined;
      }

      bool forget( bool skipCheck = false ){                                                                   // [+] 2020.11.10
                                                                                                                              /*
        NOTE: Forgetting without check assumes that explication this.E() is empty;
              Forgetting with no check is much more fast but must be used cautiously:
                                                                                                                              */
        Gnosis& G    { gnosis()        };
        Shard&  shard{ G.segment( id ) };
                                                                                                                              /*
        Be shure that this entity is not IMMORTAL:
                                                                                                                              */
        Signs* Y{ shard[ id ] };  assert( Y );
        if( Y->contains( G.IMMORTAL.id ) ) return false;
                                                                                                                              /*
        Execute external event processors:
                                                                                                                              */
        for( auto [ key, f ]: G.onChangeID ) f( id, CoreAGI::NIHIL, is( gnosis().ATTRIBUTE ) );
        if( not skipCheck ){
                                                                                                                              /*
          Remove this entity from all syndromes:
                                                                                                                              */
          unsigned n{ 0 };
          for( auto& S: G.segments ) n += S.forgotten( id );
          if( n ) gnosis().log.vital( kit( "[Entity.forget] Entity %u excluded from %u syndromes", id, n ) ); // DEBUG
        }
        shard.excl( id );                                                                                      // [m] 2021.01.01
        id = CoreAGI::NIHIL;
        return true;
      }

      Entity& incl( const Entity& sign ){                                                       // [m] 2020.07.31 [m] 2020.12.26
        if( sign.id == CoreAGI::NIHIL ) return *this;
        assert( mate( sign ) );
        Gnosis&  G      { gnosis()      };
        Shard& segment{ G.segment( id ) };
        Signs* Y{ segment[id] }; assert( Y );
        sign.S().process(
                                                                                                                              /*
          Add heritable signs of the adding sign and remove mutually exclusive signs:
                                                                                                                              */
          [&]( const Entity& signSign )->bool{
            if( signSign.is( G.HERITABLE ) ) Y->incl( signSign.id );
            if( signSign.is( G.MUTEX     ) ) for( const auto& Ai: signSign.E() ) Y->excl( Ai.id );
            return true;
          }
        );
                                                                                                                              /*
        Finally add `sign` itself:
                                                                                                                              */
        Y->incl( sign.id );
        return *this;                                                                                          // [+] 2020.07.31
      }

      Entity& incl( std::initializer_list< Entity > syndrome ){                                                // [+] 2020.07.31
        Gnosis& G      { gnosis()        };
        Shard&  segment{ G.segment( id ) };
        Signs*  Y      { segment[id]     }; assert( Y );
        if( not Y->contains( G.IMMUTABLE.id ) ) for( const auto& sign: syndrome ){
          if( sign.id == CoreAGI::NIHIL ) continue;
          assert( mate( sign ) );
          Y->incl( sign.id );
        }
        return *this;
      }

      Entity& excl( const Entity& sign ){                                                                      // [m] 2020.07.31
        if( sign.id == CoreAGI::NIHIL ) return *this;
        assert( mate( sign ) );
        Gnosis& G      { gnosis()        };
        Shard&  segment{ G.segment( id ) };
        Signs*  Y      { segment[id]     }; assert( Y );
        Y->excl( sign.id );
        return *this;                                                                                          // [+] 2020.07.31
      }

      Entity& excl( std::initializer_list< Entity > syndrome ){                                                // [+] 2020.07.31
        Gnosis& G      { gnosis()        };
        Shard&  segment{ G.segment( id ) };
        Signs*  Y      { segment[id]     }; assert( Y );
        if( not Y->contains( G.IMMUTABLE.id ) ) for( const auto& sign: syndrome ){
          if( sign.id == CoreAGI::NIHIL ) continue;
          assert( mate( sign ) );
          Y->excl( sign.id );
        }
        return *this;
      }

      bool absorb( Entity& e ){                                                                                // [+] 2020.11.11
                                                                                                                              /*
        This entity absorb `e` entity;
        Absorbtion is impossible if:
          - `e` is immortal (so can't be deleted)
          - `this` entity is immutable (so can't absorb `e` syndrome)
          - at least one of `e` childs (elements of e.E()) is immutable
                                                                                                                              */
        Gnosis& G{ gnosis() };
        if( e.is( G.IMMORTAL  ) ) return false;
        if(   is( G.IMMUTABLE ) ) return false;
                                                                                                                              /*
        Collect child entities:
                                                                                                                              */
        std::vector< Entity > E{ e.E() };
        for( auto& elem: E     ) if( elem.is( G.IMMUTABLE ) ) return false;
                                                                                                                              /*
        Move signs:
                                                                                                                              */
        e.S().process(
          [&]( const Entity& sign )->bool{
              incl( sign );
            e.excl( sign );                                                                                    // [+] 2021.06.13
            return true;
          }
        );  // :absorb syndrome
                                                                                                                              /*
        Move childs:
                                                                                                                              */
//      for( auto& elem: E ) elem.incl( *this ); // :absorb childs of `e`
        for( auto& elem: E ) elem.incl( *this ).excl( e );                                                     // [m] 2021.06.13
                                                                                                                              /*
        Process ID changing in dependent data structures:
                                                                                                                              */
        for( auto& [ k, f ]: G.onChangeID ) f( e.id, id, false );                                                        // [+] 2021.06.13
                                                                                                                              /*
        Finally forget...
                                                                                                                              */
        assert( e.forget( false ) );                 // :delete absorbed entity `e`
        return true;
      }

      bool is( const Entity& sign ) const {
        if( sign.id == CoreAGI::NIHIL ) return false;
        if( not mate( sign )          ) return false;
        Gnosis& G      { gnosis()        };
        Shard&  segment{ G.segment( id ) };
        Signs*  Y      { segment[id]     }; assert( Y );
        return Y->contains( sign.id );
      }

      Entity type() const {                                                                                    // [+] 2021.05.31
        if( not is( gnosis().ATTRIBUTE ) ) return gnosis().none();
        if(     is( gnosis().INTEGER   ) ) return gnosis().INTEGER;
        if(     is( gnosis().RATIONAL  ) ) return gnosis().RATIONAL;
        if(     is( gnosis().STRING    ) ) return gnosis().STRING;
        return gnosis().none();
      };

      Entity& operator+= ( const                  Entity&  sign     )       { return incl( sign     );              }
      Entity& operator-= ( const                  Entity&  sign     )       { return excl( sign     );              }
      Entity& operator+= ( std::initializer_list< Entity > syndrome )       { return incl( syndrome );              }
      Entity& operator-= ( std::initializer_list< Entity > syndrome )       { return excl( syndrome );              }
      bool    operator<< ( Entity& e                                )       { return absorb( e );                   }
      bool    operator== ( const                  Entity&  e        ) const { return id == e.id and unit == e.unit; }
      bool    operator!= ( const                  Entity&  e        ) const { return id != e.id or  unit != e.unit; }
      bool    operator > ( const                  Entity&  sign     ) const { return is( sign );                    }

      Entity operator-- ( int ) const { return *this; }

      bool seq( Sequence& sequence ){
        assert( mate( sequence ) );
        Gnosis& G{ gnosis() };
        if( is( G.IMMUTABLE ) ) return false;
        Shard& segment{ G.segment( id ) };
        Seq S;
        const unsigned n = sequence.size();
        for( unsigned i = 0; i < n; i++ ) S += sequence[i].id;
        segment.sequences[id] = S;
        return true;
      }

      bool seq( std::initializer_list< Entity > sequenceOfEntities ){
        Gnosis& G{ gnosis() };
        if( is( G.IMMUTABLE ) ) return false;
        Shard& segment{ G.segment( id ) };
        bool ok{ true };
        Seq S;
        for( const auto& entity: sequenceOfEntities ){ if( mate( entity ) ) S += entity.id; else ok = false; }
        segment.sequences[id] = S;
        return ok;
      }

      uint64_t operator^ ( const Entity& e ) const {
        assert( mate( e ) );
        return combination( id, e.id );
      }

    };//class Gnosis::Entity

    const              Gnosis::Syndrome S( const Gnosis::Entity& e );                                          // [+] 2020.12.25
    const              Gnosis::Sequence Q( const Gnosis::Entity& e );                                          // [+] 2020.12.25
    const std::vector< Gnosis::Entity > E( const Gnosis::Entity& e );                                          // [+] 2020.12.25
    const std::vector< Gnosis::Entity > A( const Gnosis::Entity& e );                                          // [+] 2020.12.26

  private:

    static std::vector< std::reference_wrapper< Gnosis > > instance; // :list of Gnosis instances

    const char*           TITLE;
    const unsigned        ID;                             // :Gnosis instance ID
    Logger::Log           log;                            // :Gnosis instance` log
    Shard                 segments[ NUMBER_OF_SEGMENTS ]; // :segments                                         // [+] 2020.07.11
    mutable Arena         arena;                          // :arena allocator
    std::vector< Entity > CONGENITAL;
                                                                                                                              /*
    Processors of the ID changing event:
      ID can be removed at `forget` or replaced by another one at `merge`
      On merge  first  ID repaced by second one
      On forget second ID is NIHIL
      When `attr` is `true` it meand that ID refers attribite entity
                                                                                                                              */
    std::map< Identity, std::function< void( const Identity&, const Identity&, bool attr ) > > onChangeID;     // [m] 2021.06.13

    static std::mutex globalMutex;                        // :static

  public:
                                                                                                                              /*
    Congenital concepts:
                                                                                                                              */
    Entity ABSORB;
    Entity ADJECTIVE;              // [+] 2021.04.28
    Entity AND;
    Entity ATTRIBUTE;
    Entity CLEAR;
    Entity DECR;
    Entity DIFF;
    Entity DIV;
    Entity EXCL;
    Entity EXPL;
    Entity FORGET;
    Entity FORK;
    Entity FUNCTION;
    Entity HERITABLE;
    Entity IF;
    Entity IMMORTAL;
    Entity IMMUTABLE;
    Entity INCL;
    Entity INCR;
    Entity INTEGER;
    Entity LET;
    Entity MULT;
    Entity MUTEX;
    Entity NAME;                // [+] 2021.06.03
    Entity NOUN;                // [+] 2021.04.28
    Entity OPERATOR;
    Entity OR;
    Entity POP;
    Entity PROD;
    Entity PROPER;              // [+] 2021.04.28
    Entity QUOT;
    Entity RATIONAL;
    Entity REF;
    Entity ROUTINE;
    Entity RULE;
    Entity RUN;
    Entity SEQ;
    Entity SEQUENCE;
    Entity STRING;
    Entity SUM;
    Entity SWAP;
    Entity SYN;
    Entity VAL;
    Entity VERB;                // [+] 2021.04.28

  private:
                                                                                                                              /*
    Get segment that contains particular entity:
                                                                                                                              */
    const Shard& segment( Identity i ) const { return segments[ i % NUMBER_OF_SEGMENTS ]; }                    // [+] 2020.07.27
          Shard& segment( Identity i )       { return segments[ i % NUMBER_OF_SEGMENTS ]; }                    // [m] 2020.07.11

    bool is( const Identity& e, const Identity& s ) const {                                                 // [+] 2020.12.21
      printf( "\n  [Gnosis.is] %u %u\n", e, s ); fflush( stdout ); // DEBUG
      if( e == CoreAGI::NIHIL ) return false;
      if( s == CoreAGI::NIHIL ) return false;
      printf( "\n  [Gnosis.is] checkpoint A\n" ); fflush( stdout ); // DEBUG
      assert( exist( e ) ); // DEBUG
      assert( exist( s ) ); // DEBUG
      printf( "\n  [Gnosis.is] checkpoint B\n" ); fflush( stdout ); // DEBUG
      const Shard* shard{ &segment( e ) };
      assert( shard                );
      //assert( shard->contains( e ) );
      printf( "\n  [Gnosis.is] checkpoint C\n" ); fflush( stdout ); // DEBUG
      const Signs* S = shard->get( e );
      printf( "\n  [Gnosis.is] checkpoint D\n" ); fflush( stdout ); // DEBUG
      assert( S );
      printf( "\n  [Gnosis.is] checkpoint E\n" ); fflush( stdout ); // DEBUG
      bool result = S->contains( s );
      printf( "\n  [Gnosis.is] checkpoint F\n" ); fflush( stdout ); // DEBUG
      return result;
    }

    const Seq*   Q_( const Identity& id ) const { return segment( id ).Q( id ); }                              // [+] 2020.12.21
    const Signs* S_( const Identity& id ) const { return segment( id )[ id ];   }                              // [+] 2020.12.21
                                                                                                                              /*
    Tell all segments to finish:
                                                                                                                              */
    void finish(){ for( auto& S: segments ) S.terminate(); }

  public: // Gnosis

    Gnosis( const char* title, Logger& logger ):

      TITLE      { title                          },  // :assign Gnosis` instange name
      ID         { unsigned( instance.size() )    },  // :assign Gnosis` instance ID
      log        { logger.log( title )            },  // :set logging channel
      arena      { Config::gnosis::ARENA_CAPACITY },
      CONGENITAL {                                },
      onChangeID {                                },
      ABSORB     { ID                             },
      ADJECTIVE  { ID                             },
      AND        { ID                             },
      ATTRIBUTE  { ID                             },
      CLEAR      { ID                             },
      DECR       { ID                             },
      DIFF       { ID                             },
      DIV        { ID                             },
      EXCL       { ID                             },
      EXPL       { ID                             },
      FORGET     { ID                             },
      FORK       { ID                             },
      FUNCTION   { ID                             },
      HERITABLE  { ID                             },
      IF         { ID                             },
      IMMORTAL   { ID                             },
      IMMUTABLE  { ID                             },
      INCL       { ID                             },
      INCR       { ID                             },
      INTEGER    { ID                             },
      LET        { ID                             },
      MULT       { ID                             },
      MUTEX      { ID                             },
      NAME       { ID                             },    // [+] 2021.06.03
      NOUN       { ID                             },
      OPERATOR   { ID                             },
      OR         { ID                             },
      POP        { ID                             },
      PROD       { ID                             },
      PROPER     { ID                             },
      QUOT       { ID                             },
      RATIONAL   { ID                             },
      REF        { ID                             },
      ROUTINE    { ID                             },
      RULE       { ID                             },
      RUN        { ID                             },
      SEQ        { ID                             },
      SEQUENCE   { ID                             },
      STRING     { ID                             },
      SUM        { ID                             },
      SWAP       { ID                             },
      SYN        { ID                             },
      VAL        { ID                             },
      VERB       { ID                             }
    {                                                                                                                         /*
      Define syndromes of congenital concepts:
                                                                                                                              */
      std::lock_guard< std::mutex >lock( globalMutex );
                                                                                                                              /*
      Add this Gnosis instance to the list of Gnosis instances:
                                                                                                                              */
      instance.push_back( std::ref( *this ) );
                                                                                                                              /*
      Print capacity:
                                                                                                                              */
      log.vital( kit( "`%s` ID: %u",                       TITLE, ID                              ) );
      log.vital( kit( "  Gnosis   capacity: %8i entities", CAPACITY_OF_SEGMENT*NUMBER_OF_SEGMENTS ) );
      log.vital( kit( "  Syndrome capacity: %8i signs",    CAPACITY_OF_SYNDROME                   ) );
      log.vital( kit( "  Syndrome capacity: %8i signs",    CAPACITY_OF_SYNDROME                   ) );
      if      constexpr( std::endian::native == std::endian::big    ) log.vital( "  big-endian"    );
      else if constexpr( std::endian::native == std::endian::little ) log.vital( "  little-endian" );
      else                                                            log.vital( "  mixed-endian"  );
                                                                                                                              /*
      Activate segments:
                                                                                                                              */
      for( unsigned s = 0; s < NUMBER_OF_SEGMENTS; s++ ){
        char segmentName[ Config::logger::CHANNEL_NAME_CAPACITY ];
        snprintf( segmentName, Config::logger::CHANNEL_NAME_CAPACITY - 1, "%s:%03u", title, s + 1 ); // :make segment name
        log.sure( segments[s].start( segmentName, s ), kit( "Segment %s not started", segmentName ) );
      }
                                                                                                                              /*
      Assign ID to congenital concepts (generated by call randomNumber() from "random.h"):
                                                                                                                              */
      log.vital();
      log.vital( "Segments activated, make congenital concepts" ); log.vital(); log.flush();

      CONGENITAL.push_back( ABSORB    =  6739698 );
      CONGENITAL.push_back( ADJECTIVE =  4087907 ); // [+] 2021.04.29
      CONGENITAL.push_back( AND       =   374564 );
      CONGENITAL.push_back( ATTRIBUTE = 15039847 );
      CONGENITAL.push_back( CLEAR     =  2832983 );
      CONGENITAL.push_back( DECR      =  4930630 );
      CONGENITAL.push_back( DIFF      =  8699352 );
      CONGENITAL.push_back( DIV       = 11704920 );
      CONGENITAL.push_back( EXCL      =  2701626 );
      CONGENITAL.push_back( EXPL      = 13421964 );
      CONGENITAL.push_back( FORGET    =  4735681 );
      CONGENITAL.push_back( FORK      = 11435494 );
      CONGENITAL.push_back( FUNCTION  = 15354407 );
      CONGENITAL.push_back( HERITABLE = 12454336 );
      CONGENITAL.push_back( IF        =  6662231 );
      CONGENITAL.push_back( IMMORTAL  = 12888623 );
      CONGENITAL.push_back( IMMUTABLE =  8325804 );
      CONGENITAL.push_back( INCL      = 14665902 );
      CONGENITAL.push_back( INCR      =  7036504 );
      CONGENITAL.push_back( INTEGER   = 10608339 );
      CONGENITAL.push_back( LET       =  9276241 );
      CONGENITAL.push_back( MULT      = 15984293 );
      CONGENITAL.push_back( MUTEX     =  1484405 );
      CONGENITAL.push_back( NAME      =  2327283 ); // [+] 2021.06.03
      CONGENITAL.push_back( NOUN      =  9807832 ); // [+] 2021.04.29
      CONGENITAL.push_back( OPERATOR  = 10638075 );
      CONGENITAL.push_back( OR        =   606745 );
      CONGENITAL.push_back( POP       =  3107661 );
      CONGENITAL.push_back( PROD      =  6264904 );
      CONGENITAL.push_back( PROPER    =   556209 ); // [+] 2021.04.29
      CONGENITAL.push_back( QUOT      = 15636372 );
      CONGENITAL.push_back( RATIONAL  =  7644169 );
      CONGENITAL.push_back( REF       =  2951283 );
      CONGENITAL.push_back( ROUTINE   =  8222403 );
      CONGENITAL.push_back( RULE      =  5157699 );
      CONGENITAL.push_back( RUN       =  4527056 );
      CONGENITAL.push_back( SEQ       =   532165 );
      CONGENITAL.push_back( SEQUENCE  =  2215104 );
      CONGENITAL.push_back( STRING    =  5853461 );
      CONGENITAL.push_back( SUM       =  3491838 );
      CONGENITAL.push_back( SWAP      = 15599439 );
      CONGENITAL.push_back( SYN       =  2527987 );
      CONGENITAL.push_back( VAL       = 12131759 );
      CONGENITAL.push_back( VERB      =  8829778 ); // [+] 2021.04.29
                                                                                                                              /*
      Assigned random ID: xHJi  8829778  2021.04.29 VERB
                          27ON   556209  2021.04.29 PROPER
                          fC1z  4087907  2021.04.29 ADJECTIVE
                          8UbP  2327283  2012.06.03 NAME

      More: see 'CoreAGI/.backyard/random-id.txt

      Define signs of NAME before it become immutable:
                                                                                                                              */
      NAME += STRING;
      NAME += ATTRIBUTE;
                                                                                                                              /*
      Assign syndromes to congenital entities and store into adequate segment:
                                                                                                                              */
      log.vital();
      log.vital( "Assign syndromes for congenital concepts.." ); log.flush();
      for( auto& e: CONGENITAL ) e = { IMMUTABLE, IMMORTAL };
                                                                                                                              /*
      Check uniqueness of congenital ID:
                                                                                                                              */
      {
        std::set< Identity > CG;
        for( const auto& e: CONGENITAL ) CG.insert( Identity( e ) );
        log.sure(
          CG.size() == CONGENITAL.size(),
          kit( "Non-unique ID: %u concepts but %lu different ID", CONGENITAL.size(), CG.size() )
        );
      }
                                                                                                                              /*
      Log info about congenital entities:
                                                                                                                              */
      auto expo = [&]( const char* name, const Entity& e ){
        log.vital( kit( "  %-12s %8u %s", name, e.id, exist( e.id ) ? "presented" : "missed" ) );
      };

      log.vital();
      expo( "ABSORB",    ABSORB    );
      expo( "AND",       AND       );
      expo( "ADJECTIVE", ADJECTIVE );  // [+] 2021.04.29
      expo( "ATTRIBUTE", ATTRIBUTE );
      expo( "CLEAR",     CLEAR     );
      expo( "DECR",      DECR      );
      expo( "DIFF",      DIFF      );
      expo( "DIV",       DIV       );
      expo( "EXCL",      EXCL      );
      expo( "EXPL",      EXPL      );
      expo( "FORGET",    FORGET    );
      expo( "FORK",      FORK      );
      expo( "FUNCTION",  FUNCTION  );
      expo( "HRITABLE",  HERITABLE );
      expo( "IF",        IF        );
      expo( "IMMORTAL",  IMMORTAL  );
      expo( "IMMUTABLE", IMMUTABLE );
      expo( "INCL",      INCL      );
      expo( "INCR",      INCR      );
      expo( "INTEGER",   INTEGER   );
      expo( "LET",       LET       );
      expo( "MULT",      MULT      );
      expo( "MUTEX",     MUTEX     );
      expo( "NAME",      NAME      );  // [+] 2021.06.03
      expo( "NOUN",      NOUN      );  // [+] 2021.04.29
      expo( "OPERATOR",  OPERATOR  );
      expo( "OR",        OR        );
      expo( "POP",       POP       );
      expo( "PROD",      PROD      );
      expo( "PROPER",    PROPER    );  // [+] 2021.04.29
      expo( "QUOT",      QUOT      );
      expo( "RATIONAL",  RATIONAL  );
      expo( "REF",       REF       );
      expo( "ROUTINE",   ROUTINE   );
      expo( "RULE",      RULE      );
      expo( "RUN",       RUN       );
      expo( "SEQ",       SEQ       );
      expo( "SEQUENCE",  SEQUENCE  );
      expo( "STRING",    STRING    );
      expo( "SUM",       SUM       );
      expo( "SWAP",      SWAP      );
      expo( "SYN",       SYN       );
      expo( "VAL",       VAL       );
      expo( "VERB",      VERB      );  // [+] 2021.04.29
      log.vital();
      log.sure( size() == CONGENITAL.size(), kit( "Invalid initial size: expected %lu, actual %lu",CONGENITAL.size(),size() ) );
      log.vital();
      log.vital( "Started" );
      log.vital();
      log.flush();
    }

    Gnosis             (               ) = delete;   // :no default constructor
    Gnosis             ( const Gnosis& ) = delete;   // :can`t be copied
    Gnosis& operator = ( const Gnosis& ) = delete;   // :can`t be assigned

   ~Gnosis(){
      log.vital();
      log( kit( "%s going to finish", TITLE ) );
      log( kit( "%s idle state reached; stop segments...", TITLE ) );
      for( auto& S: segments ) log.sure( S.terminate(), kit( "Segment %s cn`t terminate", S.name() ) );
      log( kit( "%s finished", TITLE ) );
      log.vital();
    }
                                                                                                                              /*
    Test identity existence:
                                                                                                                              */
    bool exist( Identity e ) const {
      const Shard* shard{ &segment( e ) };
      return shard->contains( e );
    }
                                                                                                                              /*
    Include / Exclude function for processing entity forgetting (data consistency keeping):
                                                                                                                              */
    Identity onChangeIdIncl( std::function< void( const Identity&, const Identity&, bool ) > f ){
      for(;;){
        const Identity key = randomNumber();
        if( onChangeID.contains( key ) ) continue;
        onChangeID.insert_or_assign( key, f );
        return key;
      }
    }

    bool onChangeIdExcl( const Identity& key ){
      if( not onChangeID.contains( key ) ) return false;
      onChangeID.erase( key );
      return true;
    }
                                                                                                                              /*
    List of all congenital concepts:
                                                                                                                              */
    const std::vector< Entity >& congenital() const { return CONGENITAL; }

    size_t size() const {
      size_t total{ 0 };
      for( const auto& segment: segments ) total += segment.size();
      return total;
    }

    void process( std::vector< Identity >& data, std::function< bool( std::vector< Identity >& ) > f ) const {                             // [+] 2021.04.29
      for( const auto& segment: segments ) segment.process( data, f );
    }

    bool save( const char* folder ) const {
      log( kit( "Save %s into the `%s` folder", TITLE, folder ) );
      namespace fs = std::filesystem;
      fs::path dir{ folder };
      if( not fs::exists( folder ) ){ // Create folder:
        if( not fs::create_directories( folder ) ){
          log( kit( "Can`t create folder `%s`", std::string( folder ).c_str() ) );
          return false;
        }
      }
      fs::path syndromes    { SYNDROMES     };
      fs::path sequences    { SEQUENCES     };
      fs::path pathSyndromes{ dir/syndromes };
      fs::path pathSequences{ dir/sequences };
      FILE* fileSyndromes = fopen( pathSyndromes.string().c_str(), "w" );
      FILE* fileSequences = fopen( pathSequences.string().c_str(), "w" );
      if( not fileSyndromes or not fileSequences ) return false;
      unsigned Ns{ 0 };
      unsigned Nq{ 0 };
      for( auto& segment: segments ){
        Ns += segment.saveSyndromes( fileSyndromes );
        Nq += segment.saveSequences( fileSequences );
      }
      fclose( fileSyndromes );
      fclose( fileSequences );
      log( kit( "  Stored %u syndromes and %u sequences", Ns, Nq ) );
      return true;
    }

    bool load( const char* folder ){
                                                                                                                              /*
      File format:

      [1] Each line defines single entity
      [2] Encoded Entity ID followed by encoded signs ID or sequence elements ID
      [3] Ecoded ID separated by one or more space; space after last ID not required
      [4] Avoiding extra spaces improves performance
                                                                                                                              */
      log( kit( "Load %s from the `%s` folder", title(), folder ) );
      namespace fs = std::filesystem;
      fs::path dir{ folder };
      if( not fs::exists( folder ) ){
        log( kit( "Folder `%s` not found", std::string( folder ).c_str() ) );
        return false;
      }
                                                                                                                              /*
      Check file presence:
                                                                                                                              */
      fs::path pathSyndromes{ dir/fs::path( SYNDROMES ) };
      fs::path pathSequences{ dir/fs::path( SEQUENCES ) };
      if( not fs::exists( pathSyndromes ) ){
        log( kit( "Graph file `%s` not found", std::string( pathSyndromes ).c_str() ) );
        return false;
      }
      if( not fs::exists( pathSequences ) ){
        log( kit( "Sequence file `%s` not found", std::string( pathSequences ).c_str() ) );
        return false;
      }
                                                                                                                              /*
      Open files:
                                                                                                                              */
      FILE* fileSyndromes = fopen( pathSyndromes.string().c_str(), "r" );
      if( not fileSyndromes ) return false;
      FILE* fileSequences = fopen( pathSequences.string().c_str(), "r" );
      if( not fileSequences ) return false;
                                                                                                                              /*
      Load syndromes:
                                                                                                                              */
      log( kit( "  Load syndromes from the `%s` file..", std::string( pathSyndromes ).c_str() ) ); log.flush();
      for( unsigned i = 0; i < NUMBER_OF_SEGMENTS; i++ ) segments[i].clear();
      log.vital( "  Segments cleared.." ); log.flush();
      Signs syndrome{};
      auto  addSign = [&]( Identity ID ){ syndrome.incl( ID ); };
      Timer timer;
      unsigned num{ 0 };
      for(;;){
        syndrome.clear();
        const Identity EntityId = readAndDecode( addSign, fileSyndromes );
        if( not EntityId ) break;
        Shard& shard{ segment( EntityId ) };
        shard.incl( EntityId, syndrome );
        num++;
      }
      fclose( fileSyndromes );
      log( kit( "  Graph with %u nodes loaded in %.3f msec", num, timer.elapsed( Timer::MILLISEC ) ) );
                                                                                                                              /*
      Load sequences:
                                                                                                                              */
      log( kit( "  Load sequences from `%s`..", std::string( pathSyndromes ).c_str() ) );
      Seq  seq{};
      auto addElem = [&]( Identity ID ){ seq.append( ID ); };
      timer.start();
      for(;;){
        seq.clear();
        const Identity entityId = readAndDecode( addElem, fileSequences );
        if( not entityId ) break;
        log.sure( exist( entityId ), kit( "Sequence of the non-existing entity `%u`", entityId ) );
        Shard& shard{ segment( entityId ) };
        shard.seq( entityId, seq );
      }
      fclose( fileSequences );
      log( kit( "  Sequences loaded in %.3f msec", timer.elapsed( Timer::MILLISEC ) ) );
      return true;
    }

    void ping(){
      for( auto& segmentRef: segments ){
        Shard& segment = segmentRef;
        log.vital( kit( "%s %s", segment.name(), segment.active() ? "live" : "dead" ) );
      }
    }

    bool operator == ( const Gnosis& X ) const { return ID == X.ID; }

    operator std::string() const { return std::string( TITLE ); }

    const char* title() const { return TITLE; }

                                                                                                                              /*
    ____________________________________________________________________________________________________________________________

    METHODS FOR CONTRUCTION OF ENTITIES, SYNDROMES, SEQUENCES AND SUBSETS
                                                                                                                              */
    Entity entity(){
                                                                                                                              /*
      Create new entity with random ID:
                                                                                                                              */
      constexpr unsigned ATTEMPT_LIMIT{ 32 };
      unsigned maxDistance;
      for( maxDistance = 0; maxDistance <= 6; maxDistance++ ){
        for( unsigned attempt = 0; attempt < ATTEMPT_LIMIT; attempt++ ){
          constexpr size_t UINT24MASK{ 0xFFFFFF };
          const Identity id{ Identity( randomNumber() & UINT24MASK ) };
          if( id == CoreAGI::NIHIL ) continue;
          assert( id < Flat::UINT24 );
          Shard* shard{ &segment( id ) };
          if( shard->vacant( id, maxDistance ) ) return Entity( ID, id );
          attempt++;
        }
      }
                                                                                                                              /*
      Failure:
                                                                                                                              */
      log.vital();
      log.vital( kit( "Can`t create random ID in %u attempts, max distane %u", ATTEMPT_LIMIT, maxDistance ) );
      log.vital( "Segment occupancy:" );
      unsigned totalEntities{ 0 };
      for( unsigned i = 0; i < NUMBER_OF_SEGMENTS; i++ ){
        Shard* shard( &segments[i] );
        const unsigned num   { shard->size() };
        const double   fract { double( num )/CAPACITY_OF_SEGMENT };
        log.vital( kit( " %s %8u %6.2f %%", shard->name(), num, 100.0*fract ) );
        totalEntities += num;
      }
      const unsigned nominalCapacity{ CAPACITY_OF_SEGMENT*NUMBER_OF_SEGMENTS };
      const double   fract{ double( totalEntities )/nominalCapacity };
      log.vital( kit( "Nominal capacity %u, occupied %u ~ %.2f %%", nominalCapacity, totalEntities, 100.0*fract ) );
      log.abend( "Actual capacity exceeded" );
      return Entity( ID );  // :dummy return, failed to create new entity
    }//entity()

    Entity none() const { return Entity( ID ); } // :unexisting entity

    Entity recover( Identity id ) const {                                                                      // [+] 2020.12.23
                                                                                                                              /*
      Reconstruction existing Entity by ID:
                                                                                                                              */
      log.sure( segment( id ).contains( id ), kit( "Alien entity with id `%u`", id ) );
      Entity e{ ID }; // :private constructor
      e.id = id;
      return e;
    }//entity( Identity )

    Entity entity( std::initializer_list< Entity > syndrome ){
                                                                                                                              /*
      Create new entity with random ID and provided syndrome:
                                                                                                                              */
      Entity e{ entity() };
      for( const auto& sign: syndrome ) e.incl( sign );
      return e;
    }//Entity( syndrome )

  private:

    std::vector< Entity > S( Identity id ) const {
      log.sure( id != CoreAGI::NIHIL, "NIHIL id" );                                                            // [+] 2021.04.27
                                                                                                                              /*
      Get syndrome by entity ID
                                                                                                                              */
      std::vector< Entity > syndrome;
      const Shard& shard{ segment( id ) };
      const Signs* Y    { shard[ id ]   };
      if( Y ) for( const auto& signId: *Y ) syndrome.push_back( recover( signId ) );                           // [+] 2020.12.23
      return syndrome;
    }

    std::vector< Entity > Q( Identity id ) const {
                                                                                                                              /*
      Get sequence by entity ID (if any):
                                                                                                                              */
      std::vector< Entity > sequence;
      const Shard& shard{ segment( id ) };
      const Seq*   seq  { shard.Q( id ) };
//    if( seq ) for( const auto& elemId: *seq ) sequence.push_back( entity( elemId ) );
      if( seq ) for( const auto& elemId: *seq ) sequence.push_back( recover( elemId ) );
      return sequence;
    }

  public:

    bool exists( Identity id ) const {
      const Shard* shard{ &segment( id ) };
      return shard->contains( id );
    }
                                                                                                                              /*
    Syndrome is an inner class of the Gnosis that represents set of signs
    composed of entities contained in the prticular Gnosis instance:
                                                                                                                              */
    class Syndrome: public Local {

      friend class Gnosis;
      friend class Entity;

      Signs syndrome; // :set of Identity

      Syndrome( unsigned unit                 ): Local( unit ), syndrome{   }{}
      Syndrome( unsigned unit, const Signs& S ): Local( unit ), syndrome{ S }{}

    public:

      Syndrome( const Syndrome& S ): Local( S.unit ), syndrome{ S.syndrome }{}

      Syndrome& operator = ( const Syndrome& S ){ syndrome = S.syndrome; return *this; }

      size_t size() const { return syndrome.size(); }

      void clear(){ syndrome.clear(); }

      bool empty() const { return syndrome.empty(); }

      bool contains( const Entity& e ) const {
        if( e.id == 0 ) return false;
        assert( mate( e ) );
        return syndrome.contains( Identity( e ) );
      }

      bool incl( const Entity& e ){
        if( e.id == CoreAGI::NIHIL ) return false;
        assert( mate( e ) );
        const bool included = syndrome.incl( Identity( e ) );
        return included;
      }

      bool incl( const Syndrome& S ){
        assert( mate( S ) );
        bool included = true;
        for( const auto& s: S.syndrome ) included = included | syndrome.incl( s );
        return included;
      }

      void excl( const Entity& e ){
        if( e.id == CoreAGI::NIHIL ) return;
        assert( mate( e ) );
        syndrome.excl( Identity( e ) );
      }

      void excl( const Syndrome& S ){
        assert( mate( S ) );
        for( const auto& s: S.syndrome ) syndrome.excl( s );
      }

      Signs operator* ( const Syndrome& S ) const {                                                            // [+] 2021.04.13
        assert( mate( S ) );
        Signs intersection;
        for( const auto& s: S.syndrome ) if( S.contains( s ) ) intersection.incl( s );
        return intersection;
      }

      Syndrome& operator()( const Entity& e ){ assert( mate( e ) ); incl( e ); return *this; }  // :add e to syndrome

      bool operator == ( const Syndrome& S ) const { return       mate( S )  and   syndrome == S.syndrome;   }
      bool operator != ( const Syndrome& S ) const { return ( not mate( S ) ) or ( syndrome != S.syndrome ); }

      void process( std::function< bool( const Entity& ) > f ) const {
        for( const auto& signId: syndrome ){
          Entity sign( unit );
          sign.id = signId;
          if( not f( sign ) ) return;
        }
      }

      std::vector< Identity > id() const {                                                                     // [+] 2021.04.30
        std::vector< Identity > ID;
        for( const auto& signId: syndrome ) ID.push_back( signId );
        return ID;
      }

      void print( Logger::Log& log, const char* prefix = "" ) const {
        std::stringstream S;
        S << prefix << unsigned( size() ) << " {";
        for( auto e: syndrome ) S << ' ' << int( e );
        S << " }";
        log.vital( S.str().c_str() );
      }

    };// Gnosis::Syndrome


    class Sequence: public Local {
                                                                                                                              /*
      Note: this class quite similar to Seq, but uses Entity as parameters instead of
            Identity, so Seq as undelying container hidden from user; it has also
            access to Gnosis and Entity internals.
                                                                                                                              */
      friend class Gnosis;
      friend class Entity;

      Seq sequence;

      Sequence( unsigned unit                 ): Local( unit ), sequence{     }{}
      Sequence( unsigned unit, const Seq& seq ): Local( unit ), sequence{ seq }{}

    public:

      Sequence( const Sequence& Q ): Local( Q.unit ), sequence{ Q.sequence }{}

      bool operator == ( const Sequence& Q ) const { return       mate( Q )  and   sequence == Q.sequence;   }
      bool operator != ( const Sequence& Q ) const { return ( not mate( Q ) ) or ( sequence != Q.sequence ); }

      void clear(){ sequence.clear(); }

      Sequence& operator += ( Entity e ){
        assert( mate( e )     );
        assert( e.id != CoreAGI::NIHIL );
        sequence += e.id;
        return *this;
      }

      Sequence& append( Entity e ){
        assert( mate( e )     );
        assert( e.id != CoreAGI::NIHIL );
        sequence += e.id;
        return *this;
      }

      Entity operator[]( size_t i ) const { Entity e{ unit }; e.id = sequence[i]; return e; }

      size_t size () const { return sequence.size ();     }
      bool   empty() const { return not bool( sequence ); }

      void reserve( unsigned capacity ){ sequence.reserve( capacity ); }

      void shrink (){ sequence.shrink(); }

      void process( std::function< bool( const Entity& ) > f ) const {
        for( auto& id: sequence ){
          Entity e{ unit }; e.id = id;
          if( not f( e ) ) return;
        }
      }

    };//class Gnosis::Sequence


    Syndrome syndrome() const { return Syndrome( ID ); }

    Syndrome syndrome( std::initializer_list< Entity > Y ){
      Syndrome S{ syndrome() };
      for( const auto& sign: Y ) S.incl( sign );
      return S;
    }

    Syndrome syndrome( const Entity& e ){
      Syndrome S{ syndrome() };
      S.incl( e );
      return S;
    }

    Entity entity( const Syndrome& syndrome ){
      Entity e{ entity() };
      for( const auto& id: syndrome.syndrome ){
        Entity sign{ ID };
        sign.id = id;
        e.incl( sign );
      }
      return e;
    }

    Sequence sequence() const { return Sequence( ID ); }

    Sequence sequence( std::initializer_list< Entity > E ){
      Sequence Q{ sequence() };
      for( const auto& elem: E ) Q.append( elem );
      return Q;
    }

                                                                                                                              /*
    SetOfEntities is an inner class of the Gnosis that represents of arbitrary subset of entities.
                                                                                                                              */
    class SetOfEntities: public Local { // Represents arbitrary subset of known entities:
                                                                                                                              /*
      Represents arbitrary subset of all entities of particular Gnosis instance
                                                                                                                              */
      friend class Gnosis;
      friend class Entity;

      BigSet S; // :(sub)sets of Entity` ID. Note: `Set` defined in the `config.h`
                                                                                                                              /*
      Private constructor acessible from Gnosis:
                                                                                                                              */
      SetOfEntities( unsigned unit ): Local( unit ), S{}{}

      bool contains( Identity i ) const { return S.contains( i ); }

    public:

      SetOfEntities( const SetOfEntities& M ): Local( unit ), S{ M.S }{}

      bool incl( const Entity& e ){
        assert( mate( e ) );
        return S.incl( Identity( e ) );
      }

      void excl( const Entity& e ){
        if( mate( e ) ) S.excl( Identity( e ) );
      }

      size_t size() const { return S.size(); }

      bool contains( const Entity& e ) const {
        assert( mate( e ) );
        return contains( Identity( e ) );
      }

      bool operator [] ( const Entity e ) const { return contains( e ); };

      SetOfEntities operator + ( const SetOfEntities& R ){
        assert( mate( R ) );
        SetOfEntities U{ *this };
        U.S = S + R.S;
        return U;
      }

      SetOfEntities operator - ( const SetOfEntities& R ){
        assert( mate( R ) );
        SetOfEntities U{ *this };
        U.S = S - R.S;
        return U;
      }

      SetOfEntities operator ^ ( const SetOfEntities& R ){
        assert( mate( R ) );
        SetOfEntities U{ *this };
        U.S = S ^ R.S;
        return U;
      }

      void process( std::function< bool( Identity ) > f ) const {                                              // [+] 2020.07.24
                                                                                                                              /*
        Call `f` for each `elem` in `S`; break loop if `f` returns `false`:
                                                                                                                              */
        for( auto id: S ) if( not f( id ) ) return;
      }

      void process( std::function< bool( const Entity& e ) > f ) const {                                       // [+] 2021.04.27
                                                                                                                              /*
        Call `f` for each `e` recovered from `S`; break loop if `f` returns `false`:
                                                                                                                              */
        for( auto id: S ){
          gnosis().log.sure( id != CoreAGI::NIHIL, "NIHIL id" );
          Entity e{ gnosis().recover( id ) };
          assert( bool( e ) );
          if( not f( e ) ) return;
        }
      }


    };// class Gnosis::SetOfEntities

                                                                                                                              /*
    Subsets constructor:
                                                                                                                              */
    SetOfEntities setOfEntities() const {
                                                                                                                              /*
      Creates empty subset:
                                                                                                                              */
      return SetOfEntities( ID );
    }
                                                                                                                              /*
    Select set of entities by single syndrome:
                                                                                                                              */
    unsigned select( const std::span< Syndrome >& syndrome, std::function< bool( unsigned, const Entity& ) > f ) const {
                                                                                                                              /*
      Convert syndromes into arrays represented by std::vector:
                                                                                                                              */
      constexpr size_t LIMIT{ 8 };

      const size_t N{ syndrome.size() };
      log.sure( N > 0, "`select` called with empty array of syndromes" );

      std::vector< Identity > Y[ LIMIT ];  assert( N < LIMIT );
      for( unsigned i = 0; i < N; i++ ) for( const auto& sign: syndrome[i].syndrome ) Y[i].push_back( sign );
                                                                                                                              /*
      Compose request:
                                                                                                                              */
      arena.reset();
      constexpr size_t RESULT_CAPACITY{ 512      }; // :capacity of selection for each segment
      Shard::Request   request [ NUMBER_OF_SEGMENTS ];
      bool             finished[ NUMBER_OF_SEGMENTS ];

      for( unsigned s = 0; s < NUMBER_OF_SEGMENTS; s++ ){
                                                                                                                              /*
        Compose request for s`th segment:
                                                                                                                              */
        request[s] = arena.span< Shard::Query >( N );
        for( unsigned i = 0; i < N; i++ ){
          const size_t Li{ syndrome[i].size() };
          request[s][i] = Shard::Query{
            syndrome: arena.span< Identity >( Li              ), // std::span< Identity >( syndromeA,  2 ),
            storage : arena.span< Identity >( RESULT_CAPACITY ), // std::span< Identity >( selectedA, 64 ),
            num     : 0,
            overrun : false
          };
          for( unsigned k = 0; k < Y[i].size(); k++ ) request[s][i].syndrome[ k ] = Y[i][k];
        }//for i
                                                                                                                              /*
        Start selection:
                                                                                                                              */
        finished[s] = false;
        log.sure( segments[s].select( request[s] ), kit( "Segment %s: selection failure", segments[s].name() ) );
      }//for s
                                                                                                                              /*
      Wait for selection completed and process results:
                                                                                                                              */
      unsigned totalSelected{ 0 };
      for(;;){
        std::this_thread::yield();
        bool done{ true };
        for( unsigned s = 0; s < NUMBER_OF_SEGMENTS; s++ ){
          if( finished[s] ) continue;
          if( segments[s].idling() ){
                                                                                                                              /*
            Process items selected by segment[s]:
                                                                                                                              */
            for( size_t i = 0; i < N; i++ ){ // Loop over syndromes:
              const size_t num{ request[s][i].num };
              assert( num <= request[s][i].storage.size() );
              totalSelected += num;
              for( size_t j = 0; j < num; j++ ) f( i, recover( request[s][i].storage[j] ) );                   // [+] 2020.12.23
            }
            finished[s] = true;
          } else {
            done = false;
          }
        }
        if( done ) break;
      }//for
      return totalSelected;

    }//select

    SetOfEntities setOfEntities( const Syndrome& syndrome ) const {
      SetOfEntities result{ setOfEntities() };
      Syndrome Y{ syndrome };
      select(
        std::span< Syndrome >( &Y, 1 ),
        [&]( unsigned syndromeIndex, Entity e )->bool{
          assert( syndromeIndex == 0 );
          result.incl( e );
          return true;
        }
      );
      return result;
    }//setOfEntities

    SetOfEntities setOfEntities( const Syndrome& syndrome, const Syndrome& tabu ) const {                      // [+] 2021.04.13
      SetOfEntities result{ setOfEntities() };
      Syndrome Y{ syndrome };
      select(
        std::span< Syndrome >( &Y, 1 ),
        [&]( unsigned syndromeIndex, Entity e )->bool{
          assert( syndromeIndex == 0 );
          if( ( e.S()*tabu ).empty() ) result.incl( e );
          return true;
        }
      );
      return result;
    }//setOfEntities

    Identity uniqueEntityId( const Syndrome& syndrome ) const {                                                    // [+] 2021.04.13
                                                                                                                              /*
      Find unique entity by syndrome:
                                                                                                                              */
      Syndrome Y { syndrome       };
      Identity id{ CoreAGI::NIHIL };
      select(
        std::span< Syndrome >( &Y, 1 ),
        [&]( unsigned syndromeIndex, Entity e )->bool{
          assert( syndromeIndex == 0 );
          if( id == CoreAGI::NIHIL ){ // Find first, continue search:
            id = Identity( e );
            return true;
          } else { // Find another, so no unique entity, set `u` back to NIHIL and break search
            id = CoreAGI::NIHIL;
            return false;
          }
          return true;
        }
      );
      return id;
    }

    Entity uniqueEntity( const Syndrome& syndrome, const Syndrome& tabu ){                              // [+] 2021.04.13
                                                                                                                              /*
      Find unique entity by syndrome and tabu:
                                                                                                                              */
      Syndrome Y { syndrome       };
      Identity id{ CoreAGI::NIHIL };
      select(
        std::span< Syndrome >( &Y, 1 ),
        [&]( unsigned syndromeIndex, Entity e )->bool{
          assert( syndromeIndex == 0 );
          auto M = e.S().syndrome;
          bool acceptable{ true };
          for( const auto& sign: M ) if( tabu.syndrome.contains( sign ) ){
            acceptable = false;
            break;
          }
          if( acceptable ){
            if( id == CoreAGI::NIHIL ){ // Find first
              id = Identity( e );
              return true; // :continue search
            } else { // Find one more; entity is not unique; set `id` back to NIHIL and finish search
              id = CoreAGI::NIHIL;
              return false;
            }
          }
          return true;
        }
      );
      return ( id == CoreAGI::NIHIL ) ? none() : recover( id );
    }

    SetOfEntities setOfEntities( std::initializer_list< Entity > syndrome ){
      SetOfEntities S{ setOfEntities() };
      for( const auto& sign: syndrome ) S.incl( sign );
      return S;
    }


    bool analogic(
      const Sequence&                                 pattern,
      const Syndrome&                                 mask,
      std::function< bool       ( const Sequence& ) > f,
      std::function< std::string( Identity        ) > lex   = nullptr, // :converts ID to string
      const char*                                     trace = nullptr  // :trace file` path
    ) const {
                                                                                                                              /*
      Seach for Entity` tuples analogical to provided `origin` tuple
                                                                                                                              */
      constexpr unsigned CAPACITY{ CAPACITY_OF_ANALOGY };                                                      // [m] 2021.06.14

      const uint8_t N = pattern.size();
      log.sure( N > 1,              "Single entity pattern"                         );
      log.sure( N <= CAPACITY, kit( "Too big pattern: %u; limit: %u", N, CAPACITY ) );
      for( unsigned i = 0; i < N; i++ ) log.sure( not mask.contains( pattern[i] ), "Mask contains pattern element" );
                                                                                                                              /*
      Syndrome has no default constructor, so std::vector used to form array:
                                                                                                                              */
      std::vector< Syndrome > Sx; // :`external` syndromes for each `variable`
      for( uint8_t i = 0; i < N; i++ ) Sx.push_back( pattern[i].S() );
                                                                                                                              /*
      Compose mapping local id <-> global id for subgraph nodes and compose array of syndromes
                                                                                                                              */
      using Candidates = std::vector< Identity >;

      std::map< Identity, uint8_t > M; // :map global ID -> local ID

      struct Node {
        Identity   global;     // :ID of Gnosis` pattern endity
        Candidates candidates; // :list of Gnosis` ID of candidates
        double     complexity; // :log10( number of candidates )
        Node(): global{ CoreAGI::NIHIL }, candidates{}, complexity{}{}
      };

      Node node[ CAPACITY ];  // :array of node data: local id -> global ID & node data

      for( uint8_t i = 0; i < N; i++ ){
        node[i].global = pattern[i].id;
        M[ pattern[i].id ] = i;
      }
                                                                                                                              /*
      Compose pattern subgraph as directed graph represened by adjacency matrix:
                                                                                                                              */
      [[maybe_unused]] bool D[ CAPACITY ][ CAPACITY ];
      for( uint8_t i = 0; i < N; i++ )
        for( uint8_t j = 0; j < N; j++ )
          D[i][j] = Sx[i].syndrome.contains( node[j].global );
                                                                                                                              /*
      Syndrome used for candidates selection for each pattern element should be `cleared`
      from masked signs and from `inner` signs:
                                                                                                                              */
      for( uint8_t i = 0; i < N; i++ ){
        for( const auto& s: mask.syndrome ) Sx[i].syndrome.excl( s );  // :exclude masked signs
        for( uint8_t j = 0; j < N; j++    ) Sx[i].excl( pattern[j] );  // :exclude pattern entities
      }
                                                                                                                              /*
      Check pattern` graph connectivity:
                                                                                                                              */
      {
        unsigned N{ 0 };
        bool visited[ CAPACITY ];

        const std::function< void( unsigned ) > traverse = [&]( unsigned u ){
          visited[u] = true; //mark v as visited
          for( unsigned v = 0; v < N; v++ ) if( D[u][v] and not visited[v] ) traverse( v );
        };

        for( unsigned u; u < N; u++ ){ // Check whether all nodes are accessible or not:
          for( unsigned i = 0; i < N; i++ ) visited[i] = false; //:initialize as no node is visited
          traverse( u );
          for( unsigned i = 0; i < N; i++ ) if( not visited[i] ){
            log.vital( "Pattern`s graph is not fully connected" );
            return false;
          };
        }//for u
      }
                                                                                                                              /*
      Fill up lists of candidates - avoid to include pattern element and entities
      with empty syndrome that obviously must be rejected:
                                                                                                                              */
      select(
        std::span< Syndrome >( Sx ),
        [&]( unsigned i, const Entity& e )->bool{
          if( e.id != node[i].global and not e.S().empty() ) node[i].candidates.push_back( e.id );             // [+] 2021.01.02
          return true;
        }
      );
                                                                                                                              /*
      Reduce list of candidates in case of self-loop:
                                                                                                                              */
      for( unsigned i = 0; i < N; i++ ){
        const Entity& Ei{ pattern[i] };
        if( Ei.is( Ei ) ){
                                                                                                                              /*
          Pattern contains self-loop, so candidates must be self-looped to:
                                                                                                                              */
          Node& Ni{ node[i] };
          std::vector< Identity > original( Ni.candidates );
          Ni.candidates.clear();
          for( Identity id: original ){
            Entity e{ recover( id ) };
            if( e.is( e ) ) Ni.candidates.push_back( e.id );
            if( lex ) log.vital( kit( "[analogic] Detected loop for %u:%s; set reduced from %u to %u",
                                         i, lex( Ei.id ).c_str(), original.size(), Ni.candidates.size() ) );
            else      log.vital( kit( "[analogic] Detected loop for %u:#%u; set reduced from %u to %u",
                                         i, Ei.id, original.size(), Ni.candidates.size() ) );
          }
        }
      }//for i

      if( lex ){
        log.vital();
        log.vital( "[analogic] Sizes of lists of node candidates:" );
      }
      for( uint8_t i = 0; i < N; i++ ){
        Node& Ni{ node[i] };
        const double n = Ni.candidates.size();
        Ni.complexity = ::log10( n );
        if( lex ) log.vital( kit( " %2u  %8.2e ~ %6.3f  %s", i, n, Ni.complexity, lex( Ni.global ).c_str() ) );
      }
                                                                                                                              /*
      Check for empty lists of candidates:
                                                                                                                              */
      { bool empty{ false };
        for( unsigned i = 0; i < N; i++ ) if( node[i].candidates.empty() ){
          if( lex ) log.vital( kit( "[analogic] No candidates for %u:%s", i, lex( node[i].global ).c_str() ) );
          else      log.vital( kit( "[analogic] No candidates for %u:#%u", i,     node[i].global           ) );
          empty = true;
        }
        if( empty ) return false;
      }
                                                                                                                              /*
      Make array of edges:
                                                                                                                              */
      struct Edge {
        uint8_t from;
        uint8_t into;
        double  RC;    // :residual complexity
      };
      Edge     edge[ CAPACITY*CAPACITY ];
      unsigned ord [ CAPACITY*CAPACITY ]; // :index of sorted list of edges
      unsigned Ne{ 0 }; // :number of edges
      for( uint8_t i = 0; i < N; i++ ) for( uint8_t j = 0; j < N; j++ ) if( D[i][j] ){
        double RC{ 0.0 };
        for( uint8_t p = 0; p < N; p++ ) if( p != i and p != j ) RC += node[p].complexity;
        edge[ Ne ] = Edge{ i, j, RC };
        ord [ Ne ] = Ne;
        Ne++;
      }
      if( lex ){
        log.vital();
        log.vital( "[analogic] List of edges with Residual Complexity:" );
        for( unsigned i = 0; i < Ne; i++ ){
          const unsigned Oi{ ord [ i  ] };
          const Edge&    Ei{ edge[ Oi ] };
          log.vital( kit( "  %7.3f  %3u %s -> %s",
            Ei.RC, Oi, lex( node[ Ei.from ].global ).c_str(), lex( node[ Ei.into ].global ).c_str() ) );
        }
      }
                                                                                                                              /*
      Sort edges by `residual complexity`:
                                                                                                                              */
      heapSort< unsigned >(
        ord, Ne,
        [&]( const unsigned i, const unsigned j )->int{
          auto& Vi{ edge[ i ] };
          auto& Vj{ edge[ j ] };
          if( Vi.RC < Vj.RC ) return  1;
          if( Vi.RC > Vj.RC ) return -1;
          return 0;
        }
      );
      if( lex ){
        log.vital();
        log.vital( "[analogic] Edges sorted by residual complexity (RC):" );
        for( unsigned i = 0; i < Ne; i++ ){
          const unsigned Oi{ ord [ i  ] };
          const Edge&    Ei{ edge[ Oi ] };
          log.vital( kit( "  %7.3f  %3u %s -> %s   ",
            Ei.RC, Oi, lex( node[ Ei.from ].global ).c_str(), lex( node[ Ei.into ].global ).c_str() ) );
        }
      }
                                                                                                                              /*
      Construct check sequence:
      - clear list of assigned nodes
      - mark all edges as not checked
      - check edge with biggest RC, break if fail
      - mark edge as checked
      - include into list of assigned nodes these two nodes
      - REPEAT WHILE non-checked nodes presented
        * make list of edges that connected to assigned nodes and not checked, and assign node(s) id need
        * sord list of such edges by decreasing RC
        * check them in such order, break if fail, else mark as checked
                                                                                                                              */
      enum Action: char{
        STOP, // :finish execution
        INIT, // :clear set of assigned nodes
        NODE, // :assign mapping node `from` to candidate; jump if no more candidates
        EDGE, // :test edge `from`->`into`;                jump if failed
        CALL  // :call provided `f` function
      };

      struct Code {
        Action   action;
        unsigned node;   // :node index
        unsigned into;   // :`into` node for TEST or ignored
        int      jump;   //
      };

      std::set< unsigned > assignedNodes;
      std::set< unsigned > testedEdges;

      const unsigned Oo{ ord [ 0  ] };
      const Edge&    Eo{ edge[ Oo ] }; // :start edge
      assignedNodes.insert( Eo.from );
      assignedNodes.insert( Eo.into );
      testedEdges  .insert( Oo      );

      if( lex ){
        log.vital();
        log.vital( kit( "[analogic] Start from edge %u:", Oo ) );
        log.vital( kit( "  %6.2f  %3u %s -> %s",
          Eo.RC, Oo, lex( node[ Eo.from ].global ).c_str(), lex( node[ Eo.into ].global ).c_str() ) );
        log.vital( kit( "[analogic] %u unassigned nodes, %u untested edges",
          N - assignedNodes.size(), Ne - testedEdges.size() ) );
      }
                                                                                                                              /*
      Initial part of code:
                                                                                                                              */
      constexpr unsigned CODE_CAPACITY{ CAPACITY*( CAPACITY + 1 ) + 4 }; // :max num. of edges + max num of vertices + extra
      Code code[ CODE_CAPACITY ];
      int vacant{ 0 }; // :first vacant position in the code array
      code[ vacant++ ] = Code{ action:STOP, node:0, into:0, jump: 0 };  // :STOP at the begin
      code[ vacant++ ] = Code{ action:INIT, node:0, into:0, jump: 0 };
      int jump{ 1 };
      code[ vacant++ ] = Code{ action:NODE, node:Eo.from, into:0, jump:jump }; jump = vacant - 1;
      code[ vacant++ ] = Code{ action:NODE, node:Eo.into, into:0, jump:jump }; jump = vacant - 1;
                                                                                                                              /*
      Check for edges between two first nodes:
                                                                                                                              */
      for( unsigned i = 0; i < N; i++ ){
        const Edge& Ei{ edge[i] };
        if( assignedNodes.contains( Ei.from ) and assignedNodes.contains( Ei.into ) ){
          code[ vacant++ ] = Code{ action:EDGE, node:Ei.from, into:Eo.into, jump:jump };
          testedEdges.insert( i );
        }
      }

      for(;;){

        if( lex ){
          std::stringstream S;
          S << "[analogic] assigned nodes: {";
          for( const auto i: assignedNodes ) S << ' ' << i << ':' << lex( node[i].global );
          S << " }";
          log.vital( S.str() );
        }
                                                                                                                              /*
        Make list of non-tested edges that can be tested as ord[]:
                                                                                                                              */
        double RC{ 0.0 };
        for( unsigned i = 0; i < N; i++ ) if( not assignedNodes.contains( i ) ) RC += node[i].complexity;
        unsigned ord    [ CAPACITY ];  // :index
        double   nextRC [ CAPACITY ];
        unsigned Np{ 0 };
        for( unsigned i = 0; i < Ne; i++ ){
          if( testedEdges.contains( i ) ) continue;
          const Edge Ei{ edge[i] };
          if(
            ( assignedNodes.contains( Ei.from ) and not assignedNodes.contains( Ei.into ) )
            or
            ( assignedNodes.contains( Ei.into ) and not assignedNodes.contains( Ei.from ) )
          ){
            nextRC[ Np ] = RC - ( assignedNodes.contains( Ei.from ) ? node[ Ei.into ].complexity : node[ Ei.from ].complexity );
            ord   [ Np ] = i;
            Np++;
          }
        }//for i

        if( Np == 0 ){
          log.vital( "[analogic] All edges tested" ); // DEBUG
          break;
        }

        if( lex ){
          log.vital();
          log.vital( "[analogic] potential edges to test:" );
          for( unsigned i = 0; i < Np; i++ ){
            const unsigned Oi{ ord[i] };
            const Edge& Ei{ edge[ Oi ] };
            log.vital( kit( "  %3i  %6.2f  %3u  %u:%s -> %u:%s",
              i, nextRC[ Oi  ],  Oi, Ei.from, lex( node[ Ei.from ].global ).c_str(),
                                     Ei.into, lex( node[ Ei.into ].global ).c_str() ) );
          }
        }//if lex
                                                                                                                                      /*
        Sort list of potential next edges:
                                                                                                                              */
        heapSort< unsigned >(
          ord, Np,
          [&]( const unsigned i, const unsigned j )->int{
            auto& Vi{ nextRC[i] };
            auto& Vj{ nextRC[j] };
            if( Vi < Vj ) return  1;
            if( Vi > Vj ) return -1;
            return 0;
          }
        );
        log.vital( "[analogic] sorted edges to test:" );
        for( unsigned i = 0; i < Np; i++ ){
          const unsigned Oi{ ord[i] };
          const Edge& Ei{ edge[ Oi ] };
          log.vital( kit( "  %3i  %6.2f  %3u  %u:%s -> %u:%s",
            i, nextRC[ Oi ], Oi, Ei.from, lex( node[ Ei.from ].global ).c_str(),
                                 Ei.into, lex( node[ Ei.into ].global ).c_str() ) );
        }//for i
                                                                                                                              /*
        Assign next node:
                                                                                                                              */
        unsigned bestNextEdge = ord[0];
        unsigned nextNode     = assignedNodes.contains( edge[ bestNextEdge ].from )
                              ? edge[ bestNextEdge ].into : edge[ bestNextEdge ].from;
                                                                                                                              /*
        Code:
                                                                                                                              */
        code[ vacant++ ] = Code{ action:NODE, node:nextNode, into:0, jump:jump }; jump = vacant - 1;
        assignedNodes.insert( nextNode );
        if( lex ) log.vital( kit( "[analogic] Assign node %u:%s", nextNode, lex( node[nextNode].global ).c_str() ) );
        RC = nextRC[ ord[0] ];
                                                                                                                              /*
        Check edges starting from the best one with index ord[0]:
                                                                                                                              */
        for( unsigned i = 0; i < Np; i++ ){
          auto& Oi = ord[i];
          const Edge& Ei{ edge[ Oi ] };
          if( not assignedNodes.contains( Ei.from ) ) continue;
          if( not assignedNodes.contains( Ei.into ) ) continue;
          testedEdges.insert( Oi );
          if( lex ) log.vital( kit( "[analogic] Test edge %u  %u:%s -> %u:%s",
            Oi, Ei.from, lex( node[ Ei.from ].global ).c_str(), Ei.into, lex( node[ Ei.into ].global ).c_str() ) );
                                                                                                                              /*
          Code:
                                                                                                                              */
          code[ vacant++ ] = Code{ action:EDGE, node:Ei.from, into:Ei.into, jump: jump };
        }
        if( lex ){
          log.vital( "[analogic] Untested edges:" );
          for( unsigned i = 0; i < Ne; i++ ){
            if( testedEdges.contains( i ) ) continue;
            const Edge& Ei{ edge[ i ] };
            log.vital( kit( " %3i  %u:%s -> %u:%s",
              i, Ei.from, lex( node[ Ei.from ].global ).c_str(), Ei.into, lex( node[ Ei.into ].global ).c_str() ) );
          }
        }

      }//forever

      code[ vacant++ ] = Code{ action:CALL, node:0, into:0, jump:jump }; // :last operations
                                                                                                                              /*
      Print code:
                                                                                                                              */
      if( lex ){
        log.vital( "[analogic] Code:" );
        log.vital();
        for( int i = 0; i < vacant; i++ ){
          const Code& Ci{ code[i] };
          switch( Ci.action ){
            case STOP:
              log.vital( kit( " %3u [ %3u ] STOP", i, Ci.jump ) );
              break;
            case INIT:
              log.vital( kit( " %3u [ %3u ] INIT", i, Ci.jump ) );
              break;
            case NODE:
              log.vital( kit( " %3u [ %3u ] NODE %2u:%s", i, Ci.jump, Ci.node, lex( node[ Ci.node ].global ).c_str() ) );
              break;
            case EDGE:
              log.vital( kit( " %3u [ %3u ] EDGE %2u:%s -> %2u:%s", i, Ci.jump,
                Ci.node, lex( node[ Ci.node ].global ).c_str(), Ci.into, lex( node[ Ci.into ].global ).c_str() ) );
              break;
            case CALL:
              log.vital( kit( " %3u [ %3u ] CALL", i, Ci.jump ) );
              break;
            default: assert( false );
          }
        }
        log.vital();
        log.flush();
      }//if lex

      std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // DEBUG

      FILE* out = nullptr;
      int Nt{ NUMBER_OF_THREADS };
      if( trace ){
        out = fopen( trace, "w" ); // :for debugging only
        Nt  = 1;
      }
      std::atomic< size_t > totalTests{ 0 };
                                                                                                                              /*
      Interpret code:
                                                                                                                              */
      auto run = [&]( int Nt, int t ){ // Nt ~ number of threads, t ~ thread index

        assert( t >= 0 and t < Nt );

        using Bitset = std::bitset < CAPACITY >;

        Bitset   det;             // :set of currently assigned variables
        int      len[ CAPACITY ]; // :length  of candidates lists
        int      num[ CAPACITY ]; // :indices of current candidates
        Identity var[ CAPACITY ]; // :identities of tested combination

        for( unsigned i = 0; i < N; i++ ){
          const int Li = node[i].candidates.size();
          len[i] = Li;                                                                                         // [-] 2021.01.02
          num[i] = ( i == Eo.from ) ? t - 1 : -1;
          if( out ){
            fprintf( out, "\n\n %u candidates for %u:%s:", Li, i, lex( node[i].global ).c_str() );
            for( const auto& c: node[i].candidates )
            fprintf( out, "\n   %8u %s", c, lex( c ).c_str() );
          }
        }

        constexpr int EXIT { 0 }; // :code index for exit
        constexpr int START{ 1 }; // :code index where main loop started
        constexpr int OUTER{ 2 }; // :code index where first node assigned
        int o{ START }; // :index of the  current intruction in the `code` array

        for(;;){

          assert( o >= 0 and o < vacant );

          Code& cmd{ code[o] };
          if( out ) fprintf( out, "\n [%3u] ", o );

          if( cmd.action == STOP ){
            if( out ) fprintf( out, "INIT" );
            break;
          }

          switch( cmd.action ){

            case INIT:
              if( out ){ fprintf( out, "INIT" ); fflush( out ); }
              det.reset();
              o++;
              break;

            case NODE:
              if( out ){ fprintf( out, "NODE %u", cmd.node ); fflush( out ); }
              { bool complete{ false };
                bool twin    { false };
                do{
                  if( cmd.node == Eo.from ) num[ cmd.node ] += Nt; else num[ cmd.node ]++;                     // [+] 2021.01.02
                  if( num[ cmd.node ] >= len[ cmd.node ] ){
                    complete = true;
                    if( out ){ fprintf( out, " complete" ); fflush( out ); }
                    break;
                  }
                  var[ cmd.node ] = node[ cmd.node ].candidates[ num[ cmd.node ] ];
                  if( out ){ fprintf( out, " <= %s (%u/%u)", lex( var[ cmd.node ] ).c_str(),
                                                           num[ cmd.node ], len[ cmd.node] ); fflush( out ); }
                  twin = false;
                  for( unsigned n = 0; n < N; n++ ){
                    if( n == cmd.node ) continue;
                    if( det[n] and var[n] == var[ cmd.node ] ){ twin = true; break; }
                  }
                  if( out ) if( twin ){ fprintf( out, " twin\n             " ); fflush( out ); }
                } while( twin );
                if( complete ){
                  if( o == OUTER ){
                    o = EXIT;
                    if( out ){ fprintf( out, "  EXIT" ); fflush( out ); }
                  } else {
                    det.reset( cmd.node );
                    num[ cmd.node ] = -1;
                    o = cmd.jump;
                    if( out ){ fprintf( out, "  JUMP to %u", o ); fflush( out ); }
                  }
                } else {
                  det.set( cmd.node );
                  o++;
                }
              }
              break;

            case EDGE:
              if( out ){ fprintf( out, "EDGE %u:%s -> %u:%s", cmd.node, lex( var[ cmd.node ] ).c_str(),
                                                              cmd.into, lex( var[ cmd.into ] ).c_str() ); fflush( out ); }
              totalTests++;
              {
                Entity from = recover( var[ cmd.node ] );
                Entity into = recover( var[ cmd.into ] );
                if( from.is( into ) ){
                  if( out ){ fprintf( out, " fit" ); fflush( out ); }
                  o++;
                } else {
                  o = cmd.jump;
                  if( out ){ fprintf( out, "  JUMP to %u", o ); fflush( out ); }
                }
              }
              break;

            case CALL:
              if( out ) fprintf( out, "CALL [" );
              {
                Sequence Q = sequence();
                for( unsigned i = 0; i < N; i++ ){
                  Q += recover( var[i] );
                  if( out ) fprintf( out, " %s", lex( var[i] ).c_str() );
                }
                if( out ){ fprintf( out, " ]" ); fflush( out ); }
                if( f( Q ) ){
                  o = cmd.jump;
                  if( out ){ fprintf( out, "  JUMP to %u", o ); fflush( out ); }
                } else {
                  o = EXIT;
                  if( out ){ fprintf( out, "  EXIT" ); fflush( out ); }
                }
              }
              break;

            default: assert( false );

          }//switch action

        }//forever

      };//run

      if( Nt == 1 ){
        run( 1, 0 );
      } else {
        std::vector< std::thread > T;
        for( int t = 0; t < Nt; t++ ) T.push_back( std::thread( run, Nt, t ) );
        for( auto& Ti: T ) Ti.join();
      }

      return true;

    }//analogic

  };//class Gnosis


  const Gnosis::Syndrome Gnosis::Entity::S() const {
    Gnosis&      G      { gnosis()        };
    Shard&       segment{ G.segment( id ) };
    const Signs* Y      { segment[id]     };
    Gnosis::Syndrome result { Y ? Gnosis::Syndrome{ unit, *Y } : Gnosis::Syndrome{ unit } };
     return result;
  }

  const Gnosis::Sequence Gnosis::Entity::Q() const {
    Gnosis&    G      { gnosis()        };
    Shard&     segment{ G.segment( id ) };
    const Seq* Q      { segment.Q( id ) };
    return Q ? Gnosis::Sequence{ unit, *Q } : Gnosis::Sequence{ unit };
  }


  std::vector< std::reference_wrapper< Gnosis > > Gnosis::instance;
  std::mutex                                      Gnosis::globalMutex;

  const              Gnosis::Syndrome S( const Gnosis::Entity& e ){ return e.S(); }                            // [+] 2020.12.25
  const              Gnosis::Sequence Q( const Gnosis::Entity& e ){ return e.Q(); }                            // [+] 2020.12.25
  const std::vector< Gnosis::Entity > E( const Gnosis::Entity& e ){ return e.E(); }                            // [+] 2020.12.25
  const std::vector< Gnosis::Entity > A( const Gnosis::Entity& e ){ return e.A(); }                            // [+] 2020.12.26

};//namespace CoreAGI

#endif // GNOSIS_H_INCLUDED
