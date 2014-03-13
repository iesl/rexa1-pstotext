#include <stdio.h>
#include <time.h>
#include "trie.h"

static int _count_internal_nodes( Trie *node,
                                  unsigned char *prefix,
                                  void *callback_data ) {
  if ( !node->is_end_of_string ) {
    ++*( (int*) callback_data );
  }
  return 1;
}

static int _count_nodes( Trie *node,
                         unsigned char *prefix,
                         void *callback_data ) {
  ++*( (int*) callback_data );
  return 1;
}

static int _count_leaf_nodes( Trie *node,
                              unsigned char *prefix,
                              void *callback_data ) {
  if ( !node->children ) {
    ++*( (int*) callback_data );
  }
  return 1;
}

static int _remove_word(  Trie *node,
                          unsigned char *prefix,
                          void *callback_data ) {
  trie_remove( (Trie*) callback_data, // the trie from which we're removing a word
               (unsigned char*) node->data, // the word to be removed
               trie_free_simple_data_callback );
}


int trie_test_contains_all( Trie *trie, char *file ) {
  FILE *words_file = fopen( file, "r" );
  unsigned char *buf = NULL;
  size_t n_read, buf_size = 0;
  int status = 1;
  while ( ( n_read = getline( (char**) &buf, &buf_size, words_file ) ) != -1 ) {
    if ( n_read > 0 && buf[n_read - 1] == '\n' ) --n_read;
    buf[n_read] = 0;
    if ( n_read ) {
      Trie *node = trie_contains( trie, buf );
      if ( !node ) {
        printf( "error: trie should contain %s\n", buf );
        status = 0;
      } else if ( strcmp( node->data, buf ) ) {
        status = 0;
      }
    }
  }
  free( buf );
  fclose( words_file );
  return status;
}

int trie_test_remove_all( Trie *trie, char *file ) {
  FILE *words_file = fopen( file, "r" );
  unsigned char *buf = NULL;
  size_t n_read, buf_size = 0;
  int status = 1;
  while ( ( n_read = getline( (char**) &buf, &buf_size, words_file ) ) != -1 ) {
    if ( n_read > 0 && buf[n_read - 1] == '\n' ) --n_read;
    buf[n_read] = 0;
    if ( n_read ) {
      int remove_result = trie_remove( trie, buf, trie_free_simple_data_callback );
      if ( !remove_result ) {
        printf( "error: could not remove %s\n", buf );
        status = 0;
      } else if ( trie_contains( trie, buf ) ) {
        printf( "error: attempted removal of %s, but trie still contains the word\n", buf );
        status = 0;
      }
    }
  }
  free( buf );
  fclose( words_file );
  return status;
}


int verify_node_data( Trie *node,
                      unsigned char *prefix,
                      void *data ) {
  int okay = !node->is_end_of_string ||
    !strcmp( prefix, (unsigned char*) node->data );
  if ( !okay ) {
    printf( "error: node has key=%s but data is %s\n",
            prefix,
            (unsigned char*) node->data );
    *( (int*) data ) = 0;
  }
  return okay;
}

/*
 * A convenience method to populate a new trie from a file containing
 * a list of words, one per line.  The 'data' field of each new node
 * is set to the word itself (if the node represents a word at all).
 * Use trie_free_simple_data_callback() as the callback argument to
 * trie_free() and trie_remove(), if either of these methods are
 * invoked on the trie returned by this function.
*/
Trie* trie_from_file( char *file ) {
  FILE *words_file = fopen( file, "r" );
  unsigned char *buf = NULL;
  size_t n_read, buf_size = 0;
  Trie *trie = trie_new( "", (void*) strdup( "" ) );
  while ( ( n_read = getline( (unsigned char**) &buf, &buf_size, words_file ) ) != -1 ) {
    if ( n_read > 0 && buf[n_read - 1] == '\n' ) --n_read;
    buf[n_read] = 0;
    if ( n_read ) {
      if ( !trie_add( trie, buf, (unsigned char*) strdup( buf ) ) ) {
        printf( "error: could not add %s\n", buf );
        return NULL;
      }
    }
  }
  free( buf );
  fclose( words_file );
  return trie;
}


int main( int argc, char **argv ) {
  if ( argc < 2 ) {
    return 1;
  }
  clock_t start = clock();
  Trie *trie = trie_from_file( argv[1] );
  clock_t end = clock();

  if ( trie == NULL ) {
    return 1;
  }

  printf( "time=%.3f\n", ( end - start ) / (float) CLOCKS_PER_SEC );
  printf( "trie size=%d\n", trie_size( trie ) );

  printf( "verifying trie contains all words..." );
  if ( trie_test_contains_all( trie, argv[1] ) ) {
    printf( "success!\n" );
  } else {
    printf( "NO!!!\n" );
  }

  printf( "verifying node data is correct..." );
  int result = 1;
  trie_iterate_dfs( trie, verify_node_data, &result );
  if ( result ) {
    printf( "success!\n" );
  } else {
    printf( "NO!!!\n" );
  }

  if ( argc > 2 ) {
    trie_display( trie );
  }

  int nodes = 0;
  trie_iterate_dfs( trie, _count_nodes, &nodes );
  printf( "nodes=%d\n", nodes );

  int leaf_nodes = 0;
  trie_iterate_dfs( trie, _count_leaf_nodes, &leaf_nodes );
  printf( "leaf nodes=%d\n", leaf_nodes );

  int internal_nodes = 0;
  trie_iterate_dfs( trie, _count_internal_nodes, &internal_nodes );
  printf( "internal nodes=%d\n", internal_nodes );

  trie_free( trie, trie_free_simple_data_callback );

  /* now test the trie_remove() function; we test separately because
     it doesn't actually shrink the tree struture, and we don't want
     to confuse ourselves, above. */
  printf( "verifying node removal..." );
  trie = trie_from_file( argv[1] );
  trie_test_remove_all( trie, argv[1] );
  int stripped_trie_size = trie_size( trie );
  if ( stripped_trie_size > 0 ) {
    result = 0;
    printf( "error: attempt to remove all words from trie was not successful!!!\n" );
  } else { 
    printf( "success!\n" );
  }
  trie_free( trie, trie_free_simple_data_callback ); 
  
  return !result;
}
