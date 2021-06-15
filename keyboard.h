                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Intercepting keyboard input

 2021.05.17
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef KEYBOARD_H_INCLUDED
#define KEYBOARD_H_INCLUDED

#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <atomic>
#include <thread>

#include "active.h"

namespace CoreAGI{

  constexpr unsigned MAX_ESCAPE_SEQUENCE_LENGTH{ 8 };

  using KeySeq = std::array< char, MAX_ESCAPE_SEQUENCE_LENGTH >;

  class Keyboard: public Active, public Queue< KeySeq, 16 > {

  public:

    Keyboard(): Active{}, Queue{}{ start(); }

   ~Keyboard(){
      stop();
      while( live() ) std::this_thread::yield();
    }

  private:

    virtual void run() override {
                                                                                                                              /*
      Preserve terminal setting then modify:
                                                                                                                              */
      termios originalSettings;
      termios modifiedSettings;
      tcgetattr( fileno( stdin ), &originalSettings );
      modifiedSettings = originalSettings;
      modifiedSettings.c_lflag &= (~ICANON & ~ECHO);
      tcsetattr( fileno( stdin ), TCSANOW, &modifiedSettings );
                                                                                                                              /*
      Set parameters for `stdin` polling:
                                                                                                                              */
      fd_set SET;
      timeval tv;
      tv.tv_sec  =     0;  // :zero seconds
      tv.tv_usec = 10000;  // :10   millisec == 10,000 microsec

      //constexpr unsigned TYPED_CAPACITY{ 32 };
      //char input[ TYPED_CAPACITY ];
      terminated.store( false );
      KeySeq input;
                                                                                                                              /*
      Main loop:
                                                                                                                              */
      while( not terminate.load() ){
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        FD_ZERO( &SET );
        FD_SET ( fileno( stdin ), &SET );
        //memset( input, 0, TYPED_CAPACITY );
        input.fill( '\0' );
        int res = select( fileno( stdin )+1, &SET, NULL, NULL, &tv );
        if( res < 0 ){ expl = "Keyboard input error"; break; }
        if( res > 0 ){
          int length = read( fileno( stdin ), input.data(), MAX_ESCAPE_SEQUENCE_LENGTH );
          if( length ) push( input );     // :NB can fail due to queue overrun
        }
      }//while
                                                                                                                              /*
      Restore `stdin` settings:
                                                                                                                              */
      tcsetattr( fileno( stdin ), TCSANOW, &originalSettings );

      terminated = true;
    }//run

  };//class Keyboard

}//namesace CoreAGI

#endif // KEYBOARD_H_INCLUDED
