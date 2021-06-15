                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 HMI Terminal

 2021.05.17 Modified
 2021.06.15 Modified
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED

#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <atomic>
#include <thread>

#include "active.h"
#include "channel.h"
#include "color.h"
#include "def.h"
#include "random.h"
#include "codec.h"
#include "keyboard.h"

namespace CoreAGI {

  class Terminal: public Active {

    static constexpr unsigned WAIT_TIMEOUT{ 5000 }; // :millisec

    Communication::Channel channel;
    Keyboard               keyboard;

  public:

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

    Terminal( int thisPort, const char* peerAddr, int peerPort ):
      Active  {                              },
      channel { thisPort, peerAddr, peerPort },
      keyboard{                              }
    {
      while( not channel .live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
      while( not keyboard.live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
      std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
      start();
      while( not live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

   ~Terminal(){
      keyboard.stop();
      while( keyboard.live() ) std::this_thread::yield();
      channel.stop();
      while( channel.live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

  private:

    virtual void run() override {

      bool debug{ false };

      constexpr char ESC      {  27 };
      constexpr char BACKSPACE{ 127 };

      const KeySeq F1   { ESC, 79, 80, 0,    0, 0, 0, 0 };
      const KeySeq F2   { ESC, 79, 81, 0,    0, 0, 0, 0 };
      const KeySeq F3   { ESC, 79, 82, 0,    0, 0, 0, 0 };
      const KeySeq F4   { ESC, 79, 83, 0,    0, 0, 0, 0 };

      const KeySeq F5   { ESC, 91, 49, 53, 126, 0, 0, 0 };

      [[maybe_unused]]
      const KeySeq F6   { ESC, 91, 49, 55, 126, 0, 0, 0 }; // :reserved

      [[maybe_unused]]
      const KeySeq F7   { ESC, 91, 49, 56, 126, 0, 0, 0 }; // :reserved

      const KeySeq F8   { ESC, 91, 49, 57, 126, 0, 0, 0 };

      const KeySeq LEFT { ESC, 91, 68,  0,   0, 0, 0, 0 };
      const KeySeq RIGHT{ ESC, 91, 67,  0,   0, 0, 0, 0 };
      const KeySeq DEL  { ESC, 91, 51,126,   0, 0, 0, 0 };
      const KeySeq HOME { ESC, 91, 72,  0,   0, 0, 0, 0 };
      const KeySeq ENDL { ESC, 91, 70,  0,   0, 0, 0, 0 };

      enum Who { AGI, SYS, YOU };

      auto WHO = [&]( Who who )->const char*{
        switch( who ){
          case AGI: return " AGI: ";
          case SYS: return " SYS: ";
          case YOU: return " YOU: ";
        }
        return " ???: ";
      };

      constexpr unsigned ROW_CAPACITY{ 256                          }; // :max number of multibytr characters
      constexpr unsigned CAPACITY    { Communication::DATA_CAPACITY }; // :max number of bytes

      struct Byte {
        char    val;
        uint8_t ord;
      };

      Byte        input[ CAPACITY ];   // :single row user input
      std::string expectedId;          // :expected feedback ID, zero-ended
      Timer       timer;               // :timer used for feedback management
      KeySeq      keySeq{   };         // :symbol or keyboard key escape sequence
      unsigned    N     { 0 };         // :input length, bytes
      unsigned    L     { 0 };         // :input length, multibyte symbols
      int         col   { 0 };         // :current column = cursor position, refers byte, not a symbol

      auto clearCurrentLine = [&](){
        putchar( ESC ); putchar( '[' ); putchar( '2'         ); putchar( 'K' ); // :erase whole line
        putchar( ESC ); putchar( '[' ); printf ( "%u", N + 6 ); putchar( 'D' );  // :move cursor to first logical column
      };

      auto exposeUserInput = [&](){
        fflush( stdout );
                                                                                                                              /*
        Print whole user input:
                                                                                                                              */
        clearCurrentLine();
        printf( "\r%s%s%s", YELLOW, WHO( YOU ), RESET );
        for( unsigned i = 0; i < N; i++ ) putchar( input[i].val );
                                                                                                                              /*
        Print user input from begin to cursor (as way to set cursor position):
                                                                                                                              */
        printf( "\r%s%s%s", YELLOW, WHO( YOU ), RESET );
        for( int i = 0; i < col; i++ ) putchar( input[i].val );
        fflush( stdout );
      };

      auto clear = [&](){
                                                                                                                              /*
        Clear user input:
                                                                                                                              */
        N   = 0;
        col = 0;
        L   = 0;
        exposeUserInput();
      };

      auto above = [&]( Who who, std::string text ){
        clearCurrentLine();
        switch( who ){
          case AGI: printf( "\r%s%s%s%s\n", GREEN,    WHO( who ), RESET, text.c_str() ); break;
          case SYS: printf( "\r%s%s%s%s\n", xMAGENTA, WHO( who ), text.c_str(), RESET ); break;
          case YOU: assert( false );                                                     break;
        }
        exposeUserInput();
      };

      auto inclByte = [&]( const Byte& byte ){
//      if( N >= ROW_CAPACITY ) return;
        if( N >= CAPACITY ) return;
                                                                                                                              /*
        Cursor after the last symbol - just append one more:
                                                                                                                              */
        if( col == int( N ) ){
          input[ col ] = byte;
          col++, N++;
          return;
        }
                                                                                                                              /*
        Cursor inside the text - shift text starting after cursor:
                                                                                                                              */
//      for( int i = ROW_CAPACITY-1; i >= col; i-- ) input[i] = input[ i-1 ];
        for( int i = CAPACITY-1; i >= col; i-- ) input[i] = input[ i-1 ];
        input[ col ] = byte;
        col++, N++;
      };

      auto incl = [&]( const char* seq ){
        uint8_t i{ 0 };
//      if( L + 2 >= CAPACITY ){
        if( L + 2 >= ROW_CAPACITY ){
          above( SYS, kit( "Max statement length of %u characters has been reached", ROW_CAPACITY ) );
          return;
        }
        for( const char* c = seq; *c; c++ ) inclByte( Byte{ *c, ++i } );
        L++;
        exposeUserInput();
      };

      auto del = [&](){
        if( N == 0 ) return;
                                                                                                                              /*
        Get indices of the first and last bytes [ Co .. Ct ] of the current multibyte symbol:
                                                                                                                              */
        int Ct{ col };
        while( input[Ct+1].ord > input[Ct].ord ) Ct++;  // :move to last byte of the multibyte symbols
        int Co{ col };
        while( input[Co].ord > 1 ){ assert( Co > 0 ); Co--; }
        assert( Ct >= Co );
                                                                                                                              /*
        Bytest shift size:
                                                                                                                              */
        int shift{ Ct - Co + 1 };
                                                                                                                              /*
        Shift symbols left starting Co:
                                                                                                                              */
        for( int i = Co; i < int( CAPACITY ) - shift; i++ ) input[ i ] = input[ i + shift ];
                                                                                                                              /*
        Fill tail by zeros:
                                                                                                                              */
        for( int i = int( CAPACITY ) - shift; i < int( CAPACITY ); i++ ) input[i] = Byte{ '\0', 0 };
        col = Co;    // :first next byte become current
        N  -= shift; // :number of bytes
        L--;         // :number of the multibyte symbols
        exposeUserInput();
      };

      auto left = [&](){
        while( col > 0 and input[col].ord > 1 ) col--;  // :move to first byte of multibyte symbol
        if( --col < 0 ) col = 0;
        exposeUserInput();
      };

      auto right = [&](){
                                                                                                                              /*
        Calculate shift size:
                                                                                                                              */
        int shift{ 1 };
        while( input[ col + shift + 1 ].ord > input[ col + shift ].ord ) shift++;
                                                                                                                              /*
        Move if possible:
                                                                                                                              */
        col += shift;
        if( col + 1 >= int( CAPACITY ) ) col = int( CAPACITY ) - 1;
                                                                                                                              /*
        Place space as single byte symbol if referred byte is empty:
                                                                                                                              */
        if( input[ col ].ord == 0 ){
          input[ col ] = Byte{ ' ', 1 };
          N = col + 1;   // :number of bytes inreased
          L++;           // :number of symbols increased
        }
        exposeUserInput();
      };

      auto backspace = [&](){
        if( col == 0 ) return;
        left();
        del();
      };

      auto home  = [&](){ col = 0;  exposeUserInput(); };

      auto endl  = [&](){ col = N; exposeUserInput(); };
                                                                                                                              /*
      Data format:

        0123 4      56..       <- bytes
          ID prefix statement  <- meaning
                                                                                                                              */
      using Message = std::tuple< std::string, char, std::string >;

      auto send = [&]( Prefix prefix, const Byte* text, unsigned length )->std::string{
        constexpr uint32_t  UINT24MASK{ 0xFFFFFF                                };
        Encoded< uint32_t > encoded   { uint32_t( randomNumber() ) & UINT24MASK };
        char                data      [ Communication::DATA_CAPACITY            ];
        const char*         ID        { encoded.c_str()                         };
        const int           idLength  { int( strlen( ID ) )                     };
        memset( data, 0, Communication::DATA_CAPACITY );
        for( unsigned i = 0; i < 4; i++ ) data[i] = '0';
        const int indent = 4 - idLength;
        for( int i = 0; i < idLength; i++ ) data[ i + indent ] = ID[i];
        data[4] = prefix;
        unsigned i{ 5 };
        unsigned L{ 0 };
        for( const Byte* s = text; s->val and i < Communication::DATA_CAPACITY and L < length; s++ ) data[i++] = s->val, L++;
        channel.push( data );
        timer.start();
        return ID;
      };

      auto receive = [&]()->Message{
        const std::string data = channel.pull(); assert( not data.empty() );
        if( debug ) above( SYS, std::string( "First symbol code: " ) + std::to_string( int( data[ 5 ] ) ) );
        unsigned head{ 5 };
        return std::make_tuple( data.substr( 0, 4 ), data.at( 4 ), data.substr( head ) );
      };
                                                                                                                              /*
      __________________________________________________________________________________________________________________________

      State automaton states and eventd defined below.
      Message format : `XXXX message..` where `XXXX` is a message ID or spaces
                                                                                                                              */
      enum State{
        TERM = 0, // :terminated
        EDIT,     // :edit user input; send for parsing or send request for past input/system info/termination
        PAST,     // :get past statement until received
        SCAN,     // :scan (parse) statement
        FACT,     // :get facts about statement and request for athorization to execute
        AUTH,     // :authorize execution
        EXEC      // :execute and got results
      };

      auto LEX = [&]( State state )->const char*{
        switch( state ){
          case TERM: return "TERM";
          case EDIT: return "EDIT";
          case PAST: return "PAST";
          case SCAN: return "SCAN";
          case FACT: return "FACT";
          case AUTH: return "AUTH";
          case EXEC: return "EXEC";
          default  : break;
        }
        return "????";
      };

      auto exposeWait = [&]( State state ){
        constexpr const double dt{ 200 }; // :update interval, msec
        constexpr const char*  xx[4]{ "##--", "-##-", "--##", "#--#" };
        static Timer    timer;
        static unsigned n{ 0 };
        if( timer.elapsed() < dt ) return;
        timer.start();
        if( ++n >= 4 ) n = 0;
        fflush( stdout );
        clearCurrentLine();
        const auto color = state == AUTH ? YELLOW : GREEN;
        printf( "\r%s %s %s", color, xx[n], RESET );
        for( unsigned i = 0; i < N; i++ ) putchar( input[i].val );
        fflush( stdout );
      };

      auto edit = [&]()->State {
        using namespace Symbol;
        exposeUserInput();
        for(;;){
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          if( not keyboard.empty() ){
            keySeq = keyboard.pull();
            const unsigned len = strlen( keySeq.data() );
            if( len == 1 ){
              char& symbol{ keySeq[0] };
              if( symbol == ESC       ) /* ignore */  continue;
              if( symbol == BACKSPACE ){ backspace(); continue; }
              if( symbol == '\n' ){
                expectedId = send( ORIGINAL, input, N );
                                                                                                                              /*
                Make decision about next state:
                                                                                                                              */
                if( N == 2 and input[0].val == '#' ){ // There is not a Gel statement, continue editing:
                  clear();
                  return EDIT;
                } else { // There is a Gel statement, switch to waiting AGI response:
                  clear();
                  exposeUserInput();
                  return SCAN;
                }
              }
              incl( keySeq.data() );
              exposeUserInput();
              continue;
            }
            if( F8    == keySeq ){ debug = not debug;  continue; }
            if( HOME  == keySeq ){ home (           ); continue; }
            if( ENDL  == keySeq ){ endl (           ); continue; }
            if( DEL   == keySeq ){ del  (           ); continue; }
            if( LEFT  == keySeq ){ left (           ); continue; }
            if( RIGHT == keySeq ){ right(           ); continue; }
            if( F5    == keySeq ){ incl ( RING      ); continue; }
            if( F4    == keySeq ){ incl ( EXCESS    ); continue; }
            if( F3    == keySeq ){ incl ( EMPTY_SET ); continue; }
            if( F2    == keySeq ){ incl ( NOT       ); continue; }
            if( F1    == keySeq ){ incl ( DOWNARROW ); continue; }
            if( len == 3 and keySeq[0] == ESC and keySeq[1] == 91 ){
              const Byte UP  [2]{ Byte{ '#', 1 }, Byte{ 'U', 2 } };
              const Byte DOWN[2]{ Byte{ '#', 1 }, Byte{ 'D', 2 } };
              if( int( keySeq[2] ) == 65 ){ send( PRIOR, UP,   2 ); return PAST; }
              if( int( keySeq[2] ) == 66 ){ send( PRIOR, DOWN, 2 ); return PAST; }
            }
            std::stringstream msg;
            msg << "Ignored key with code";
            for( unsigned i = 0; i < L; i++ ) msg << ' ' << int( keySeq[i] );
            above( SYS, msg.str() );
          }
          if( not channel.empty() ){
            auto[ id, prefix, info ] = receive();
            above( AGI, info );
          }
        }//forever
      };//edit

      auto past = [&]()->State{
        timer.start();
        while( timer.elapsed() < 5000 ){
          exposeWait( PAST );
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          while( not keyboard.empty() ) keySeq = keyboard.pull(); // :ignore input
          if( channel.empty() ) continue;
          auto[ id, prefix, text ] = receive();
          if( prefix == PRIOR ){
            L = N = text.length(); // : assuming all symboks are single byte ones
            col = 0;
            memset( input, 0, CAPACITY*sizeof( Byte ) );
            for( unsigned i = 0; i <= N; i++ ) input[i] = Byte{ text[i], 1 };
                                                                                                                              /*
            Search for multibyte symbols and correct value of `L`  :
                                                                                                                              */
            using namespace Symbol;
            constexpr const char* SYMBOLS[5]{ NOT, EMPTY_SET, EXCESS, DOWNARROW, RING };
            for( const auto symbol: SYMBOLS ){
              const auto head = text.find( symbol );
              if( head == std::string::npos ) continue;
              const unsigned n = strlen( symbol );
              above( SYS, kit( "Symbol %s%s%s at %u", RED, symbol, RESET, head ) ); // DEBUG
              for( unsigned i = 0; i < n; i++ ) input[ head + i ].ord = i + 1;
              L -= ( n - 1 );
            }
            clearCurrentLine();
            exposeUserInput();
            return EDIT;
          } else {
            above( AGI, text );
          }
        }
        above( SYS, "EDIT Timeout" );
        exposeUserInput();
        return EDIT;
      };

      auto scan = [&]()->State{
        timer.start();
        while( timer.elapsed() < 2000 ){
          exposeWait( SCAN );
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          while( not keyboard.empty() ) keySeq = keyboard.pull(); // :ignore input
          if( channel.empty() ) continue;
          auto[ id, prefix, text ] = receive();
          above( AGI, text );
          if( prefix == QUIT ) return TERM;
          if( prefix == CORRECT or prefix == ERROR ){
            if( id != expectedId ) above( SYS, std::string( "Invalid ID " ) +  id ); // NB
            N = 0;
            exposeUserInput();
            if( prefix == CORRECT ) return FACT; // :correct   parsed/decorated statement received
            if( prefix == ERROR   ) return EDIT; // :incorrect parsed/decorated statement received
          }
        }
        above( SYS, "SCAN Timeout" );
        exposeUserInput();
        return EDIT;
      };

      auto fact = [&]()->State {
        timer.start();
        while( timer.elapsed() < 2000 ){
          exposeWait( FACT );
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          while( not keyboard.empty() ) keySeq = keyboard.pull(); // :ignore input
          if( channel.empty() ) continue;
          auto[ id, prefix, text ] = receive();
          above( AGI, text );
          exposeUserInput();                                // NB ???
          if( prefix == FACTS ) return AUTH;
        }
        above( SYS, "FACT Timeout" );
        exposeUserInput();
        return EDIT;
      };

      auto auth = [&]()->State {
        timer.start();
        while( timer.elapsed( Timer::SEC ) < 16 ){
          exposeWait( AUTH );
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          while( not keyboard.empty() ){
            keySeq = keyboard.pull();
            const unsigned L = strlen( keySeq.data() );
            if( L == 1 ){
              char symbol = toupper( keySeq[0] );
              constexpr Byte Y{ 'Y', 1 };
              constexpr Byte N{ 'N', 1 };
              if( symbol == '\n' or symbol == 'N' ){ send( REPLY, &N, 1 ); return EDIT; }
              if( symbol == 'Y'                   ){ send( REPLY, &Y, 1 ); return EXEC; }
            }
          }
          if( channel.empty() ) continue;
          auto[ id, prefix, text ] = receive();
          above( AGI, text );
        }
        above( SYS, "AUTH Timeout" );
        exposeUserInput();
        return EXEC;
      };

      auto exec = [&]()->State {
        timer.start();
        while( timer.elapsed( Timer::SEC ) < 15 ){
          exposeWait( EXEC );
          std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
          while( not keyboard.empty() ) keySeq = keyboard.pull();
          if( channel.empty() ) continue;
          auto[ id, prefix, text ] = receive();
          if( prefix == END ) return EDIT;
          above( AGI, text );
        }
        above( SYS, "EXEC Timeout" );
        exposeUserInput();
        return EDIT;
      };
                                                                                                                              /*
      MAIN LOOP ________________________________________________________________________________________________________________
                                                                                                                              */
      terminate .store( false );
      terminated.store( false );

      printf( "\n\n %sTo finish terminal close window or type %sCtrl^C%s", YELLOW, RED,           RESET );
      printf(   "\n %sTo get AGI statistics: %s#S < ENTER >%s",            YELLOW, GREEN,         RESET );
      printf(   "\n %sTo get Gel version:    %s#@ < ENTER >%s",            YELLOW, GREEN,         RESET );
      printf(   "\n %sTo stop AGI:           %s#  < ENTER >%s",            YELLOW, GREEN,         RESET );
      printf(   "\n %sTo navigate over previous statements: %sUP DOWN%s",  YELLOW, GREEN,         RESET );
      printf(   "\n %sTo navigate over previous statements: %sUP DOWN%s",  YELLOW, GREEN,         RESET );
      printf( "\n\n   %sF1%s skip             %s ↓ %s",                    GREEN,  YELLOW, GREEN, RESET );
      printf(   "\n   %sF2%s not              %s ¬ %s",                    GREEN,  YELLOW, GREEN, RESET );
      printf(   "\n   %sF3%s nihil            %s ∅ %s",                    GREEN,  YELLOW, GREEN, RESET );
      printf(   "\n   %sF4%s show definition  %s ∹ %s",                    GREEN,  YELLOW, GREEN, RESET );
      printf(   "\n   %sF5%s show ID          %s ∘ %s\n\n",                GREEN,  YELLOW, GREEN, RESET );

      exposeUserInput();
      const auto sys = WHO( SYS );
      for( State state = EDIT; state != TERM; ){
        if(    terminate.load() ){ printf( "\n\n%s%s%s%s%s%s\n", xMAGENTA,sys,RESET,YELLOW, "Terminate",     RESET ); break; }
        if( not keyboard.live() ){ printf( "\n\n%s%s%s%s%s%s\n", xMAGENTA,sys,RESET,YELLOW, "Dead keyboard", RESET ); break; }
        if( not channel .live() ){ printf( "\n\n%s%s%s%s%s%s\n", xMAGENTA,sys,RESET,YELLOW, "Dead channel",  RESET ); break; }
        if( debug ) above( SYS, std::string( "State: " ) + LEX( state ) );
        switch( state ){
          case EDIT: state = edit(); break;
          case PAST: state = past(); break;
          case SCAN: state = scan(); break;
          case FACT: state = fact(); break;
          case AUTH: state = auth(); break;
          case EXEC: state = exec(); break;
          default  :                 break;
        }
      }// for state
                                                                                                                              /*
      Stop keyboard thread:
                                                                                                                              */
      printf( "\n%s%s%s%sTerminate keyboard..%s", xMAGENTA, WHO( SYS ), RESET, YELLOW, RESET ); fflush( stdout );
      keyboard.stop();
      while( keyboard.live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      printf( "\n%s%s%s%sKeyboard finished%s", xMAGENTA, WHO( SYS ), RESET, YELLOW, RESET ); fflush( stdout );

      printf( "\n%s%s%s%sTerminate communication channel..%s", xMAGENTA, WHO( SYS ), RESET, YELLOW, RESET ); fflush( stdout );
      channel.stop();
      while( channel.live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      printf( "\n%s%s%s%sCommunucation channel finished%s", xMAGENTA, WHO( SYS ), RESET, YELLOW, RESET ); fflush( stdout );

      terminated.store( true );
      printf( "\n%s%s%s%sTerminal finished%s\n", xMAGENTA, WHO( SYS ), RESET, YELLOW, RESET ); fflush( stdout );
      assert( not live() );

      terminated.store( true );

    }//run

  };//class Terminal

}//CoreAGI

#endif // TERMINAL_H_INCLUDED
