#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED
                                                                                                                              /*
________________________________________________________________________________________________________________________________

 2020.04.14  Bug fixed in abend(..)

 2020.04.15  Added log( string ) method

 2020.04.28  Modified `writer`: write only when a butch records collected

 2020.04.29  Added default value of msg in abend

 2020.04.30  Capacity constants moved to Config::logger namespace

 2020.06.12  Collecting rest of channel`s messages added int merger

 2020.06.15  Added logger.sure(..) methods

 2020.07.22  Added `flush()` methods to `Logger` and `Log` classes

 2020.07.31  Added methods combines vital(..) & flush()

 2020.10.15  Logger::active() become public

 2020.11.22  Logger::newLog(..) added

 2020.11.28  `abend(..)` fixed (avoid possible non-zero-ended string using `snprintf`)

 TODO:

________________________________________________________________________________________________________________________________
                                                                                                                              */
#include <cassert>
#include <cstring>

#include <atomic>
#include <experimental/source_location>
#include <thread>
#include <sstream>

#include "config.h"
#include "def.h"
#include "queue.h"
#include "timer.h"

namespace CoreAGI {

  using Location = std::experimental::source_location;
  using Str      = std::string;

  namespace logging {

    struct Note {

      enum Type{ NONE = 0, VITAL = 1, BRIEF = 2, WORDY = 3 };

      double time;                                           // :time
      Type   type;                                           // :message type
      char   text[ Config::logger::LOG_RECORD_CAPACITY ];    // :log record

      Note(): time{}, type{ NONE }, text{ 0 } {}

      Note( double time, Type type  ): time{ time }, type{ type }, text{} {}

      bool operator == ( const Note& note ) const { return type == note.type; }
      bool operator != ( const Note& note ) const { return type != note.type; }

      operator bool() const { return( time > 0.0 ); }
    };

    struct Channel{

      Queue< Note, Config::logger::CHANNEL_QUEUE_CAPACITY > Q;
      std::thread::id threadId;                                       // :channel thread ID
      unsigned        ID;                                             // :index
      char            name[ Config::logger::CHANNEL_NAME_CAPACITY ];  // :channel/thread name
      Channel(): Q{}, threadId{}, ID{}, name{ 0 }{};

    };//Channel

    struct Record: Note {

      char channelName[ Config::logger::CHANNEL_NAME_CAPACITY ];

      Record(): Note{}, channelName{ 0 }{}

      Record( const Note note, const char name[ Config::logger::CHANNEL_NAME_CAPACITY ] ): Note{ note }{
        memcpy( channelName, name, Config::logger::CHANNEL_NAME_CAPACITY );
      }

      bool operator == ( const Record& record ) const { return type == record.type; }
      bool operator != ( const Record& record ) const { return type != record.type; }

    };

  }//namespace logging

  using namespace logging;

  struct Logger {

    static constexpr const char* VERSION{ "2020.11.28" };

    using Clock = std::chrono::steady_clock;
    using Time  = std::chrono::time_point< Clock >;

    Logger( const Logger& ) = delete;
    Logger& operator = ( const Logger& ) = delete;

    void flush() const { dump.store( true ); }

  private:

    Queue< logging::Record, Config::logger::LOGGER_QUEUE_CAPACITY > Q;

    Timer::Unit                 TIME_UNIT;
    char                        FORMAT[ Config::logger::FORMAT_CAPACITY ];
    char                        BRIEF [ Config::logger::  PATH_CAPACITY ];
    char                        WORDY [ Config::logger::  PATH_CAPACITY ];
    Time                        startTime;
    std::atomic< bool >         mergerStarted;      // :thread start       flag
    std::atomic< bool >         writerStarted;      // :thread start       flag
    std::atomic< bool >         terminateMerger;    // :thread termination flag
    std::atomic< bool >         terminateWriter;    // :thread termination flag
    std::atomic< bool >         writerFinished;     // :thread termination flag
    std::atomic< bool > mutable dump;       // :write out all collected imediately
    logging::Channel            channel[ Config::logger::CHANNEL_CAPACITY ];
    std::thread                 mergerThread;
    std::thread                 writerThread;

    double time() const {
      return Timer::UNIT[ TIME_UNIT ]*double(
        std::chrono::duration_cast< std::chrono::nanoseconds >( Clock::now() - startTime ).count()
      );
    }

    void merger(){
      std::this_thread::yield();
      mergerStarted.store( true );
      while( not terminateMerger.load() ){
        for( auto& Ci: channel ){
          const auto EMPTY_NOTE{ Ci.Q.nihil() };
          while( not Ci.Q.empty() ){
            auto note = Ci.Q.pull();
            assert( note != EMPTY_NOTE );                                                                               // DEBUG
            const logging::Record record{ note, Ci.name };
            for(;;){
              if( Q.push( record ) ) break;
              std::this_thread::yield();
            }
          }
        }//while Ci not empty
//      std::this_thread::yield();                                                                             // [-] 2021.06.08
        std::this_thread::sleep_for( std::chrono::milliseconds( Config::logger::NO_JOB_PAUSE ) );              // [-] 2021.06.08
      }//while
                                                                                                                              /*
      Collect rest of channel`s messages after writing stopped:                                                   [+] 2020.06.12
                                                                                                                              */
      for( auto& Ci: channel ){
        const auto EMPTY_NOTE{ Ci.Q.nihil() };
        while( not Ci.Q.empty() ){
          auto note = Ci.Q.pull();
          assert( note != EMPTY_NOTE );                                                                                 // DEBUG
          const logging::Record record{ note, Ci.name };
          for(;;){
            if( Q.push( record ) ) break;
            std::this_thread::yield();
          }
        }
      }//while Ci not empty

    }//merger

    void write( FILE* brief, FILE* wordy, const logging::Record& record ){
      assert( strlen( record.text ) < Config::logger::LOG_RECORD_CAPACITY );
      constexpr unsigned LINE_CAPACITY{ Config::logger::LOG_RECORD_CAPACITY + Config::logger::FORMAT_CAPACITY };
      char line[ LINE_CAPACITY ];
      snprintf( line, LINE_CAPACITY-1, FORMAT, record.time, record.channelName, record.text );
      if( record.type           == logging::Note::VITAL )   puts( line        );
      if( brief and record.type != logging::Note::WORDY ){ fputs( line, brief ); fputc( '\n', brief ); }; // putr( brief );
      if( wordy                                         ){ fputs( line, wordy ); fputc( '\n', wordy ); }; // putr( wordy );
    };

    void writer(){
      unsigned constexpr BATCH{ Config::logger::LOGGER_QUEUE_CAPACITY/4 };
      printf( "\n Logger`writer: started" );  fflush( stdout ); // DEBUG
      std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      writerStarted.store( true );
      assert( writerStarted.load() );
      FILE* brief{ nullptr };
      FILE* wordy{ nullptr };
      if( strlen( BRIEF ) > 0 ){
        brief = fopen( BRIEF, "w" );
        assert( brief );
        printf( "\n Logger`writer: brief log file created" ); // DEBUG
      } else {
        printf( "\n Logger`writer: brief log not requested" ); // DEBUG
      }
      if( strlen( WORDY ) > 0 ){
        wordy = fopen( WORDY, "w" );
        assert( wordy );
        printf( "\n Logger`writer: wordy log file created" ); // DEBUG
      } else {
        printf( "\n Logger`writer: wordy log not requested" ); // DEBUG
      }
      printf( "\n\n" ); fflush( stdout );
                                                                                                                              /*
      Main loop:
                                                                                                                              */
      while( not terminateWriter.load() ){
        unsigned n{ 0 };
        if( dump.load() or ( Q.size() >= BATCH ) ) while( not Q.empty() ){
          auto const record = Q.pull();
          write( brief, wordy, record );
          n++;
        }
        if( n > 0 ) dump.store( false );
//      std::this_thread::yield();                                                                             // [-] 2021.06.08
        std::this_thread::sleep_for( std::chrono::milliseconds( Config::logger::NO_JOB_PAUSE ) );              // [-] 2021.06.08
      }
                                                                                                                              /*
      Final writing:
                                                                                                                              */
      const logging::Record NIHIL{ Q.nihil() };
      while( not Q.empty() ){
        auto const record = Q.pull();
        assert( record != NIHIL );
        write( brief, wordy, record );
      }
                                                                                                                              /*
      Files closing:
                                                                                                                              */
      if( wordy ){
        printf( "\n Logger`writer: close wordy log file.." ); // DEBUG
        fflush( wordy );
        fclose( wordy );
      }
      if( brief ){
        printf( "\n Logger`writer: close brief log file.." ); // DEBUG
        fflush( brief );
        fclose( brief );
      }
      printf( "\n Logger`writer: finished\n\n" );  fflush( stdout ); // DEBUG
      writerFinished.store( true );
    }

    void start(){
                                                                                                                              /*
      NB: Method called from Logger.log(..)
                                                                                                                              */
      mergerThread = std::thread( &Logger::merger, this );
      while( not mergerStarted.load() ) std::this_thread::yield();
      writerThread = std::thread( &Logger::writer, this );
      while( not writerStarted.load() ){
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        std::this_thread::yield();
      }
    }

  public:

    Logger( Timer::Unit timeUnit = Timer::MILLISEC ):
      Q                 {              },
      TIME_UNIT         { timeUnit     },
      FORMAT            {              },
      BRIEF             { 0            },
      WORDY             { 0            },
      startTime         { Clock::now() },
      mergerStarted     { false        },
      writerStarted     { false        },
      terminateMerger   { false        },
      terminateWriter   { false        },
      writerFinished    { false        },
      dump              { false        },
      channel           {              },
      mergerThread      {              },
      writerThread      {              }
    {
      strncpy( FORMAT, Config::logger::DEFAULT_FORMAT, Config::logger::FORMAT_CAPACITY-1 );
      printf( "\n Logger: log record format: `%s`, time unit: %s\n", FORMAT, Timer::lex( TIME_UNIT ) ); fflush( stdout );
    }

    bool active() const { return not terminateMerger.load(); }

    Timer::Unit timeUnit() const { return TIME_UNIT; }

    void update( logging::Note::Type type, const char* val ){
      // Note: val == nullptr allowed
      assert( not mergerStarted.load() );
      printf( "\n Logger`update %u: `%s`", type, val ); // DEBUG
      switch( type ){
        case logging::Note::VITAL:
          strncpy( FORMAT, val ? val : Config::logger::DEFAULT_FORMAT, Config::logger::FORMAT_CAPACITY-1 );
          break;
        case logging::Note::BRIEF:
          strncpy( BRIEF, val, Config::logger::PATH_CAPACITY );
          break;
        case logging::Note::WORDY:
          strncpy( WORDY, val, Config::logger::PATH_CAPACITY );
          break;
        default:
          break;
      }
      //printf( "\n Logger: update finished; BRIEF: `%s`; WORDY: `%s`", BRIEF, WORDY );                                 // DEBUG
    }

    void shutdown( unsigned error = 0 ){
                                                                                                                              /*
      Termination sequence called from destructor
                                                                                                                              */
      // printf( "\n Logger`shutdown started" ); fflush( stdout );
      // Finish merger first:
      terminateMerger.store( true );
      // printf( "\n Logger`shutdown: wait for merger thread joined" ); fflush( stdout );
      if( mergerThread.joinable() ) mergerThread.join();

      // Finish writer:
      // printf( "\n Logger`shutdown tell writer to finish" ); fflush( stdout );
      terminateWriter.store( true );

      // printf( "\n Logger`shutdown: wait for writer finished" ); fflush( stdout );
      while( not writerFinished.load() ) std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

      // printf( "\n Logger`shutdown: join writer thread" ); fflush( stdout );
      if( writerThread.joinable() ) writerThread.join();
      // printf( "\n Logger`shutdown finished, error: %u\n", error ); fflush( stdout );
      if( error > 0 ) exit( error );
    }

   ~Logger(){ shutdown(); }

    bool put( unsigned channelIndex, const Note::Type type, const char* msg ){                                 // [m] 2021.06.04
                                                                                                                              /*
      Check correctness of the channel` index; in case of cross-thread access
      print info and stop:
                                                                                                                              */
      if( channel[ channelIndex ].threadId != std::this_thread::get_id() ){
        //std::stringstream So; So << channel[ channelIndex ].threadId;
        //std::stringstream St; St << std::this_thread::get_id();
        printf( "\n\n [Logger.put] FATAL ERROR: Cross-thread access\n" );
        printf( "\n   msg:           `%s`", msg          );
        switch( type ){
          case Note::VITAL : printf( "\n   type:           VITAL" ); break;
          case Note::BRIEF : printf( "\n   type:           BRIEF" ); break;
          case Note::WORDY : printf( "\n   type:           WORDY" ); break;
          default          : assert( false );
        }
        printf( "\n   channel:        %u %s", channelIndex, channel[ channelIndex ].name );
        //printf( "\n   channel thread: %s",    So.str().c_str() );
        //printf( "\n   this    thread: %s",    St.str().c_str() );
        printf( "\n\n ABNORMAL END\n\n" );
        fflush( stdout );
        exit( 1 );
      }
                                                                                                                              /*
      Split long message into sequence of parts with acceptable length.
      Message may contains terminal escape sequences `... RESET` (colorizing)
      that shouldn`t be splitted.
                                                                                                                              */
      constexpr int MAXLEN{ int( Config::logger::LOG_RECORD_CAPACITY ) };

      const double t   { time() };
      const char*  tail{ msg    }; while( *tail ) tail++; // :zero at the end of `msg`
      const char*  TAIL{ tail   };
      const char*  head{ msg    };
      do{
                                                                                                                              /*
        Prepare `Note` struct:
                                                                                                                              */
        logging::Note note{ t, type };
        memset( note.text, 0, Config::logger::LOG_RECORD_CAPACITY );
                                                                                                                              /*
        Find head and tail of the chunk by acceptable length:
                                                                                                                              */
        const char* tail = TAIL;
        int len = tail - head;
        if( len >= MAXLEN ){
                                                                                                                              /*
          Find an end of the started at `head` and ended by space:
                                                                                                                              */
          while( tail >= head ){
            if( std::isspace( *tail ) and ( len = tail - head ) < MAXLEN ) break;
            tail--;
          }
          if( len == 0 ){
                                                                                                                              /*
             Message is too long but has no spaces:
                                                                                                                              */
             constexpr const char* WARNING( "< something too long with no spaces >" );
             static_assert( std::strlen( WARNING ) < Config::logger::LOG_RECORD_CAPACITY );
             std::strcpy( note.text, WARNING );
             return false;
          }
        }
                                                                                                                              /*
        Push chunk into channel:
                                                                                                                              */
        memcpy( note.text, head, len );
        assert( strlen( note.text ) < Config::logger::LOG_RECORD_CAPACITY ); // DEBUG                                                                      // [+] 2021.06.04
        if( not channel[ channelIndex ].Q.push( note ) ) return false;
                                                                                                                              /*
        Move head:
                                                                                                                              */
        head += len;
      } while( head < TAIL );
      return true;
    }//put

    bool put( unsigned channelIndex, Note::Type type, const Str& msg ){
      if( terminateMerger.load() ) return false;
      return put( channelIndex, type, msg.c_str() );
    }

    void vacate( unsigned channelIndex ){ channel[ channelIndex ].threadId = std::thread::id(); }

    class Log {
      Logger&  logger;
      unsigned channelIndex;
    public:
      Log() = delete;
      Log& operator = ( const Log& ) = delete;
      Log( Logger& logger, unsigned i ): logger{ logger }, channelIndex( i ){}

     ~Log(){ logger.vacate( channelIndex ); }

      void flush() const { logger.flush(); }

      void operator()( const char* msg ) const { logger.put( channelIndex, Note::VITAL, msg ); flush(); }      // [+] 2020.07.31
      void operator()( const Str&  msg ) const { logger.put( channelIndex, Note::VITAL, msg ); flush(); }      // [+] 2020.07.31

      bool vital( const char* msg = "" ) const { return logger.put( channelIndex, Note::VITAL, msg ); }
      bool brief( const char* msg = "" ) const { return logger.put( channelIndex, Note::BRIEF, msg ); }
      bool wordy( const char* msg = "" ) const { return logger.put( channelIndex, Note::WORDY, msg ); }

      bool vital( const Str&  msg      ) const { return logger.put( channelIndex, Note::VITAL, msg ); }
      bool brief( const Str&  msg      ) const { return logger.put( channelIndex, Note::BRIEF, msg ); }
      bool wordy( const Str&  msg      ) const { return logger.put( channelIndex, Note::WORDY, msg ); }

      void abend( const char* msg = "", const Location& L = Location::current() ) const {                      // [m] 2020.04.29
        std::stringstream S;
        S << "ABEND [" << L.file_name() << ':' << L.line() << '`' << L.function_name() << "]: " << msg;
        vital( S.str() );
        logger.shutdown();
      }

      void abend( const Str& msg, const Location& L = Location::current() ) const { abend( msg.c_str(), L ); }

      void sure( bool condition, const char* msg, const Location& L = Location::current() ) const {
        if( not condition ) abend( msg, L );
      }

      void sure( bool condition, const Str& msg, const Location& L = Location::current() ) const {
        if( not condition ) abend( msg, L );
      }

    };

    Log log( const char* name ){
      if( not mergerStarted.load() ) start();
      //printf( "\n\n Create Log `%s`.. ", name ); fflush( stdout ); // DEBUG
      assert( name );
      std::thread::id NONE;
      for( unsigned i = 0; i < Config::logger::CHANNEL_CAPACITY; i++ ){
        if( channel[i].threadId == NONE ){
          // strncpy( channel[i].name, name, Config::logger::CHANNEL_NAME_CAPACITY - 1 );
          const char* last{ channel[i].name + sizeof( channel[i].name ) - 1 };
          const char* from;
          char* into;
          for( from = name, into = channel[i].name; *from && from < last; from++, into++ ) *into = *from;
          *(into++) = '\0';
          channel[i].threadId = std::this_thread::get_id();
          //printf( " [ok]\n" ); fflush( stdout );// DEBUG
          return Log( *this, i );
        }
      }
      printf( "\n\n ABEND: too many logging channels" );
      exit( 1 );
    };

    Log* newLog( const char* name ){                                                                           // [+] 2020.11.22
      if( not mergerStarted.load() ) start();
      //printf( "\n\n Create Log `%s`.. ", name ); fflush( stdout );                                                    // DEBUG
      assert( name );
      std::thread::id NONE;
      for( unsigned i = 0; i < Config::logger::CHANNEL_CAPACITY; i++ ){
        if( channel[i].threadId == NONE ){
          // strncpy( channel[i].name, name, Config::logger::CHANNEL_NAME_CAPACITY - 1 );
          const char* last{ channel[i].name + sizeof( channel[i].name ) - 1 };
          const char* from;
          char* into;
          for( from = name, into = channel[i].name; *from && from < last; from++, into++ ) *into = *from;
          *(into++) = '\0';
          channel[i].threadId = std::this_thread::get_id();
          //printf( " [ok]\n" ); fflush( stdout );// DEBUG
          return new Log( *this, i );
        }
      }
      printf( "\n\n ABEND: too many logging channels" );
      exit( 1 );
    };

    Log log( std::string name ){ return log( name.c_str() ); }                                                 // [+] 2020.04.15

    void sure( bool condition, const char* msg, const Location& L = Location::current() ){                     // [+] 2020.06.15
      if( not condition ) log( "ABEND" ).abend( msg, L );
    }

    void sure( bool condition, const Str& msg, const Location& L = Location::current() ){                      // [+] 2020.06.15
      sure( condition, msg.c_str(), L );
    }

  };//Logger

}//namespace CoreAGI

#endif
