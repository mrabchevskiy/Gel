                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Active and ActiveQueue based on Active

 2021.05.17
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef ACTIVE_H_INCLUDED
#define ACTIVE_H_INCLUDED

#include <atomic>
#include <thread>
#include <string>

#include "queue.h"

namespace CoreAGI {
                                                                                                                              /*
  Pure virtual class intended to be used as a base class fow a few actual ones:
                                                                                                                              */
  class Active {
  protected:
    std::thread         thread;
    std::atomic< bool > terminate;
    std::atomic< bool > terminated;
    std::string         expl;        // :error explication message
    virtual void start(){ thread = std::thread( &Active::run, this ); }
    virtual void run() = 0;
  public:
    Active(): thread{}, terminate{ false }, terminated{ true }, expl{} {}
    virtual ~Active(){
      if( thread.joinable() ){ terminate.store( true ); thread.join(); }
    }
    void stop()       { terminate.store( true );      }
    bool live() const { return not terminated.load(); }
    std::string error() const { return expl; }
  };//class ActiveUnit

}

#endif // ACTIVE_H_INCLUDED
