                                                                                                                              /*
  Configuration parameters for whole CoreAGI project.
  Parameters includes constexpr constants, type aliases and
  some related things tightly coupled logically with type
  aliases.

  Configuration should not refer (directly or indirectly) logger
  and headers/classes except ones ( like `Set` in set.h ) that
  defines unification interface (wrappers) for different underlying
  containers.

  Each configurable class has own configuration namespace enclosed into
  `CoreAGI::Config` namespace. Configuration parameters that related to
  whole project located in `CoreAGI` namespace.

  ____________________

  2020.04.30  Initial version

  2020.05.21  Config::terminal added

  2021.02.13  Key definition moved from `data.h`

  2021/02.20  Definitions of Identity, Key etc moved to `def.h`

________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <atomic>
#include <string>

#include "def.h"
#include "flat_hash.h"
#include "set.h"
#include "flat.h"                                                                                              // [+] 2020.11.14

namespace CoreAGI {

  namespace Config {

    namespace logger {
      constexpr unsigned    LOG_RECORD_CAPACITY    {  256 }; // :max length of log record
      constexpr unsigned    LOGGER_QUEUE_CAPACITY  { 4096 };
      constexpr unsigned    CHANNEL_QUEUE_CAPACITY {  512 };
      constexpr unsigned    CHANNEL_NAME_CAPACITY  {   16 }; // :including End-Of-Line symbol
      constexpr unsigned    CHANNEL_CAPACITY       {   48 }; // :max number of channels
      constexpr unsigned    FORMAT_CAPACITY        {   64 }; // :max length of record format
      constexpr unsigned    PATH_CAPACITY          {  256 }; // :max length of log file path
      constexpr unsigned    NO_JOB_PAUSE           {   50 }; // :pause at `no request` situation, millisec     // [+] 2021.06.08

      constexpr const char* DEFAULT_FORMAT { " %8.1f [ %-10s ] %s" }; // :time, channel name, message

    }

    namespace ui {
      namespace text {
        constexpr unsigned CAPACITY { 4096 };  // :max text size (symbols)
      }
      namespace sfml {
        constexpr const char* NAME                { "UI"                       };
        constexpr const char* TITLE               {  "CoreAGI"                 };
        constexpr const char* BUTTON_FONT         {  "Ubuntu-B.ttf"            };
        constexpr const char* TEXT_FONT           {  "Courier_10_Pitch_BT.ttf" }; //  Courier 10 Pitch Regular.otf
        constexpr unsigned    CAPACITY_ACTION     {   32 };
        constexpr unsigned    CAPACITY_IN         {  128 };
        constexpr unsigned    CAPACITY_OUT        { 1024 };
        constexpr unsigned    WINDOW_WIDTH        { 1200 };
        constexpr unsigned    WINDOW_HEIGHT       {  800 };
        constexpr unsigned    QUERY_MAX_COL       {   58 };
        constexpr unsigned    QUERY_MAX_ROW       {   16 };
      }
    }

    namespace postfix {
      constexpr unsigned STACK_CAPACITY { 4096 };
    }

    namespace data {

      constexpr unsigned CACHE_RESERVATION_FACTOR { 3 }; //

      namespace fast {
        constexpr size_t      SPACE         { 256*1024 }; // :memory pool size, bytes
        constexpr size_t      CAPACITY      {    32803 }; // :max number of stored items      (prime number preferred)
        constexpr unsigned    TYPE_CAPACITY {       67 }; // :max number of stored data types (prime number preferred)
        constexpr unsigned    PATH_CAPACITY {      256 }; // :max length of path to data file
      }
    }

    namespace cache {
      constexpr unsigned THREAD_LIMIT           {   32   }; // :max number of threads accessed cache
      constexpr unsigned PATH_CAPACITY          {   64   }; // :max length of path to dataset file
      constexpr unsigned DATASET_CAPACITY       {  256   }; // :max number of cached datasets
      constexpr double   HEARTBEAT_INTERVAL     { 1000.0 }; // :millisec
    }

    namespace gnosis {

      std::atomic< bool > spurt{ false };                      // :normal/spurt mode flag                         [+] 2021.06.08

      constexpr unsigned ARENA_CAPACITY       {  256*1024   }; // :Bites

      constexpr unsigned NUMBER_OF_THREADS    {         3   }; // :in the analogic                                [+] 2021.01.02
      constexpr unsigned NO_JOB_PAUSE         {        50   }; // :pause at `no request` situation, millisec   // [+] 2021.06.08

      constexpr unsigned NUMBER_OF_SEGMENTS   {         8   }; // :number of graph`s segments                  // [+] 2020.07.11
      constexpr unsigned CAPACITY_OF_SEGMENT  {  128*1024   };                                  // [+] 2020.07.11 [-] 2020.11.11

      constexpr unsigned CAPACITY_OF_QUERY    {        16   }; // :max number of syndroms in the query         // [+] 2020.12.03
      constexpr unsigned CAPACITY_OF_SELECTION{      1024   }; // :max number of selected entities             // [+] 2020.12.03
      constexpr unsigned CAPACITY_OF_SETS     {       256   }; // :maximal expected number of sets of entitis
      constexpr unsigned CAPACITY_OF_RECORD   {      2048   }; // :maximal length of entity record             // [+] 2020.07.24
      constexpr unsigned CAPACITY_OF_SYNDROME {       127   }; // :maximal expected number of entity signs
      constexpr unsigned MAX_LOAD_FACTOR      {        75   }; // :max load factor for sets/maps
//    constexpr unsigned CAPACITY_OF_PATH     {       256   }; // :maximal expected number of entity signs     // [+] 2020.07.24

      constexpr unsigned CAPACITY_OF_ANALOGY  {        16   }; // :max size of graph pattern                   // [+] 2021.06.14

      constexpr const char* SYNDROMES         { "syndromes" }; // :syndromes file                              // [+] 2020.07.24
      constexpr const char* SEQUENCES         { "sequences" }; // :sequences file                              // [+] 2020.07.24

    }

    namespace processor {
      constexpr const char* NAME          { "Proc"  };
      constexpr unsigned    STACK_CAPACITY{ 32*1024 };
      constexpr unsigned    NESTING_LIMIT {     256 };
    }

    namespace tri{
       constexpr unsigned CAPACITY   { 1024 };
       constexpr unsigned NEXT_LIMIT {  256 };
    }

    namespace quant{
      constexpr unsigned STR_CAPACITY { 64*1024 }; // :capacity of string storage
    }

    namespace glossary {
      constexpr const char* GLOSSARY             { "glossary" }; // :glossary file                             // [+] 2020.07.24
      constexpr unsigned    CAPACITY_OF_LEX      {       1024 }; // :max length of entity name                 // [+] 2020.07.24
    }

    namespace sequel {
      constexpr unsigned CAPACITY  { 16*1024 }; // :sequence capacity
      constexpr unsigned CARD_LIMIT{ 1 << 16 }; // :sequence capacity
    }

    namespace contour {

      constexpr unsigned  SPOT_CAPACITY                   { 256   };
      constexpr unsigned  IMAGE_CELL_SIZE                 { 128   };
      constexpr unsigned  ZOOM                            {   5   };
      constexpr unsigned  SLIDING_WINDOW                  {   3   };
      constexpr unsigned  MIN_ACCEPTABLE_NUMBER_OF_PIXELS {   2   };
      constexpr double    MAX_BRIGHTNESS                  { 250.0 };
      constexpr double    MIDDLE_BRIGHTNESS               { 125.0 };

    }

  };//namespace Config

  using HiddenSet = ska::flat_hash_set< Identity, IdentityHash >; // :underlying entity set container
  using BigSet    = Set< Identity, HiddenSet, Config::gnosis::CAPACITY_OF_SETS, Config::gnosis::MAX_LOAD_FACTOR >;
  using Signs     = Flat::Set< Config::gnosis::CAPACITY_OF_SYNDROME >;

}// namespace

#endif // CONFIG_H_INCLUDED
