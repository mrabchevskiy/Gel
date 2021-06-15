                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Communication channel for asynchronous two-way communication using UDP

 2021.05.17
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef CHANNEL_H_INCLUDED
#define CHANNEL_H_INCLUDED

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include <functional>
#include <unistd.h>
#include <poll.h>
#include <tuple>

#include "active.h"
#include "timer.h"
#include "queue.h"

namespace CoreAGI {

  namespace Communication {

    constexpr unsigned    QUEUE_CAPACITY   {   32          };
    constexpr unsigned    DATA_CAPACITY    { 1536          }; // :1.5KB
    constexpr unsigned    SEND_INTERVAL    {  250          }; // :millisec
    constexpr unsigned    ACK_TIMEOUT      { 2000          }; // :millisec
    constexpr unsigned    RECV_TIMEOUT     {  100          }; // :millisec
    constexpr const char* ACK              { "OK"          }; // :acknowledgement feedback
    constexpr unsigned    ACK_LENGTH       { strlen( ACK ) };

    class EndPoint: public Active, public Queue< std::string, QUEUE_CAPACITY > {
    protected:
      int  port; // :UDP port
      int  SOCK; // :socket file descriptor
      char data[ DATA_CAPACITY ];
    public:
      EndPoint( int PORT ): Active{}, Queue{}, port{ PORT }, SOCK{ socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) }, data{}{
        if( SOCK == -1 ) expl = "Can't create socket";
      }
     ~EndPoint(){  }
    };

    class Receiver: public EndPoint {
    public:
      Receiver( int PORT ): EndPoint{ PORT }{ start(); }
    private:
      virtual void run() override {
                                                                                                                              /*
        Prepare data structure and bind socket to port:
                                                                                                                              */
        sockaddr_in self;
        memset( (char*)&self, 0, sizeof( self ) );
        self.sin_family      = AF_INET;
        self.sin_port        = htons( port );
        self.sin_addr.s_addr = htonl( INADDR_ANY );
        if( bind( SOCK, (sockaddr*)&self, sizeof( self ) ) == -1 ){ expl = "Can`t bind UDP socket to port"; return; }
                                                                                                                              /*
        Main loop:
                                                                                                                              */
        sockaddr_in peerAddr{};
        unsigned    len{ sizeof( peerAddr ) };
        pollfd polled[1];
        polled[0].fd     = SOCK;
        polled[0].events = POLLIN;
        terminated = false;
        while( not terminate ){
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
          if( size() >= QUEUE_CAPACITY ){
            continue; // :full queue
          }
                                                                                                                              /*
          Try to receive some data:
                                                                                                                              */
          int num_events = poll( polled, 1, RECV_TIMEOUT );
          if( num_events == 0 ) continue;
          int pollin_happened = polled[0].revents & POLLIN;
          if( pollin_happened ){
            int recv_len{ 0 };
            if( ( recv_len = recvfrom( SOCK, data, DATA_CAPACITY, 0, (struct sockaddr*) &peerAddr, &len ) ) == -1 ){
              expl = "Recv failed";
              continue;
            }
                                                                                                                              /*
            Send back delivery confirmation:
                                                                                                                              */
            if( sendto( SOCK, ACK, ACK_LENGTH, 0, (sockaddr*) &peerAddr, len ) == -1 ){ expl = "Ack failed"; continue; }
                                                                                                                              /*
            Queue received data:
                                                                                                                              */
            assert( push( std::string( data, recv_len ) ) );
          }//if
        }//while not terminate
        close( SOCK );
        terminated = true;
      }//run

    };//Receiver

    class Transmitter: public EndPoint {
      const char* peerIpAddress;
    public:
      Transmitter( const char* PEER_ADDR, int PEER_PORT ): EndPoint{ PEER_PORT }, peerIpAddress{ PEER_ADDR }{
        assert( peerIpAddress ); start();
      }
      Transmitter            ( const Transmitter& ) = delete;
      Transmitter& operator= ( const Transmitter& ) = delete;
    private:
      virtual void run() override {
        sockaddr_in peerAddr;
        memset( (char*)&peerAddr, 0, sizeof( peerAddr ) );
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port   = htons( port );
        if( inet_aton( peerIpAddress, &peerAddr.sin_addr ) == 0 ){ expl = "Invalid peer IP address"; return; }
        pollfd polled[1];
        polled[0].fd     = SOCK;
        polled[0].events = POLLIN;
        terminated = false;
        while( not terminate ){
          std::this_thread::sleep_for( std::chrono::milliseconds( SEND_INTERVAL ) );
          if( empty() ) continue;
                                                                                                                              /*
          Copy data from the oldest queue item into `data` array and trasmit:
                                                                                                                              */
          const std::string* rec{ firstRef() }; assert( rec );
          unsigned len = rec->length(); if( len > DATA_CAPACITY ) len = DATA_CAPACITY;
          memcpy( data, rec->data(), len );
          if( sendto( SOCK, data, len, 0, (sockaddr*)&peerAddr, sizeof( peerAddr ) ) == -1 ) continue;
          memset( data, '\0', len ); // :clear `data`
          int num_events = poll( polled, 1, ACK_TIMEOUT );\
          if( num_events == 0 ) continue;
          int pollin_happened = polled[0].revents & POLLIN;
          if( pollin_happened ){
                                                                                                                              /*
            Receive ACK in `data` array (blocking call):
                                                                                                                              */
            unsigned peerAddrlen;
            int receivedLength = recvfrom( SOCK, data, DATA_CAPACITY, 0, (sockaddr*)&peerAddr, &peerAddrlen );
            assert( receivedLength  != -1 );
            if( strcmp( data, ACK ) !=  0 ) continue; // :invalid ACK
                                                                                                                              /*
            OK, remove sent data from queue:
                                                                                                                              */
            pull();
          }
        }//while
        close( SOCK );
        terminated = true;
      }//run
    };//Transmitter


    class Channel {
    protected:
      Receiver    receiver;
      Transmitter transmitter;
      std::string expl;
    public:
      Channel            ( const Channel& ) = delete;
      Channel& operator= ( const Channel& ) = delete;
      Channel( int port, const char* peerAddr, int peerPort ): receiver{ port }, transmitter{ peerAddr, peerPort }, expl{}{
        Timer timer;
        while( timer.elapsed() < 1000 ){
          std::this_thread::yield();
          if( live() ) return;
        }
        expl = receiver.error() + ';' + transmitter.error();
      }
      std::tuple< bool, int, std::string, bool, int, std::string > state() const {
        return std::make_tuple(
          receiver   .live (),
          receiver   .size (),
          receiver   .error(),
          transmitter.live (),
          transmitter.size (),
          transmitter.error()
        );
      }
      std::string error(                         ) const { return expl;                                   }
      bool        empty(                         ) const { return receiver   .empty();                    }
      bool        done (                         ) const { return transmitter.empty();                    }
      bool        live (                         ) const { return receiver.live() and transmitter.live(); }
      std::string pull (                         )       { return receiver.pull();                        }
      bool        push ( const std::string& data )       { return transmitter.push( data );               }
      void        clear(                         )       { return transmitter.clear();                    }

      void stop(){
        transmitter.stop();
        receiver.stop();
      }

     ~Channel(){
        stop();
        while( transmitter.live() or receiver.live() ) std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      }
    };

  }//namespace Communication

}//namespace CireAGI

#endif // CHANNEL_H_INCLUDED
