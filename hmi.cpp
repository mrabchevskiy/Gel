                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________




________________________________________________________________________________________________________________________________
                                                                                                                              */
#include <cstdio>
#include <cstdlib>

#include <string>
#include <vector>
#include <wchar.h>

#include "def.h"
#include "hmi.h"
#include "gnosis.h"
#include "glossary.h"
#include "logger.h"
#include "table.h"                                                                                        // [+] 2020.04.15

int main( int argc, char *argv[] ){

  using namespace CoreAGI;

  Logger logger{};
  logger.update( logging::Note::Type::BRIEF, "hmi.brief" );
  Logger::Log log{ logger.log( "Main" ) };
                                                                                                                              /*
  Check parameters:
                                                                                                                              */
  auto usage = [&](){
    constexpr const char* USAGE{ "\n\n usage: channel <this-port> <peer-ip-addr:peer-port>\n" };
    printf( USAGE );
    exit( EXIT_SUCCESS );
  };

  if( argc < 3 ) usage();
  char* colon = strchr( argv[2], ':' ); if( not colon ) usage();
  *colon = '\0';
  int   thisPort = atoi( argv[1] );
  char* peerAddr{       argv[2]   };
  int   peerPort{ atoi( colon+1 ) };

  log.vital( kit( "%sthis:%s port %u", YELLOW, RESET, thisPort           ) );
  log.vital( kit( "%speer:%s %s:%u",   YELLOW, RESET, peerAddr, peerPort ) );
                                                                                                                              /*
  Construct Gnosis instance:
                                                                                                                              */
  log.vital( "Contruct gnosis.." ); log.vital();
  Gnosis gnosis{ "Gnosis", logger  };                                                                // [m] 2020.11.11
  log.flush();
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                                                                                                                              /*
  Construct Data storage instance:
                                                                                                                              */
  log.vital( "Construct data storage.." ); log.vital(); log.flush();
  Data::Mini data{ "Data", logger, gnosis };
  log.flush();
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                                                                                                                              /*
  Construct Glossary instance:
                                                                                                                              */
  log.vital( "Construct glossary.." ); log.vital(); log.flush();
  Glossary glossary{ "EN", logger, gnosis, data };
  log.flush();
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                                                                                                                              /*
  Construct HMI:
                                                                                                                              */
  log.vital();
  log.vital( "Construct HMI.." );
  log.vital();
  log.flush();
  Hmi hmi{ logger, gnosis, glossary, data, thisPort, peerAddr, peerPort };
  log.sure( hmi.live(), "Channel failed to start" );
  log.vital();
  log.vital( "Special symbols:" );
  log.vital();
  log.vital( kit( "  EMPTY_SET: %s", Symbol::EMPTY_SET ) );
  log.vital( kit( "  NOT:       %s", Symbol::NOT       ) );
  log.vital( kit( "  RING:      %s", Symbol::RING      ) );
  log.vital( kit( "  EXCESS:    %s", Symbol::EXCESS    ) );
  log.vital( kit( "  DOWNARROW: %s", Symbol::DOWNARROW ) );
  log.vital();
  log.flush();

  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  {
    log.vital();
    log.vital( "Create data structure used in `Chapter 007`.." );
                                                                                                                              /*
    Named entities:
                                                                                                                              */
    auto HYDROCARBON = glossary.entity( "hydrocarbon"                                                    );
    auto COMPUTER    = glossary.entity( "computer"                                                       );
    auto ELECTRICITY = glossary.entity( "electricity"                                                    );
    auto GASOLINE    = glossary.entity( "gasoline"                                                       );
    auto GROUND      = glossary.entity( "ground"                                                         );
    auto METHANE     = glossary.entity( "methane",    { HYDROCARBON }                                    );
    auto MOTORCYCLE  = glossary.entity( "motorcycle"                                                     );
    auto OXYGEN      = glossary.entity( "oxygen"                                                         );
    auto PRODUCES    = glossary.entity( "produces",   { gnosis.VERB }                                    );
    auto REUSABLE    = glossary.entity( "reusable"                                                       );
    auto SPACE       = glossary.entity( "space"                                                          );
    auto STARSHIP    = glossary.entity( "Starship"                                                       );
    auto USES        = glossary.entity( "uses",       { gnosis.VERB, gnosis.IMMUTABLE, gnosis.IMMORTAL } );
    auto VEHICLE     = glossary.entity( "vehicle"                                                        );
    auto WIND        = glossary.entity( "wind"                                                           );
    auto WINDMILL    = glossary.entity( "windmill"                                                       );
    auto QUOTED      = glossary.entity( "quoted entity"                                                  );

    auto SPACE_VEHICLE        = gnosis.entity( { SPACE,    VEHICLE     } );
    auto GROUND_VEHICLE       = gnosis.entity( { GROUND,   VEHICLE     } );
    auto USES_WIND            = gnosis.entity( { USES,     WIND        } );
    auto USES_OXYGEN          = gnosis.entity( { USES,     OXYGEN      } );
    auto USES_GASOLINE        = gnosis.entity( { USES,     GASOLINE    } );
    auto USES_METHANE         = gnosis.entity( { USES,     METHANE     } );
    auto PRODUCES_ELECTRICITY = gnosis.entity( { PRODUCES, ELECTRICITY } );
    auto USES_ELECTRICITY     = gnosis.entity( { USES,     ELECTRICITY } );
                                                                                                                              /*
    Connectons:
                                                                                                                              */
    STARSHIP   += USES_METHANE;
    STARSHIP   += USES_OXYGEN;
    STARSHIP   += SPACE_VEHICLE;
    STARSHIP   += REUSABLE;
    COMPUTER   += USES_ELECTRICITY;
    MOTORCYCLE += USES_GASOLINE;
    WINDMILL   += USES_WIND;
    WINDMILL   += PRODUCES_ELECTRICITY;
                                                                                                                              /*
    Attributes and sequence:
                                                                                                                              */
    auto CHEMICAL_FORMULA = glossary.entity( "chemical formula", { gnosis.ATTRIBUTE, gnosis.STRING } );
    METHANE += CHEMICAL_FORMULA;

    auto MOLECULAR_MASS = glossary.entity( "molecular mass", { gnosis.ATTRIBUTE, gnosis.RATIONAL } );
    METHANE += MOLECULAR_MASS;

    ELECTRICITY.seq( { COMPUTER, WINDMILL } );

    using Cargo = Data::Cargo;

    METHANE.seq( { HYDROCARBON, STARSHIP } );

    data.put( METHANE, CHEMICAL_FORMULA, Cargo{ "CH4"   } );
    data.put( METHANE, MOLECULAR_MASS,   Cargo{ 16.043d } );

  }

  log.vital();
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  log.vital( "Switch to process terminal data.." );
  log.vital(); log.flush();
                                                                                                                              /*
  Process user input:
                                                                                                                              */
  hmi.run();

  log.vital(); log.flush();
  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  log.vital(); log.flush();
  hmi.stop();
  while( hmi.live() ) std::this_thread::yield();
  log.vital();
  log.vital( "Finish" ); log.flush();

  return 0;
}
