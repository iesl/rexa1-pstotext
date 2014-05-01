#include <stdio.h>
#include "ligature.h"
#include "trie.h"

#define N_LIGATURES sizeof( LIGATURES ) / sizeof( int )

#define MIN(a,b) ((a)<=(b)?(a):(b))

static Trie* deligatured_words;

/* pre-declarations */
void _free_ligature_data_callback2( void *data );


static int longest_lig_at_pos( unsigned char *word, int i ) {
  int word_len = strlen( word );

  int done = 0;
  int lig_id;
  int found_lig_id = -1;
  int pos_in_lig = 0;
  int *found_lig_ids = (int*) malloc( N_LIGATURES * sizeof( int ) );
  for ( lig_id = 0; lig_id < N_LIGATURES; ++lig_id ) {
    found_lig_ids[lig_id] = LIGATURE_LENGTHS[lig_id];
  }
  do {
    done = 1;
    for ( lig_id = 0; lig_id < N_LIGATURES; ++lig_id ) {
      if ( pos_in_lig < LIGATURE_LENGTHS[lig_id] &&
           i + pos_in_lig < word_len ) {
        done = 0;
        if ( word[i + pos_in_lig] == LIGATURES[lig_id][pos_in_lig] ) {
          --found_lig_ids[lig_id];
        }
      }
    }
    ++pos_in_lig;
  } while ( !done );

  // we can only return one found ligature per word position, so we'll
  // select the longest one (assumes LIGATURES are listed in order of
  // increasing length)
  //
  // TODO: returning the longest ligature may not always be the right
  // thing to do
  for ( lig_id = N_LIGATURES - 1; lig_id >= 0; --lig_id ) {
    if ( found_lig_ids[lig_id] == 0 ) {
      found_lig_id = lig_id;
      break;
    }
  }
  
  free( found_lig_ids );
  return found_lig_id;
}

/*
 * Determines the positions at which ligatures occur in the specified
 * word.  The return value is the number of ligatures found.  If
 * non-null, 'ligature_positions' is set to a newly allocated int
 * array, where the i'th element contains the position of the i'th
 * ligature (the calling function should free this array).
 */
static int word_ligatures( unsigned char *word, 
                           int **ligature_positions,
                           int **ligature_ids ) {
  int i, l;
  int lig_count = 0; // count of ligatures found in word
  int ligature_index[] = { 0, 0, 0, 0, 0 };
  int tmp_ligature_pos[MAX_WORD_LEN];
  int tmp_ligature_ids[MAX_WORD_LEN];
  for ( i = 0; i < strlen( word ); ++i ) {
    int lig_id = longest_lig_at_pos( word, i );
    if ( lig_id >= 0 ) {
      tmp_ligature_pos[lig_count] = i;
      tmp_ligature_ids[lig_count] = lig_id;
      i += LIGATURE_LENGTHS[lig_id] - 1;
      ++lig_count;
    }
  }

  if ( ligature_positions != NULL && ligature_ids != NULL ) {
    *ligature_positions = (int*) malloc( lig_count * sizeof( int ) );
    *ligature_ids = (int*) malloc( lig_count * sizeof( int ) );
    memcpy( *ligature_positions, tmp_ligature_pos, lig_count * sizeof( int ) );
    memcpy( *ligature_ids, tmp_ligature_ids, lig_count * sizeof( int ) );
  }
  return lig_count;
}

/* "Re-ligatures" the specified deligatured word by finding the
   corresponding "real" word.  Modifies the 'word' argument, which
   must be large enough to hold the "real" word.  Returns the
   difference in size between the real world and the deligatured word
   (the real word will always be longer).  THIS FUNCTION CAN ONLY BE
   CALLED AFTER load_ligatures has been called to initialize our
   global 'deligatured_words' variable.
*/
int religature_word( unsigned char *word, int n ) {
  int chars_added = 0;
  Trie *node = trie_contains( deligatured_words, word );
  if ( node ) {
    int deligatured_word_len = strlen( word );
    unsigned char *real_word = (unsigned char*) node->data;
    strncpy( word, real_word, n );
    word[n-1] = 0; // ensure null-termination
    return strlen( real_word ) - deligatured_word_len;
  }
  return 0;
}    
    

/* int religature_word( unsigned char *word, int n ) { */
/*   int chars_added = 0; */
/*   Trie *node = trie_contains( deligatured_words, word ); */
/*   if ( node ) { */
/*     LigatureData *lig_data = (LigatureData*) node->data; */
/*     int l; */
/*     int word_len = strlen( word ); */
/*     // insert each ligature into word, expanding word as necessary */
/*     for ( l = lig_data->ligature_count - 1; l >= 0; --l ) { */
/*       int lig_id = lig_data->ligature_ids[l]; */
/*       int lig_pos = lig_data->ligature_positions[l]; */
/*       int lig_len = LIGATURE_LENGTHS[lig_id]; */
/*       if ( word_len + lig_len >= n ) { */
/*         fprintf( stderr, "warning: word buffer too small, religaturization incomplete\n" ); */
/*         break; */
/*       } */
/*       int suffix_len = word_len - lig_pos; */
/*       memmove( word + lig_pos + lig_len, */
/*                word + lig_pos, */
/*                suffix_len + 1 ); */
/*       memcpy( word + lig_pos, LIGATURES[lig_id], lig_len ); */
/*       word_len += lig_len; */
/*       chars_added += lig_len; */
/*     } */
/*   } */
/*   return chars_added; */
/* } */


/**
 * Returns a newly-allocated modified version of 'word' (a
 * null-terminated string) by removing and "marking" the positions of
 * ligatures found at the positions specified in ligature_positions[],
 * where each ligature has the ID found in the corresponding element
 * of ligature_ids[]; a ligature's position is "marked" by "#".
 * For example, "affluent" would become "a#luent".
*/
static unsigned char* mark_ligatures( unsigned char *word, 
                                      int ligature_positions[], 
                                      int ligature_ids[],
                                      size_t ligatures ) {
  int last_lig_pos = -1;
  int l;
  // we'll use incr_new_word_len to keep track of new word length, as we expand it
  int incr_new_word_len = strlen( word );
  // new_word_len is the final (largest) size of the new word
  int new_word_len = strlen( word ) + ligatures;
  unsigned char *new_word = (unsigned char*) malloc( new_word_len + 1 );
  strcpy( new_word, word );
  for ( l = ligatures - 1; l >= 0; --l ) {
    // if > 1 ligature starts at same position, only remove one of
    // them (it will be the longest one)
    if ( ligature_positions[l] != last_lig_pos ) {
      int lig_start = ligature_positions[l];
      int lig_end = ligature_positions[l] + LIGATURE_LENGTHS[ligature_ids[l]];
      memmove( new_word + lig_start + 1, 
               new_word + lig_end, 
               ( incr_new_word_len - lig_end ) + 1 );
      // TODO: "#" should not be hardcoded here
      new_word[lig_start] = '#';
      incr_new_word_len++;
      last_lig_pos = ligature_positions[l];
    }
  }
  return new_word;
}

void add_deligatured_word( unsigned char *word, Trie *deligatured_words ) {

  int *ligature_positions;
  int *ligature_ids;
  int ligature_count = word_ligatures( word, &ligature_positions, &ligature_ids );
  // printf( "%s (%d)\n", word, ligature_count ); 
  if ( ligature_count > 0 ) {
    unsigned char *deligatured_word;
    deligatured_word = mark_ligatures( word,
                                       ligature_positions, 
                                       ligature_ids, 
                                       ligature_count );
/*     printf( "%s ==> %s\n", word, deligatured_word ); */

    // warn user when 2 "real" words map to the same deligatured word
    Trie *node = trie_contains( deligatured_words, deligatured_word );
    unsigned char *buf = 0;
    if ( node ) {
      unsigned char* existing_word = ( (LigatureData*) node->data )->word;
      // TODO: after a 2nd "real" source word is added to
      // node->data->word, reoccurring source words will be repeated
      // in our concatenated list; not a big deal, since this entry
      // will have to be edited by the user anyway, but we could do
      // better...
      if ( strcmp( word, existing_word ) ) { // if real words are different...
        // we will remove the existing node and replace it with one
        // whose "real" word is a concatenation of both words; the
        // user will be required to edit the result in the generated
        // file
        fprintf( stderr, 
                 "warning: deligatured word '%s' has multiple source words: %s, %s; EDIT FILE!\n",
                 deligatured_word,
                 word,
                 existing_word );
        buf = (unsigned char*) malloc( 2 + strlen( existing_word ) + 1 +
                                       strlen( word ) + 1 );
        buf[0] = 0;
        strcat( buf, "{" );
        strcat( buf, existing_word );
        strcat( buf, "|" );
        strcat( buf, word );
        strcat( buf, "}" );
        word = buf;
        trie_remove( deligatured_words, deligatured_word, _free_ligature_data_callback2 );
      }
    }
    LigatureData *lig_data = (LigatureData*) malloc( sizeof( LigatureData ) );
    lig_data->word = (unsigned char*) strdup( word );
    lig_data->ligature_positions = ligature_positions;
    lig_data->ligature_ids = ligature_ids;
    lig_data->ligature_count = ligature_count;
    trie_add( deligatured_words, deligatured_word, lig_data );
    free( deligatured_word );
    if ( buf ) {
      free( buf );
    }
  }
}


/* Generates a trie containing a mapping from deligatured words back
   to their "real" worlds.  Returns 1 on success, otherwise 0. */
static Trie* find_ligatures( char* dictionary_file ) {
  FILE *words_file = fopen( dictionary_file, "r" );
  if ( words_file == NULL ) {
    return 0;
  }
  unsigned char *buf = NULL;
  int n_read, buf_size = 0;
  Trie *trie = trie_new( "", NULL );
  while ( ( n_read = getline( &buf, &buf_size, words_file ) ) != -1 ) {
    if ( buf[n_read - 1] == '\n' ) --n_read;
    buf[n_read] = 0;

    add_deligatured_word( buf, trie );

    /* For non-proper names (i.e., normal words) we want to add to
       our trie the deligaturization of the *capitalized* version of
       the word.  Consider, for example, that "fluffy" and "Fluffy"
       have different deligaturizations: "uy" and "Fluy",
       respectively. */
    if ( islower( buf[0] ) ) {
      buf[0] = toupper( buf[0] );
      // now find ligatures again, and add it if appropriate
      add_deligatured_word( buf, trie );
    }
  }
  free( buf );
  fclose( words_file );

  printf( "Trie size=%d\n", trie_size( trie ) );
/*   trie_display( trie ); */
/*   printf( "Trie size=%d\n", trie_size( words ) ); */
/*   trie_display( words ); */
/*   printf( "aunt=%d\n", trie_contains( words, "aunt" ) ); */

/*   char test_buf[100]; */
/*   strcpy( test_buf, "eect" ); */
/*   religature_word( test_buf, sizeof( test_buf ) ); */
/*   strcpy( test_buf, "Eect" ); */
/*   religature_word( test_buf, sizeof( test_buf ) ); */
/*   strcpy( test_buf, "uy" ); // fluffy */
/*   religature_word( test_buf, sizeof( test_buf ) ); */
/*   strcpy( test_buf, "Fluy" ); // Fluffy */
/*   religature_word( test_buf, sizeof( test_buf ) ); */
/*   strcpy( test_buf, "Flanagan" ); */
/*   religature_word( test_buf, sizeof( test_buf ) ); */
/*   strcpy( test_buf, "anagan" ); // no such word! (flanagan) */
/*   religature_word( test_buf, sizeof( test_buf ) ); */

  return trie;
}

/* Initialize our global 'deligatured_words' trie by loading the
   pre-generated ligatures from a file; see generate_ligatures(). */
int load_ligatures( char *ligature_file ) {
  FILE *ligatures_fp = fopen( ligature_file, "r" );
  if ( !ligatures_fp ) {
    return 0;
  }
  deligatured_words = trie_new( 0, (void*) strdup( "" ) );
  unsigned char buf[MAX_WORD_LEN * 2 + 2];
  while ( fgets( buf, MAX_WORD_LEN * 2 + 2, ligatures_fp ) ) {
    unsigned char *deligatured_word = buf;
    char *delimiter = index( buf, '\t' );
    delimiter[0] = 0;
    unsigned char *real_word = (unsigned char*) strdup( delimiter + 1 );
    real_word[strlen( real_word ) - 1] = 0; // chomp newline
    trie_add( deligatured_words, deligatured_word, real_word );
  }
  fclose( ligatures_fp );
  return 1;
}

void free_ligatures() {
  trie_free( deligatured_words, trie_free_simple_data_callback );
  deligatured_words = NULL;
}


int write_ligature_callback( Trie *node,
                             unsigned char *deligatured_word,
                             void *data ) {
  FILE *fp = (FILE*) data;
  if ( node->is_end_of_string ) {
    unsigned char* real_word = ( (LigatureData*) node->data )->word;
    fwrite( deligatured_word, sizeof( unsigned char ), strlen( deligatured_word ), fp );
    fwrite( "\t", sizeof( unsigned char ), strlen( "\n" ), fp );
    fwrite( real_word, sizeof( unsigned char ), strlen( real_word ), fp );
    fwrite( "\n", sizeof( unsigned char ), strlen( "\n" ), fp );
  }
  return 1;
}

void _free_ligature_data_callback2( void *data ) {
  LigatureData *lig_data = (LigatureData*) data;
  if ( lig_data->word ) { free( lig_data->word ); }
  if ( lig_data->ligature_positions ) { free( lig_data->ligature_positions ); }
  if ( lig_data->ligature_ids ) { free( lig_data->ligature_ids ); }
  free( lig_data );
}


/* Generates a file containing pairs of deligatured & "real" words,
   one pair per line.  This file can then be used to efficiently load
   the ligatures into memory via the load_ligatures() function.
   Allows ligature generation to be performed as one-time
   preprocessing step. */
int generate_ligatures( char *dictionary_file,
                        char *ligature_file ) {
  Trie *trie;
  if ( trie = find_ligatures( dictionary_file ) ) {
    FILE *ligatures_fp = fopen( ligature_file, "w" );
    if ( ligatures_fp ) {
      trie_iterate_dfs( trie, 
                        write_ligature_callback,
                        ligatures_fp );
    }
    fclose( ligatures_fp );
  }
  trie_free( trie, _free_ligature_data_callback2 );
}




/*   // test */
/*   printf( "specication=%d\n", trie_contains( deligatured_words, "specication" ) ); */
/*   printf( "eect=%d\n", trie_contains( deligatured_words, "eect" ) ); */
/*   printf( "AOL=%d\n", trie_contains( t, "AOL" ) ); */
/*   printf( "AO=%d\n", trie_contains( t, "AO" ) ); */
/*   printf( "Aaron=%d\n", trie_contains( t, "Aaron" ) ); */
/*   printf( "Joex=%d\n", trie_contains( t, "Joex" ) ); */


/* static void test_word_contains_ligature( char *s ) { */
/*   printf( "%s=%d\n", */
/*           s, */
/*           word_ligatures( s, strlen( s ), NULL, NULL ) ); */
/* } */

/*   test_word_contains_ligature( "effect" ); */
/*   test_word_contains_ligature( "afflict",  ); */
/*   test_word_contains_ligature( "afoflict" ); */
/*   test_word_contains_ligature( "defficient" ); */
/*   test_word_contains_ligature( "Pfafflan" ); */
/*   test_word_contains_ligature( "friend" ); */
/*   test_word_contains_ligature( "focus" ); */
/*   test_word_contains_ligature( "woffle" ); */
/*   test_word_contains_ligature( "puff" ); */
/*   test_word_contains_ligature( "gulf" ); */
/*   test_word_contains_ligature( "sif" ); */
