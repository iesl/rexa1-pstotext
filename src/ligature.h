typedef struct {
  // the real word (with ligatures)
  unsigned char *word;
  // array of ligature insertion positions in deligatured word
  int *ligature_positions;
  // array of ligature indices (into the ligatures and ligature_len arrays)
  int *ligature_ids;
  // ligature count (i.e., size of two preceding arrays)
  int ligature_count;
} LigatureData;

static unsigned char* LIGATURES[] = { "ff", "fl", "fi", "ffi", "ffl" };
static int LIGATURE_LENGTHS[] = { 2, 2, 2, 3, 3 };

int load_ligatures( char *file );
int generate_ligatures( char *dictionary_file, char *ligature_file );
void free_ligatures();
int religature_word( unsigned char *word, int size );
