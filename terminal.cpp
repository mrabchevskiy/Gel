#include <cstdio>
#include <cstring>

#include "color.h"
#include "terminal.h"

int main( int argc, char *argv[] ){

  using namespace CoreAGI;

  printf( "\n %sGNOSIS ENGINNERING :: HMI :: vers %s2021.06%s\n", YELLOW, CYAN, RESET );

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
  printf( "\n%s%s%sport %i",  YELLOW, " YOU: ", RESET, thisPort           );
  printf( "\n%s%s%s%s:%i",    GREEN,  " AGI: ", RESET, peerAddr, peerPort );

  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  fflush( stdout );

  {
    Terminal terminal( thisPort, peerAddr, peerPort );
    // terminal.launch();
    for(;;){
      std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
      if( not terminal.live() ) break;
    }
    // printf( "\n\n Terminated\n" );
    std::string error = terminal.error();
    if( not error.empty() ){
      printf( "\n\n Terminal error: `%s`\n", error.c_str() );
      exit( EXIT_FAILURE );
    }
  }

  exit( EXIT_SUCCESS );

}//main
