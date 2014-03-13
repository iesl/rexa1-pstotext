/*
 * Compact Trie data structure.
 *
 * author: Andrew Tolopko 
 * last modified: 2005-03-27
 */

//#define TRIE_ALPHABET_SIZE ( ( 'Z' - 'A' ) + 1 + ( 'z' - 'a' ) + 1 + 1 )
#define TRIE_ALPHABET_SIZE ( ( 256 - 32 ) + 1 )
#define MAX_WORD_LEN 256

typedef struct trie_node Trie;

struct trie_node {
  // since this is a *compact* trie, any node can be associated with a
  // string *prefix* (rather than just a single char)
  unsigned char *prefix;
  // if true, this node and its parents together specify a word that
  // has been added to the trie
  int is_end_of_string;
  // for aribtrary data that can be associated with a trie node
  void *data;
  // we lazily allocate this array, since it would be wasteful to have
  // leaf nodes allocate this storage (since leaf nodes might comprise
  // the majority of the tree nodes)
  Trie** children;
};


Trie* trie_new( unsigned char *prefix, void *data );
int trie_add( Trie *t, unsigned char* s, void *data );
Trie* trie_contains( Trie *t, unsigned char* s );
int trie_remove( Trie *t, unsigned char *s, void (*free_data_callback) (void*) );
void trie_iterate_dfs( Trie *t, 
                       int (*callback) (Trie *node, 
                                        unsigned char *prefix,
                                        void *callback_data),
                       void *callback_data );
void trie_display( Trie *t );
int trie_size( Trie *t );
void trie_free( Trie *t, void (*free_data_callback) (void*) );
void trie_free_simple_data_callback( void *data );
