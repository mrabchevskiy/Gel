#ifndef SEGMENT_H_INCLUDED
#define SEGMENT_H_INCLUDED
                                                                                                                              /*
  2019.02.17 Initial version

  2020.04.15 Replaced `printf` by Logger-based logging

  2020.07.11 Some simplifications made

  2020.07.27 Method `exists` added;
             Removed forward declaration of `Gnosis`

  2020.11.10 Added Segment.forget(.)

  2020.11.11 Switch to segment size defined at activation time

  2020.11.12 Upgraded `Query` for search for a few syndromes in a single query

  2020.12.03 Refactoring:
             - `Segment` extends `Flat::Map`;
             - Thread function become static method of the `Segment`;
             - some fields removed and smoe other added;
             - `start(..)` parameters changed
             - a few other methods added

  2020.12.07 One more refactoring

  2020.12.29 Selection logic modified: empty syndrome mean all entities must be selected

________________________________________________________________________________________________________________________________
                                                                                                                              */
#include <atomic>
#include <cassert>
#include <random>
#include <thread>
#include <vector>
#include <span>     // :require -std=c++-20 (so use gcc-10 or later)

#include "config.h"
#include "flat_hash.h"
#include "flat.h"
#include "seq.h"                                                                                               // [m] 2020.07.29

namespace CoreAGI {

  using namespace Flat;


  template< unsigned CAPACITY > class Segment: public Map< Signs, CAPACITY > {
                                                                                                                              /*
    Segment is an active component of the Gnosis, accessible
    exclusively from Gnosis instance.
                                                                                                                              */
  public:

    using Array = std::span< Identity >;

    struct Query {
                                                                                                                              /*
      Contains wanted syndromes as array of `Identity` ( not as set! ) represented
      by std::span; array of the such syndromes is represented by std::span as well.
      Storage provided space for the found entities as an array of `Identity`
      represented as std::span.
      Number of found ones stored into `num` that can be > 0 at the start.
      Search breaks when num == storage.size().
                                                                                                                              */
      Array    syndrome;
      Array    storage;
      unsigned num;
      bool     overrun;

      void reset(){ num = 0; overrun = false; }

    };

    using Request = std::span< Query >;

    static void expose( const Request& request, const char* title = "" ){
      char  buffer[ 2048 ]; // :output buffer
      char* out{ buffer };  // :first available position in the buffer
      const size_t len{ request.size() };
      out += sprintf( out, "\n Request %s  size: %lu", title, len );
      for( size_t i = 0; i < len; i++ ){
        const Query& Qi{ request[i] };
        out += sprintf( out, "\n   Query %lu  num: %u, overrun: %s", i, Qi.num, Qi.overrun ? "yes" : "no " );
        out += sprintf( out, "\n     syndrome: {" );
        for( const auto& id: Qi.syndrome ) out += sprintf( out, " %u", id );
        out += sprintf( out, " }" );
        out += sprintf( out, "\n     storage:  {" );
        for( unsigned j = 0; j < Qi.num; j++ ) out += sprintf( out, " %u", Qi.storage[j] );
        out += sprintf( out, " }" );
        assert( out < buffer + 2048 );
      }
      puts( buffer );
      fflush( stdout );
    }

    friend class Entity;
    friend class Gnosis;

  private:
                                                                                                                              /*
    Note: `log` MUST be called from `permanent` method only, i.e. exclusively
          from the service thread; call `log` from any other thread wil cause
          fatal error `cross-thread access`
                                                                                                                              */
    using E2Sequence = ska::flat_hash_map< Identity, Seq, IdentityHash >; // :map entity -> sequence

    E2Sequence  sequences;       // :map entity ID to entity name
    std::thread thread;          // :permanently active thread
    unsigned    id;              // :index in the array of segmants
    char        NAME[ Config::logger::CHANNEL_NAME_CAPACITY ];

    mutable std::atomic< Request* > request;           // :selection argumens and storage
    mutable std::atomic< bool     > live;
    mutable std::atomic< bool     > idle;
    mutable std::atomic< bool     > stop;

  public:

    Segment(): Map< Signs, CAPACITY >{},
      sequences{                     },
      thread   {                     },
      id       { 0                   },
      NAME     {                     },
      request  { nullptr             },
      live     { false               },
      idle     { true                },
      stop     { false               }
    {}

    Segment( const Segment& ) = delete;
    Segment& operator = ( const Segment& ) = delete;

    bool start( const char* name, unsigned ID, double timeout = 100.0 /* millisec */  ){               // [m] 2020.11.21
                                                                                                                              /*
      Note: `id` is a segment index in range [ 0 .. NUMBER_OF_SEGMENTS-1 ];
      names of segment use common stem and suffixes [ 1..NUMBER_OF_SEGMENTS ]
                                                                                                                              */
      assert( name                                                   );
      assert( strlen( name ) < Config::logger::CHANNEL_NAME_CAPACITY );
      assert( ID < Config::gnosis::NUMBER_OF_SEGMENTS                );
      strcpy( NAME, name );
      id = ID;
                                                                                                                              /*
      Start service thread:
                                                                                                                              */
      stop.store( false );
      live.store( false );
      thread = std::thread( service, this );
      while( not live.load() ) std::this_thread::yield();
      Timer t;
      while( not live.load() ){
        std::this_thread::yield();
        if( t() > timeout ){
          printf( "\n [Segment.start] Timeout\n" ); fflush( stdout );
          return false;
        }
      }
      return true;
    }

    const char* name() const { return NAME; }

    bool active() const { return live.load(); }
    bool idling() const { return idle.load(); }
                                                                                                                              /*
    Stop service thread:
                                                                                                                              */
    bool terminate(){
      stop.store( true );
      if( thread.joinable() ) thread.join();
      return true;
    }

   ~Segment(){ terminate(); }

    void seq( const Identity id, const Seq& seq = Seq{} ){                                                     // [+] 2020.07.29
                                                                                                                              /*
      Assign entity` sequence:
                                                                                                                              */
      if( seq.size() > 0 ) sequences[id] = seq; else sequences.erase( id );
    }

    const Seq* Q( const Identity id ) const {                                                                  // [m] 2020.07.29
                                                                                                                              /*
      Get acces to entity` sequence:
                                                                                                                              */
      static const Seq NONE{};
      const auto& it = sequences.find( id );
      if( it == sequences.end() ) return nullptr;
      return &( it->second );
    }
                                                                                                                              /*
    Start search for requested data:
                                                                                                                              */
    bool select( Request& R, [[maybe_unused]]double timeout = 1000.0 /* millisec */ ) const {
      constexpr unsigned ATTEMPT_LIMIT{ 8 };
      bool IDLE{ true };
      for( unsigned attempt = 0; attempt < ATTEMPT_LIMIT; attempt++ ){
        if( idle.compare_exchange_weak( IDLE, false ) ){
          Request* EXPECTED{ nullptr };
          for( unsigned attempt = 0; attempt < ATTEMPT_LIMIT; attempt++ ){
            if( request.compare_exchange_weak( EXPECTED, &R ) ) return true;
            std::this_thread::yield();
          }
        }
        std::this_thread::yield();
      }
      return false;
    }

    size_t forgotten( Identity sign ){
                                                                                                                              /*
      Exclude `id` for each syndrome; return number of actual excluded.

      NB: key == `id` NOT excluded for `syndromes` and from `sequences`, so only syndromes can be modified.
                                                                                                                              */
      Flat::Map< Signs, CAPACITY >& M{ *this }; // :use `this` as Map
                                                                                                                              /*
      Iterator over segment has only `const` version, so list of keys to be modified
      should be created then modification will be made for each key:
                                                                                                                              */
      std::vector< Identity > toBeProcessed;
      for( auto& entry: M ) if( entry.val.contains( sign ) ) toBeProcessed.push_back( entry.key );
      for( const auto key: toBeProcessed ) M[key]->excl( sign );
      return toBeProcessed.size();
    }

    void process( std::vector< Identity >& data, std::function< bool( std::vector< Identity >& ) > f ) const {                             // [+] 2021.04.29
                                                                                                                              /*
      Call `f` with array of entity ID followed by entity's signs ID:
                                                                                                                              */
      for( const auto& entry: *this ){
        data.clear();
        data.push_back( entry.key );
        Signs syndrome = entry.val;
        for( const auto signId: syndrome ) data.push_back( signId );
        if( not f( data ) ) break;
      }
    }

  private:

    void clear(){
      Map< Signs, CAPACITY >::clear();
      sequences.clear(); assert( sequences.size() == 0 );
    }

    unsigned saveSyndromes( FILE* out ) const {
      unsigned n{ 0 };
      for( const auto& entry: *this ){
        Encoded< Identity > EncodedEntityId( entry.key );
        fprintf( out, "%s", EncodedEntityId.c_str() );
        Signs syndrome = entry.val;
        for( const auto signId: syndrome ){
          Encoded< Identity > EncodedSignId{ signId };
          fprintf( out, " %s", EncodedSignId.c_str() );
        }
        fprintf( out, "\n" );
        n++;
      }
      return n;
    }

    unsigned saveSequences( FILE* out ) const {
      auto write = [&]( const Identity& id )->bool{
        Encoded< Identity > EncodedEntityId( id );
        fprintf( out, " %s", EncodedEntityId.c_str() );
        return true;
      };
      unsigned n{ 0 };
      for( const auto& entry: sequences ){
        Encoded< Identity > EncodedEntityId( entry.first );
        fprintf( out, "%s", EncodedEntityId.c_str() );
        entry.second.process( write );
        fprintf( out, "\n" );
        n++;
      }
      return n;
    }

    static void service( const Segment* segment ){
                                                                                                                              /*
      The main thread function.
                                                                                                                              */
      assert( segment );
      const Map< Signs, CAPACITY >& M{ *segment }; // :interprets segment as Map
      segment->stop.store( false );
      segment->live.store( true  );
      segment->idle.store( true  );
      Request* R{ nullptr };
      while( not segment->stop.load() ){
        R = segment->request.load();
//      if( R == nullptr ){ std::this_thread::yield(); continue; } // :no request                              // [-] 2021.06.08
        if( R == nullptr ){ // No request:
          if( Config::gnosis::spurt.load() ){                                                                  // [+] 2021.06.08
            std::this_thread::yield();
          } else {
            std::this_thread::sleep_for( std::chrono::milliseconds( Config::gnosis::NO_JOB_PAUSE ) );
          }
          continue;
        }
        assert( R             );
        assert( R->size() > 0 );
        for( auto& query: *( R ) ) query.overrun = ( query.storage.size() == 0 );
        for( auto& entry: M ){
          for( auto& query: *( R ) ){
            if( query.overrun                            ) continue; // :storage space exhausted, don't check
            if( query.syndrome.size() > entry.val.size() ) continue; // :request`syndrome longer that currently tested one
//          if( entry.val.contains( query.syndrome ) ){                                                         //[-] 2020.12.29
            if( query.syndrome.size() == 0 or entry.val.contains( query.syndrome ) ){                           //[+] 2020.12.29
              query.storage[ query.num ] = entry.key;
              query.overrun = ( ++query.num >= query.storage.size() );
            }
          }//for Qi
        }//for entry
        segment->request.store( nullptr ); // :disconnecting from the served thread
        segment->idle.store( true );
        std::this_thread::yield(); // :NB it doesn't seem to be required
      }//for
      segment->live.store( false );
      segment->stop.store( false );
    }//permanent

  };//class Segment

}//namespace CoreAGI

#endif // SEGMENT_H_INCLUDED
