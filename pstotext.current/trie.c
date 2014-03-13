#include <stdlib.h>
#include <stdio.h>
#include "trie.h"

/*
 * Compact Trie data structure.
 *
 * author: Andrew Tolopko 
 * last modified: 2005-03-27
 */

static int _is_valid_word( unsigned char* );

static char* strdup( char* s ) {
  char *new_str;
  if ( s == NULL ) {
    return s;
  }
  new_str = malloc( strlen( s ) + 1 );
  strcpy( new_str, s );
  return new_str;
}


/* Creates a new tree node.  'prefix' is copied to newly allocated
   memory. */
Trie* trie_new( unsigned char *prefix,
                void *data ) 
{
  Trie* newNode = (Trie*) malloc( sizeof( Trie ) );
/*   newNode->letter = letter; */
  if ( prefix == NULL ) {
    prefix = "";
  }
  if ( !_is_valid_word( prefix ) ) {
    return NULL;
  }
  newNode->prefix = strdup( prefix );
  newNode->is_end_of_string = 0;
  newNode->data = data;
  newNode->children = NULL;
  return newNode;
}

static int _char_to_index( unsigned char c ) {
/*   if ( c >= 'A' && c <= 'Z' ) { */
/*     return c - 'A'; */
/*   } else if ( c >= 'a' && c <= 'z' ) { */
/*     return c - 'a' + ( 'Z' - 'A' ) + 1; */
/*   } else if ( c == '\'' ) { */
/*     return ( 'Z' - 'A' ) + ( 'z' - 'a' ) + 2; */
/*   } else { */
/*     return -1; */
/*   } */

  if ( c < 32 ) {
    return -1;
  } else {
    return c - 32;
  }
/*   return c; */
}

static int _is_valid_word( unsigned char *s ) {
  while ( *s ) {
    if ( _char_to_index( *s ) < 0 ) {
      return 0;
    }
    ++s;
  }
  return 1;
}

/* Retrieve the child node corresponding to character 'c'.  Returns
   NULL if 'c' is not in the approved alphabet. As a sideffect, if
   'prefix' is non-null, it is set to the full prefix of 't'; this is
   used to inform the caller of the (potentially) multi-level path
   that lead to the child. */
static Trie* trie_child( Trie *t, unsigned char c, unsigned char *prefixOut ) {
  if ( !t->children ) {
    return NULL;
  }
  int i = _char_to_index( c );
  if ( i < 0 ) {
    return NULL;
  }
  Trie *child = t->children[i];
  if ( prefixOut &&  child ) {
    strcpy( prefixOut, child->prefix );
  }
  return child;
}

static void _trie_alloc_children_array( Trie *t ) {
  if ( !t->children ) {
    t->children = malloc( TRIE_ALPHABET_SIZE * sizeof( Trie* ) );
    memset( t->children, 0, TRIE_ALPHABET_SIZE * sizeof( Trie* ) );
  }
}

/* Creates a new child for the specified trie node.  'prefix' is
   copied to newly allocated memory. */
static Trie* trie_new_child( Trie *t, unsigned char *prefix ) {
  int i = _char_to_index( prefix[0] );
  if ( i < 0 ) {
    return NULL;
  }
  _trie_alloc_children_array( t );
  if ( !t->children[i] ) {
    t->children[i] = trie_new( prefix, NULL );
    // assert t->children[i]->prefix != prefix
  }
  return t->children[i];
}

/* Add a word (and an optional associated data) to the trie.  's', the
 * word, will be duplicated, so the trie does assume memory
 * "ownership" of this string.  If 'data' is not null, be sure to
 * write and specify a callback function for the 'free_data_callback'
 * argument of trie_free(), so that this "user" data is freed.  If
 * word already exists in the trie, this function returns 0 and the
 * trie will not be modified.
 * 
 */
int trie_add( Trie *t, unsigned char* s, void *data ) {
  Trie *last = t;
  unsigned char prefix[MAX_WORD_LEN + 1];
  prefix[0] = 0;
  // check validity of word before (potentially) making any changes to
  // the trie, which might otherwise leave it with a partially-added
  // path (because an invalid char is found later in the word)
  if ( !_is_valid_word( s ) ) {
    return 0;
  }
  // (we could make this duplication check while we add, but this is
  // more readable)
  if ( trie_contains( t, s ) ) {
    return 0;
  }
  do {
    if ( *s == 0 || t == NULL ) {
      if ( t == NULL ) {
        // it's now time to allocate our new child node
        t = trie_new_child( last, s );
      }
      t->is_end_of_string = 1;
      t->data = data;
      return 1;
    } 
    else {
      last = t;
      Trie *child = trie_child( t, s[0], prefix );
      if ( !child ) {
        // cause logic, above, to create a new child
        t = NULL;
      } else {
        int n = 0;
        while ( prefix[n] && s[n] && prefix[n] == s[n] ) {
          ++n;
        }
        // if 's' is a prefix of child->prefix, or if 's' differs from
        // prefix at some point, then we must split the folded-path
        // child into two nodes
        if ( prefix[n] != 0 ) {
          // insert new intermediate node, breaking up a folded path
          int old_child_index = _char_to_index( child->prefix[0] );
          // set the child's prefix to the tail of the original prefix
          memmove( child->prefix, child->prefix + n, strlen( child->prefix + n ) + 1 );
          // orphan the child (for later re-parenting)
          t->children[old_child_index] = NULL;
          // insert the new intermediate child, assigning it the head
          // of the original prefix; 't' will now represent the new,
          // intermediate child
          prefix[n] = 0;
          t = trie_new_child( t, prefix );
          // make the new intermediate child the parent of the
          // original child
          _trie_alloc_children_array( t );
          t->children[_char_to_index( child->prefix[0] )] = child;
          // we now resume the normal loop logic, which will finish up
          // the initialization of our new intermediate child and then
          // return...
        } else {
          t = child;
        }
        s += n;
      }
    }
  } while ( 1 );
  return 1;
}

/* 
 * Returns non-null if trie contains the specified word, otherwise
 * null.  In fact, returns the very node of the trie that represents
 * the word, if it exists.  's' must be null terminated. */
Trie* trie_contains( Trie *t, unsigned char* s ) {
  unsigned char *prefix;
  do {
    prefix = t->prefix;
    while ( prefix[0] && s[0] && prefix[0] == s[0] ) {
      ++prefix;
      ++s;
    }
    if ( prefix[0] ) {
      // node's prefix is *longer* than the word we're checking, so
      // word is not contained in trie
      return NULL;
    }

    // if we've reached the end of our word, then this node must also
    // represent the end of a trie member, if the word is contained in
    // the trie
    unsigned char c = s[0];
    if ( c == 0 ) {
      if ( t->is_end_of_string ) {
        return t;
      } else { 
        return NULL;
      }
    }
    t = trie_child( t, c, NULL );
    if ( !t ) {
      return NULL;
    }
  } while ( 1 );
}

/*
 * Removes the specified word from the tree (effectively).  Note: this
 * function is not sophisticated, in that it does not collapse and
 * remove the node entirely; so a trie with removed nodes may not be
 * as compact as it could be (rebuild your trie if you really care
 * about this!).
 */
int trie_remove( Trie *t, 
                 unsigned char* s, 
                 void (*free_data_callback) (void*) ) {
  Trie* node = trie_contains( t, s );
  if ( node ) {
    node->is_end_of_string = 0;
    if ( node->data && free_data_callback ) {
      free_data_callback( node->data );
      node->data = NULL;
    }
    return 1;
  }
  return 0;
}


static void _do_trie_iterate_dfs( Trie *t,
                                  unsigned char *prefix,
                                  int n,
                                  int (*callback) (Trie *node,
                                                   unsigned char *prefix,
                                                   void *callback_data),
                                  void *callback_data ) {
  int i;
  strcpy( prefix + n, t->prefix );
  n += strlen( t->prefix );
  if ( callback( t, prefix, callback_data ) ) {
    if ( t->children ) {
      Trie *child;
      for ( i = 0; i < TRIE_ALPHABET_SIZE; ++i ) {
        if ( ( child = t->children[i] ) != 0 ) {
          _do_trie_iterate_dfs( child, 
                                prefix, 
                                n, 
                                callback, 
                                callback_data );
        }
      }
    }
  }
}

void trie_iterate_dfs( Trie *t, 
                       int (*callback) (Trie *node, 
                                        unsigned char *prefix,
                                        void *callback_data),
                       void *callback_data ) {
  unsigned char prefix[MAX_WORD_LEN + 1];
  _do_trie_iterate_dfs( t, prefix, 0, callback, callback_data );
}

static int trie_display_node( Trie *node, 
                              unsigned char *prefix, 
                              void *callback_data ) {
  if ( node->is_end_of_string ) {
    printf( "%s\n", prefix );
  }
  return 1;
}

void trie_display( Trie *t ) {
  trie_iterate_dfs( t, trie_display_node, NULL );
}
  

int trie_size( Trie *t ) {
  int n = 0;
  int i;
  if ( t->children ) {
    Trie *child;
    for ( i = 0; i < TRIE_ALPHABET_SIZE; ++i ) {
      if ( ( child = t->children[i] )  != 0 ) {
        n += trie_size( child );
      }
    }
  }
  if ( t->is_end_of_string ) {
    return n + 1;
  } else {
    return n;
  }
}

void trie_free( Trie *t,
                void (*free_data_callback) (void*) ) {
  int i;
  if ( t == NULL ) {
    return;
  }
  if ( t->children ) {
    Trie *child;
    for ( i = 0; i < TRIE_ALPHABET_SIZE; ++i ) {
      if ( ( child = t->children[i] ) != 0 ) {
        trie_free( child, free_data_callback );
      }
    }
  }
  if ( t->prefix ) {
    free( t->prefix );
  }
  if ( t->children ) {
    free( t->children );
  }
  if ( t->data && free_data_callback ) {
    free_data_callback( t->data );
  }
  free( t );
}

/*
 * A convenience function that can be used as the callback argument to
 * the trie_free() and trie_remove(), when the 'data' field of trie
 * nodes do not contain complex data structures (that contain nested
 * memory references that need to be freed).
 */
void trie_free_simple_data_callback( void *data ) {
  free( data );
}
