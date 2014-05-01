#include <stdio.h>
#include "ligature.h"

int main( int argc, char *argv[] ) {
  if ( argc < 2 ) {
    printf( "usage: ligature <dictionary file> <output file>\n" );
    exit( 1 );
  }
  generate_ligatures( argv[1], argv[2] );
}
  
