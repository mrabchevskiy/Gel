                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Gnosis`Ancillary - base class for classes that interacts with Gnosis

 2021.05.26
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef ANCILLARY_H_INCLUDED
#define ANCILLARY_H_INCLUDED

#include "gnosis.h"
#include "logger.h"

namespace CoreAGI {

  class Ancillary {
  protected:

    const char* TITLE;
    Logger::Log log;
    Gnosis&     gnosis;

  public:

    Ancillary( const char* title, Logger& logger, Gnosis& gnosis ):
      TITLE { title               },
      log   { logger.log( TITLE ) },
      gnosis{ gnosis              }
    {
      log.vital( kit( "Gnisis`Ancillary `%s` associated with Gnosis `%s`", title, gnosis.title() ) );
    }

    virtual ~Ancillary(){
      log.vital( "Destructed" );
      log.flush();
    }

  };//class Ancillary

}//nanespace CoreAGI

#endif // ANCILLARY_H_INCLUDED
