/* Copyright (C) 1995-1998, Digital Equipment Corporation.    */
/* All rights reserved.                                       */
/* See the file pstotext.txt for a full description.          */
/* Last modified on Wed Oct 28 08:42:15 PST 1998 by mcjones   */
/*      modified on Sun Jul 28 00:00:00 UTC 1996 by rjl       */

/* This module is based on OCR_PS.m3, a module of the Virtual Paper
   project at the DEC Systems Research Center:
   http://www.research.digital.com/SRC/virtualpaper/ */

#ifdef VMS
#include <stdlib.h>
#endif

#include <stdio.h>
#include <math.h>
#include "ptotdll.h"
#include "ligature.h"

#include <string.h>
#ifdef NEED_PROTO
#include <stdlib.h>
#endif

#ifndef NULL
#define NULL 0
#endif

#define BOOLEAN int
#define FALSE 0
#define TRUE 1

#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((b)<=(a)?(a):(b))

/* [ant] 
 * 
 * A postscript document's string data contains "raw" character codes.
 * 
 * The raw char code is used to index into the current encoding [a 256
 * element array, a postscript construct] to produce a postscript
 * glyph name.
 *
 * Using the StandardMap [a pstotext (arbitrary) construct, which
 * happens to correspond to postscript's Standard Encoding], defined
 * in ocr.ps, a glyph name is then mapped to a a glyph index.  This
 * glyph index only has meaning within the context of pstotext (in
 * particular, ptotdll.c); we're "outside" the world of postscript
 * now.
 *
 * This pstotext glyph index is used to lookup the final Latin1
 * character (that will be output by pstotext) by indexing into one of
 * the following arrays (these arrays are logically, and probably
 * should be physically, concatenated together)
 */
 

/* Character encoding.  Each element of the QE directive produced by
   ocr.ps is either an index in the StandardGlyph array, or is
   "NonstandardGlyph" (indicating the corresponding entry in the font's
   encoding specifies some nonstandard glyph). */

typedef unsigned GlyphIndex;
#define NonstandardGlyph 9999

// [ant: added]
// Value found in an Encoding Vector when a character code has no
// glyph name mapping; equivalent to the glyph name ".notdef" (as
// defined by PostScript)
#define GlyphNotDefined 0

// ASCII output char to be substituted for nonstandard glyph
#define UnknownChar '#' 

// ACS: Added: 
#define LASTAsciiCodeLow  31

static char *AsciiCodesLow[] = {
  "NUL",  // 000
  "SOH",  // 001
  "STX",  // 002
  "ETX",  // 003
  "EOT",  // 004
  "ENQ",  // 005
  "ACK",  // 006
  "BEL",  // 007
  "BS",   // 008
  "HT",   // 009
  "LF",   // 010
  "VT",   // 011
  "FF",   // 012
  "CR",   // 013
  "SO",   // 014
  "SI",   // 015
  "DLE",  // 016
  "DC1",  // 017
  "DC2",  // 018
  "DC3",  // 019
  "DC4",  // 020
  "NAK",  // 021
  "SYN",  // 022
  "ETB",  // 023
  "CAN",  // 024
  "EM",   // 025
  "SUB",  // 026
  "ESC",  // 027
  "FS",   // 028
  "GS",   // 029
  "RS",   // 030
  "US",   // 031
};

/* The first 256 entries in StandardGlyphs correspond to ISOLatin1;
   the next 28 entries correspond to characters not in ISOLatin1, but
   defined in the standard /Times-Roman font. */

#define LastISOLatin1 255

#define FIRSTSpecialGlyphs (LastISOLatin1+1)
#define LASTSpecialGlyphs (LastISOLatin1+28)
static char *SpecialGlyphs[] = {
    "\"",    /* quotedblright */
    "S\237", /* Scaron */
    "+",     /* dagger */
    "<",     /* guilsinglleft */
    "Z\237", /* Zcaron */
    "#",     /* daggerdbl */
    "L/",    /* Lslash */
    "...",   /* ellipsis */
    ">",     /* guilsinglright */
    "oe",    /* oe */
    "fi",    /* fi */
    ".",     /* bullet */
    "o/oo",  /* perthousand */
    "\"",    /* quotedblbase */
    "--",    /* endash */
    "---",   /* emdash */
    "^TM",   /* trademark */
    "f",     /* florin */
    "l/",    /* lslash */
    "s\237", /* scaron */
    "Y\250", /* Ydieresis */
    "fl",    /* fl */
    "/",     /* fraction */
    "\"",    /* quotedblleft */
    "'",     /* quotesinglbase */
    "'",     /* quotesingle */
    "z\237", /* zcaron */
    "OE"     /* OE */
  };

/* The next 256 entries correspond to the self-named glyphs used in
   Type 3 fonts from dvips: "\000", ..., "\377":  */

#define FirstDvips (LASTSpecialGlyphs+1)
#define LastDvips  (FirstDvips+256-1)

/* The next 512 entries correspond to glyph names used in Microsoft
   TrueType fonts: "G00", ..., "Gff" and "G00", ..., "GFF", which
   in both cases correspond to ISOLatin1 with some extensions. */

#define FirstTT1 (LastDvips+1)
#define LastTT1 (FirstTT1+256-1)
#define FirstTT2 (LastTT1+1)
#define LastTT2 (FirstTT2+256-1)
#define FirstOldDvips (LastTT2+1)
#define LastOldDvips (FirstOldDvips+128-1) /* note only 128 */

#define FIRSTTTSpecialGlyphs (FirstTT1+130)
#define LASTTTSpecialGlyphs (FirstTT1+159)
static char *TTSpecialGlyphs[] = {
    "'",     /* quotesinglbase */
    "f",     /* florin */
    "''",    /* quotdblbase */
    "...",   /* ellipsis */
    "+",     /* dagger */
    "#",     /* daggerdbl */
    "\223",  /* circumflex */
    "o/oo",  /* perthousand */
    "S\237", /* Scaron */
    "<",     /* guilsinglleft */
    "OE",    /* OE */
    "#",     /* <undefined> */
    "#",     /* <undefined> */
    "#",     /* <undefined> */
    "#",     /* <undefined> */
    "`",     /* ISOLatin1: quoteleft */
    "'",     /* ISOLatin1: quoteright */
    "``",    /* quotedblleft */
    "''",    /* quotedblright */
    ".",     /* bullet */
    "--",    /* endash */
    "---",   /* emdash */
    "~",     /* ISOLatin1: tilde */
    "^TM",   /* trademark */
    "s\237", /* scaron */
    ">",     /* guilsinglright */
    "oe",    /* oe */
    "#",     /* <undefined> */
    "#",     /* <undefined> */
    "Y\250"  /* Ydieresis" */
  };

#define FIRSTDvipsGlyphs FirstDvips
#define LASTDvipsGlyphs (FirstDvips+127)
static char *DvipsGlyphs[] = {
  /* 00x */
    "\\Gamma", "\\Delta", "\\Theta", "\\Lambda",
    "\\Xi", "\\Pi", "\\Sigma", "\\Upsilon",
  /* 01x */
    "\\Phi", "\\Psi", "\\Omega", "ff", "fi", "fl", "ffi", "ffl",
  /* 02x */
    "i",     /* \imath */
    "j",     /* \jmath */
    "`",
    "'",
    "\237",  /* caron */
    "\226",  /* breve */
    "\257",  /* macron */
    "\232",  /* ring */
  /* 03x */
    "\270",  /* cedilla */
    "\337",  /* germandbls */
    "ae",
    "oe",
    "\370",  /* oslash */
    "AE",
    "OE",
    "\330",  /* Oslash */
  /* 04x */
    "/" /* bar for Polish suppressed-L ??? */, "!", "''", "#",
    "$", "%", "&", "'",
  /* 05x */
    "(", ")", "*", "+",
    ",", "\255" /* hyphen */, ".", "/",
  /* 06x */
    "0", "1", "2", "3", "4", "5", "6", "7",
  /* 07x */
    "8", "9", ":", ";",
    "!" /* exclamdown */, "=", "?" /* questiondown */, "?",
  /* 010x */
    "@", "A", "B", "C", "D", "E", "F", "G",
  /* 011x */
    "H", "I", "J", "K", "L", "M", "N", "O",
  /* 012x */
    "P", "Q", "R", "S", "T", "U", "V", "W",
  /* 013x */
    "X", "Y", "Z", "[",
    "``", "]", "\223" /* circumflex */, "\227" /* dotaccent */,
  /* 014x */
    "`", "a", "b", "c", "d", "e", "f", "g",
  /* 015x */
    "h", "i", "j", "k", "l", "m", "n", "o",
  /* 016x */
    "p", "q", "r", "s", "t", "u", "v", "w",
  /* 017x */
    "x", "y", "z",
    "--",    /* en dash */
    "---",   /* em dash */
    "\235",  /* hungarumlaut */
    "~",
    "\250"   /* dieresis */
  };

#define FIRSTCorkSpecialGlyphs FirstDvips
#define LASTCorkSpecialGlyphs (FirstDvips+0277)
static char *CorkSpecialGlyphs[] = {
  /* 000 - accents for lowercase letters */
    "`",
    "'",
    "^",
    "~",
    "\230",  /* umlaut/dieresis */
    "\235",  /* hungarumlaut */
    "\232",  /* ring */
    "\237",  /* hacek/caron */
    "\226",  /* breve */
    "\257",  /* macron */
    "\227",  /* dot above/dotaccent */
    "\270",  /* cedilla */
    "\236",  /* ogonek */
  /* 015 - miscellaneous */
    "'",     /* single base quote/quotesinglbase */
    "<",     /* single opening guillemet/guilsinglleft */
    ">",     /* single closing guillemet/guilsinglright */
    "``",    /* english opening quotes/quotedblleft */
    "''",    /* english closing quotes/quotedblright */
    ",,",    /* base quotes/quotedblbase */
    "<<",    /* opening guillemets/guillemotleft */
    ">>",    /* closing guillemets/guillemotright */
    "--",    /* en dash/endash */
    "---",   /* em dash/emdash */
    "",      /* compound work mark (invisible)/ */
    "o",     /* perthousandzero (used in conjunction with %) */
    "\220",  /* dotless i/dotlessi */
    "j",     /* dotless j */
    "ff",    /* ligature ff */
    "fi",    /* ligature fi */
    "fl",    /* ligature fl */
    "ffi",   /* ligature ffi */
    "ffl",   /* ligature ffl */
    "_",     /* visible space */
  /* 041 - ASCII */
         "!", "\"", "#", "$", "%", "&", "'",
    "(", ")", "*", "+", ",", "-", ".", "/",
    "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", ":", ";", "<", "=", ">", "?",
    "@", "A", "B", "C", "D", "E", "F", "G",
    "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W",
    "X", "Y", "Z", "[", "\\","]", "^", "_",
    "`", "a", "b", "c", "d", "e", "f", "g",
    "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w",
    "x", "y", "z", "{", "|", "}", "~", "\255", /* hyphenchar (hanging) */
  /* 200 - letters for eastern European languages from latin-2 */
    "A\226", /* Abreve */
    "A\236", /* Aogonek */
    "C\264", /* Cacute */
    "C\237", /* Chacek */
    "D\237", /* Dhacek */
    "E\237", /* Ehacek */
    "E\236", /* Eogonek */
    "G\226", /* Gbreve */
    "L\264", /* Lacute */
    "L\237", /* Lhacek */
    "L/",    /* Lslash/Lstroke */
    "N\264", /* Nacute */
    "N\237", /* Nhacek */
    "\\NG",  /* Eng */
    "O\235", /* Ohungarumlaut */
    "R\264", /* Racute */
    "R\237", /* Rhacek */
    "S\264", /* Sacute */
    "S\237", /* Shacek */
    "S\270", /* Scedilla */
    "T\237", /* Thacek */
    "T\270", /* Tcedilla */
    "U\235", /* Uhungarumlaut */
    "U\232", /* Uring */
    "Y\250", /* Ydieresis */
    "Z\264", /* Zacute */
    "Z\237", /* Zhacek */
    "Z\227", /* Zdot */
    "IJ",    /* IJ */
    "I\227", /* Idot */
    "\\dj",  /* dbar */
    "\247",  /* section */
    "a\226", /* abreve */
    "a\236", /* aogonek */
    "c\222", /* cacute */
    "c\237", /* chacek */
    "d\237", /* dhacek */
    "e\237", /* ehacek */
    "e\236", /* eogonek */
    "g\226", /* gbreve */
    "l\222", /* lacute */
    "l\237", /* lhacek */
    "l/",    /* lslash */
    "n\222", /* nacute */
    "n\237", /* nhacek */
    "\\ng",  /* eng */
    "o\235", /* ohungarumlaut */
    "r\222", /* racute */
    "r\237", /* rhacek */
    "s\222", /* sacute */
    "s\237", /* shacek */
    "s\270", /* scedilla */
    "t\237", /* thacek */
    "t\270", /* tcedilla */
    "u\235", /* uhungarumlaut */
    "u\232", /* uring */
    "y\230", /* ydieresis */
    "z\222", /* zacute */
    "z\237", /* zhacek */
    "z\227", /* zdot */
    "ij",    /* ij */
    "\241",  /* exclamdown */
    "\277",  /* questiondown */
    "\243"   /* sterling */
  /* 0300-0377 is same as ISO 8859/1 except:
       0337 is Ess-zed and 0377 is ess-zed/germandbls */
};

/*
  There are 2 problems with unprintable characters:

  1. The above arrays, which specify the character codes to be output
  for particular PostScript glyphs in many cases specify codes that
  are unused/unprintable in the ISOLatin1 encoding.  I suppose the
  original developers assumed the Windows-1252 char set was being
  used, which makes these char codes printable.

  2. The standardMap in ocr.ps maps glyph names to a glyph index,
  which is then used to index into one of the above arrays (or, if the
  index < 256, directly in the implicit ISOLatin1 char set, which is
  not redefined above).  The problem is that standardMap maps some
  glyph names to unprintable ISOLatin1 char codes.  In a few cases
  this occurs because the glyph name is defined twice and the
  occurrence that ultimately makes it into the standardMap is the one
  that specifies an unprintable ISOLatin1 character.

  HACK: After pstotext figures out what ISOLatin1 characters it wants
  to output, we'll reprocess the unprintable chars by trying to map
  them to printable characters in the ISOLatin1 char set.  Some of
  these unprintable characters are printable if the Windows-1252 char
  set is used, but we don't want to use that encoding (which is a
  superset of ISOLatin1).  This is also fixes the bug where the ocr.ps
  standardMap sometimes maps glyph names to unprintable ISOLatin1
  characters.  

  The elegant way to address the unprintable char problems would be to:
  
  - Eliminate the standardMap and all of the *Glyphs arrays in ocr.ps

  - Have ocr.ps output the actual glyph *names* (strings), rather than
  indexes into our own (proprietary) mapping of glyph names->Latin1
  CharCodes.  (ocr.ps maps to our "own" glyph indexes, and then to
  Latin1 char codes).

  - Define, in ptotdll.c, a map (perhaps using trie.c) that maps glyph
  names directly Latin1 char codes.  (This eliminates the intermediate
  indexing scheme currently in place.)
*/
#define FirstUnprintableISOLatin1Code 0x7f
#define LastUnprintableISOLatin1Code 0x9f
static char UnprintableISOLatin1Glyphs[] = {
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // .notdef
  'i',    // dotlessi  (not dotless, but the best we can do?)
  '`',    // grave
  0xb4,   // acute
  '^',    // circumflex
  '~',    // tilde
  0xaf,   // macron
  GlyphNotDefined,     // breve
  0xb7,   // dotaccent (output as a "middle dot")
  0xa8,   // dieresis
  GlyphNotDefined,    // .notdef
  0xba,   // ring
  0xb8,   // cedilla 
  GlyphNotDefined,    // .notdef
  GlyphNotDefined,    // hungarumlaut
  GlyphNotDefined,    // ogonek
  GlyphNotDefined     // caron
};


typedef struct {
  double blx, bly, toprx, topry; /* font matrix in character coordinates */
  struct {double x, y;} chr[256]; /* widths in character coordinates */
} MetricsRec;
typedef MetricsRec *Metrics;
typedef Metrics MetricsTable[];

typedef GlyphIndex EncodingVector[256];
typedef EncodingVector *Encoding;
typedef Encoding EncodingTable[];

#define FONTNAME_SIZE (256)
typedef struct {
  double x, y; /* (1000,0) in font's character coordinate system */
  double xp, yp; /* (0,1000) in font's character coordinate system */
  int e; /* index in "encodings" */
  int m; /* index in "metrics" */
  double bx, by, tx, ty; /* height of font bbox in reporting coordinates */
  char fontName[FONTNAME_SIZE];
} FontRec;
typedef FontRec *Font;
typedef Font FontTable[];

typedef int TokenType;

#define TOKEN(TID) ((TokenType)TID)
#define STACK_SIZE  64
#define DECLARE_STACK(T) T T##Stack[STACK_SIZE]; int T##StackLength

#define BUFSIZE  (1024*10)

#define MAX_WORD_LEN 1000

/* Instance "T". */
typedef struct {
  double itransform[6]; /* transform from device to default coordinates */
  int metricsSize;
  MetricsTable *metrics;
  int encodingsCount;
  EncodingTable *encodings;
  BOOLEAN dvipsIsCork; /* assume Cork rather than "OT1" for dvips output */
  BOOLEAN fixMissingLigatures; /* assume Cork rather than "OT1" for dvips output */

  int fontCapacity;
  FontTable *font;
  int f; /* font number */

  /* last word output */
  char word[MAX_WORD_LEN]; 

  /* Data for current word prefix; in ParseString(), a word is built
     up within this buffer, 1 character at a time; when a complete
     word has been read in, it is copied to 'word'. */
  char buf[BUFSIZE];
  int lbuf; /* elements 0 through "lbuf-1" of "buf" are in use */

  double x0, y0, x1, y1; /* initial and final currentpoint */
  double x1_prev;
  double last_interword_space;

  BOOLEAN nonEmptyPage;
  long blx, bly, toprx, topry; /* bounding box of last word output */
  /* state_encoding: */ int encoding_e, encoding_n;
  /* state_metrics: */ int metrics_m;;
  int lastFont;
  int linenum;
  int pagenum;
  double lastGlyphXWidth;
  double lastGlyphYWidth;
  double lastGlyphSpaceToLRatio;
  
  char outputBuffer[4096];

  DECLARE_STACK( double );
  DECLARE_STACK( int );
  DECLARE_STACK( TokenType );

  /* keep track of page bounding box */
  double pageLLX, pageLLY, pageURX, pageURY; 
} T;

#ifdef NEED_PROTO
static int ReadChar(char **instr);
static int ReadTrimmedString(char **instr, char *buf);
static void UnreadChar(char **instr);
static int ReadInt(char **instr);
static long ReadLong(char **instr);
static int ParseLogComment(T *t, char *instr);
static int ParseInverseTransform(T *t, char *instr);
static int ParseEncoding(T *t, char *instr);
static int ParseEncodingMore(T *t, char *instr);
static void ReadPair(double *x, double *y, char **instr);
static int ParseFont(T *t, char *instr);
static int ParseMetrics(T *t, char *instr);
static int ParseMetricsMore(T *t, char *instr);
static void Itransform(T *t, double *x1, double *y1, double x0, double y0);
static void OutputWord(T *t, char **word, char *xmlOutputBuffer, int *llx, int *lly, int *urx, int *ury);
static BOOLEAN SameDirection(double x0, double y0, double x1, double y1);
static int ParseString(T *t, char *instr, char **word, char *xmlOutputBuffer, int *llx, int *lly, int *urx, int *ury);
#endif

static void PushTokenStack(T* t, int token);
static int PopTokenStack(T* t);
static int PeekTokenStack(T* t);
static void UnwindAndProcessTokenStack(T* t, int boundingTokenType, char* xmlOutputBuffer);
static void OutputTokenEnd(T* t, int tokenType, char* xmlOutputBuffer);
static BOOLEAN  TokenStackHasToken(T* t, int tokenType);
static int ParseEncodingMore(T *t, char *instr);
static int ParseMetricsMore(T *t, char *instr);
static void fixUnprintableLatin1CharCodes( unsigned char *buf, int *len );

#define ZAP_BUFFER(ZZBUF) do { int i=0; for(;i<sizeof(ZZBUF);i++) (ZZBUF)[i]=0; } while(0)

static void ClearOutputBuffer(T* t) {
  ZAP_BUFFER( t->outputBuffer );
  t->last_interword_space = 0;
}

static void AppendToOutputBuffer(T* t, char *text) {
  strcpy( t->outputBuffer + strlen( t->outputBuffer ), text );
}

static char* GetOutputBuffer(T* t) {
  return t->outputBuffer;
}


#define NULL_TOKEN 		TOKEN( 0 )
#define DOCUMENT_TOKEN 		TOKEN( 1 )
#define PAGE_TOKEN		TOKEN( 2 )
#define LINE_TOKEN		TOKEN( 3 )
#define FONT_TOKEN  	 	TOKEN( 4 )
#define CDATA_TOKEN		TOKEN( 5 )
#define FONT_DEFINITION_TOKEN  	TOKEN( 6 )
#define TBOX_TOKEN  		TOKEN( 7 )

/* ******************************************
*/
#define PUSH_STACK(ZZT, ARG)  t-> ZZT##Stack[ t-> ZZT##StackLength++ ] = ARG

#define POP_STACK(ZZT,NULLVAL,DEFAULTVAL)  if ( t-> ZZT##StackLength > 0 ) {\
  ZZT top = t-> ZZT##Stack[--t-> ZZT##StackLength];\
  t-> ZZT##Stack[t-> ZZT##StackLength] = NULLVAL;\
  return top; } return DEFAULTVAL

#define PEEK_STACK(ZZT,DEFAULTVAL) if ( t-> ZZT##StackLength > 0 ) return t-> ZZT##Stack[t-> ZZT##StackLength-1]; return DEFAULTVAL

#define STACK_HAS_VALUE(ZZT,VAL,NULLVAL) \
  {int i=0; for(;i<t-> ZZT##StackLength; i++) { \
    if( t-> ZZT##Stack[i] == VAL ) { return TRUE; }} \
  return (tokenType == NULLVAL); }

#define INIT_STACK(ZZT,NULLVAL) t-> ZZT##StackLength = 0; do { int i=0; for(;i<STACK_SIZE; i++) t-> ZZT##Stack[i]=(NULLVAL); } while(0)

/* ******************************************
   int stack
*/
static void PushIntArgStack(T* t, int arg) {
  PUSH_STACK( int, arg );
}

static int PopIntArgStack(T* t) {
  POP_STACK( int, 0, 0 );
}

/* ******************************************
   double stack
*/
static void PushDblArgStack(T* t, double arg) {
  PUSH_STACK( double, arg );
}

static double PopDblArgStack(T* t) {
  POP_STACK( double, 0.0, 0.0 );
}


/* ******************************************
   token stack
*/
static void PushTokenStack(T* t, int token) {
  PUSH_STACK( TokenType, token );
}

static int PopTokenStack(T* t) {
  POP_STACK( TokenType, NULL_TOKEN, NULL_TOKEN );
}

static int PeekTokenStack(T* t) {
  PEEK_STACK( TokenType, NULL_TOKEN );
}

static BOOLEAN  TokenStackHasToken(T* t, int tokenType) {
  STACK_HAS_VALUE( TokenType, tokenType, NULL_TOKEN );
}

/* ******************************************
*/
static void UnwindAndProcessTokenStack(T* t, int boundingTokenType, char* xmlOutputBuffer) {
  if ( TokenStackHasToken( t, boundingTokenType ) ) { 
    while ( PeekTokenStack( t ) != boundingTokenType ) {
      OutputTokenEnd( t, PopTokenStack( t ), xmlOutputBuffer );
    }
    OutputTokenEnd( t, PopTokenStack( t ), xmlOutputBuffer );
  }
}



/* ******************************************
*/
static void OutputTokenEnd(T* t, int tokenType, char* xmlOutputBuffer) {
  switch(tokenType) {

  case DOCUMENT_TOKEN : 
    sprintf( xmlOutputBuffer + strlen( xmlOutputBuffer ), "</document>" );
    break;

  case PAGE_TOKEN :
    sprintf( xmlOutputBuffer + strlen( xmlOutputBuffer ), "\n<pbox llx=\"%.2f\" lly=\"%.2f\" urx=\"%.2f\" ury=\"%.2f\"/>", 
	     t->pageLLX, t->pageLLY, t->pageURX, t->pageURY );

    sprintf( xmlOutputBuffer + strlen( xmlOutputBuffer ), "</page>" );

    t->pageLLX = t->pageLLY = t->pageURX = t->pageURY = -1.0;
    break;	

  case LINE_TOKEN : 
    sprintf( xmlOutputBuffer + strlen( xmlOutputBuffer ), "</line>" );
    break;

  case TBOX_TOKEN : {
    int urx = PopDblArgStack( t );
    int ury = PopDblArgStack( t );
    int llx = PopDblArgStack( t );
    int lly = PopDblArgStack( t );
    int fontIndex = PopIntArgStack( t );

    // this is where we take the accumulated list of words in
    // t->outputBuffer and inject them into our XML'ified
    // xmlOutputBuffer
    char* cdataBuffer = GetOutputBuffer( t );
    sprintf( xmlOutputBuffer + strlen( xmlOutputBuffer ), 
             "<tbox llx=\"%d\" lly=\"%d\" urx=\"%d\" ury=\"%d\" f=\"%d\"><![CDATA[%s]]></tbox>", 
	     llx, lly, urx, ury, 
	     fontIndex,
	     cdataBuffer );
  }
    break;

  case CDATA_TOKEN : 
    break;

  case FONT_TOKEN :
    break;

  default:
    break;
  }
}


/* Debug output functions. */
/* We'll use preprocessor directives to avoid incurring the expense of
   real function calls when we're not interested in seeing this
   output */
#define PEDAGOGICAL_DEBUG_OUTPUT 0
#if PEDAGOGICAL_DEBUG_OUTPUT
static void logGlyph( int charCode, int glyph, char* msg ) {
  fprintf( stderr, "%c\t%d\t%c\t%d\t%s\n", charCode, charCode, glyph, glyph, msg );
}

static void logEncoding( Encoding encoding ) {
  int i;
  for (i = 0; i<256; i++) {
    if ( i != (*encoding)[i] ) {
      fprintf( stderr, "Encoding %d\t%d\n", i, (*encoding)[i] );
    }
  }
}
#else
#define logGlyph( charCode, glyph, msg )
#define logEncoding( encoding )
#endif



/* ******************************************
*/
int DLLEXPORT pstotextInit(instance) void **instance; {
  T *t;
  int i;

  t = (T *)malloc(sizeof(T));
  if (t == NULL) return PSTOTEXT_INIT_MALLOC;

/*   t->state = state_normal; */

  /* Initialize t->itransform to the identity transform. */
  t->itransform[0] = 1.0;
  t->itransform[1] = 0.0;
  t->itransform[2] = 0.0;
  t->itransform[3] = 1.0;
  t->itransform[4] = 0.0;
  t->itransform[5] = 0.0;

  t->metricsSize = t->encodingsCount = t->fontCapacity = 100;

  t->metrics = (MetricsTable *)malloc(t->metricsSize * sizeof(Metrics));
  if (t->metrics == NULL) {
    free(t);
    return PSTOTEXT_INIT_MALLOC;
  }
  for(i=0; i<t->metricsSize; i++)(*t->metrics)[i] = NULL;

  t->encodings = (EncodingTable *)malloc(t->encodingsCount * sizeof(Encoding));
  if (t->encodings == NULL) {
    free(t);
    return PSTOTEXT_INIT_MALLOC;
  }
  for(i=0;i<t->encodingsCount;i++)(*t->encodings)[i] = NULL;

  t->dvipsIsCork = FALSE;

  t->font = (FontTable *)malloc(t->fontCapacity * sizeof(Font));
  if (t->font == NULL) {
    free(t);
    return PSTOTEXT_INIT_MALLOC;
  }
  for(i=0;i<t->fontCapacity;i++)(*t->font)[i] = NULL;

  t->lbuf = 0;
  t->nonEmptyPage = FALSE;
  t->blx = t->bly = t->toprx = t->topry = 0;

  t->lastFont=0;
  t->linenum = 0;
  t->pagenum = 0;
  t->lastGlyphXWidth = 0.0;
  t->lastGlyphYWidth = 0.0;
  t->lastGlyphSpaceToLRatio = 0.0;

  INIT_STACK( int, 0 );
  INIT_STACK( double, 0.0 );
  INIT_STACK( TokenType, NULL_TOKEN );
  
  t->pageLLX = t->pageLLY = t->pageURX = t->pageURY = -1.0;

  *instance = t;

  return 0;
}

int DLLEXPORT pstotextSetCork(instance, value) void *instance; int value; {
  T *t = (T *)instance;
  t->dvipsIsCork = value;
  return 0;  
}

int DLLEXPORT pstotextSetFixMissingLigatures(instance, value, lig_dictionary) 
     void *instance; 
     int value;
     char *lig_dictionary; {
  T *t = (T *)instance;
  t->fixMissingLigatures = value;
  load_ligatures( lig_dictionary );
  return 0;  
}

int DLLEXPORT pstotextExit(instance) void *instance; {
  T *t = (T *)instance;
  free(t->metrics);
  free(t->encodings);
  free(t->font);
  free(t);
  free_ligatures();
  return 0;
}

static int ReadChar(instr) char **instr; {
  int c = **(unsigned char**)instr;
  (*instr)++;
  return c;
}

static void UnreadChar(instr) char **instr; {
  (*instr)--;
}

static void ReadWhiteSpace( char **instr ) {
  int c;
  while ((c = ReadChar(instr))==' ' || c == '\n' ) /* skip */ ;
  UnreadChar( instr );
}

static void ReadTrimmedString(instr, buf) char **instr; char *buf; {
  int c;
  ReadWhiteSpace( instr );

  int i = 0;
  c = ReadChar(instr);
  while ( 32 <= c && c <= 255 && c != ' ' && c != '\n' ) {
    *(buf+i) = (char)c;
    ++i;
    c = ReadChar( instr );
  }
  UnreadChar(instr);
}

static int ReadInt(instr) char **instr; {
  int i = 0;
  int sign = 1;
  int c;
  ReadWhiteSpace( instr );
  c = ReadChar(instr);
  if (c=='-') {sign = -1; c = ReadChar(instr); }
  while ('0' <= c && c <= '9') {i = i*10+(c-'0'); c = ReadChar(instr);}
  UnreadChar(instr);
  return i*sign;
}

static long ReadLong(instr) char **instr; {
  long i = 0;
  int sign = 1;
  int c;
  ReadWhiteSpace( instr );
  c = ReadChar(instr);
  if (c=='-') {sign = -1; c = ReadChar(instr); }
  while ('0' <= c && c <= '9') {i = i*10+(c-'0'); c = ReadChar(instr);}
  UnreadChar(instr);
  return i*sign;
}

static int ParseLogComment(t, instr) T *t; char *instr; {
	int c;
	fprintf( stderr, "# " );
	while ( c = ReadChar(&instr) ) { fprintf( stderr, "%c", c ); }
	//fprintf( stderr, "\n" );
	return 0;
}

static int ParseInverseTransform(t, instr) T *t; char *instr; {
  int i;
  for (i = 0; i<6; i++) t->itransform[i] = ReadLong(&instr) / 100.0;
  return 0;
}

static int ParseEncoding(t, instr) T *t; char *instr; {
  /* Parse first line of QE directive. */
  // [ant] read encoding index (a document can have multiple encodings)
  int e = ReadInt(&instr);
  // [ant] read size of encoding table
  int n = ReadInt(&instr);
  int i;
  if (e<0) return PSTOTEXT_FILTER_BADENCODINGNUMBER;
  if (n>/*256*/1024) return PSTOTEXT_FILTER_TOOMANYGLYPHINDEXES;

  /* Grow "t->encodings" if necessary. */
  if (t->encodingsCount<=e) {
    int oldSize = t->encodingsCount;
    t->encodingsCount = 2*e;
    t->encodings = (EncodingTable *)realloc(
      (char *)t->encodings,
      t->encodingsCount * sizeof(Encoding)
    );
    for(i=oldSize;i<t->encodingsCount;i++)(*t->encodings)[i] = NULL;
  }

  /* If this is the first encoding numbered "e", allocate array. */
  if ((*t->encodings)[e] == NULL)
    (*t->encodings)[e] = (EncodingVector *)malloc(sizeof(EncodingVector));

  t->encoding_e = e; 
  t->encoding_n = n;

  return ParseEncodingMore( t, instr );
}


static int ParseEncodingMore(t, instr) T *t; char *instr; {
  /* Parse subsequent line of QE directive. */
  Encoding enc = (*t->encodings)[t->encoding_e];
  int i, tooSparse;

  // [ant] a glyph is "non-standard" if its value falls above the
  // encoding size

  for (i = 0; i < t->encoding_n; i++) {
    (*enc)[i] = (i<t->encoding_n) ? ReadInt(&instr) : NonstandardGlyph;
  }

  /* End of directive. */
  
  /* Some applications build the encoding vector incrementally.  If
     this one doesn't have at least the lower-case letters, we augment
     it with ISOLatin1. */
  for (i = 'a'; i<='z'; i++)
    tooSparse = (*enc)[i] == NonstandardGlyph;
  if (tooSparse)
    for (i = 0; i<256; i++)
      if ((*enc)[i] == NonstandardGlyph) (*enc)[i] = i;
  
  logEncoding( enc );

  return 0;
}

#define GuessAscend 0.9
#define GuessDescend -0.3

static void ReadPair(/*out*/ x, /*out*/ y, instr) double *x, *y; char **instr; {
  *x = ReadLong(instr) / 100.0;
  *y = ReadLong(instr) / 100.0;
}

static int ParseFont(t, instr) T *t; char *instr; {
  /* Parse QF directive. */
  int n = ReadInt(&instr), i;
  Metrics mt;
  Font f;
  double xmax, bly, topry;
  if (n<0) return PSTOTEXT_FILTER_BADFONTNUMBER;

  /* Grow "t->font" if necessary. */
  if (t->fontCapacity<=n) {
    int oldSize = t->fontCapacity;
    t->fontCapacity = 2*n;
    t->font = (FontTable *)realloc(
      (char *)t->font,
      t->fontCapacity * sizeof(Font)
    );
    for(i=oldSize;i<t->fontCapacity;i++)(*t->font)[i] = NULL;
  }

  /* If this is the first font numbered "n", allocate "FontRec". */
  if ((*t->font)[n] == NULL)
    (*t->font)[n] = (Font)malloc(sizeof(FontRec));

  f = (*t->font)[n];
  ReadPair(&f->x, &f->y, &instr);
  ReadPair(&f->xp, &f->yp, &instr);
  f->e = ReadInt(&instr);
  if ((*t->encodings)[f->e] == NULL) return PSTOTEXT_FILTER_BADENCODINGNUMBER;
  f->m = ReadInt(&instr);
  mt = (*t->metrics)[f->m];
  if (mt == NULL) return PSTOTEXT_FILTER_BADMETRICNUMBER;

  /* Transform height of font bounding box to reporting coordinates: */
  f->bx = f->x * mt->blx / 1000.0;
  f->by = f->yp * mt->bly / 1000.0;
  f->tx = f->x * mt->toprx / 1000.0;
  f->ty = f->yp * mt->topry / 1000.0;

  /* In some fonts produced by dvips, the FontBBox is incorrectly
     defined as [0 0 1 1].  We check for this, and apply the same
     heuristic used for an undefined FontBBox in "ParseMetrics".  */
  if (f->by - f->ty < 1.1) {
    xmax = 0.0;
    for (i = 0; i<256; i++)
      if (mt->chr[i].x > xmax) xmax = mt->chr[i].x;
      bly = GuessDescend * xmax; topry = GuessAscend * xmax;
      f->bx = f->x * bly / 1000.0;
      f->by = f->yp * bly / 1000.0;
      f->tx = f->x * topry / 1000.0;
      f->ty = f->yp * topry / 1000.0;
  }

  ZAP_BUFFER( f->fontName ); 
  ReadTrimmedString( &instr, f->fontName );

  return 0;
}

static int ParseMetrics(t, instr) T *t; char *instr; {
  /* Parse first line of QM directive. */
  int m = ReadInt(&instr), i;
  Metrics mt;

  if (m<0) return PSTOTEXT_FILTER_BADMETRICNUMBER;

  /* Grow "t->metrics" if necessary. */
  if (t->metricsSize<=m) {
    int oldSize = t->metricsSize;
    t->metricsSize = 2*m;
    t->metrics = (MetricsTable *)realloc(
      (char *)t->metrics,
      t->metricsSize * sizeof(Metrics)
    );
    for (i=oldSize;i<t->metricsSize;i++)(*t->metrics)[i] = NULL;
  }

  /* If this is the first metrics numbered "m", allocate "MetricsRec". */
  if ((*t->metrics)[m] == NULL)
    (*t->metrics)[m] = (Metrics)malloc(sizeof(MetricsRec));

  mt = (*t->metrics)[m];

  ReadPair(&mt->blx, &mt->bly, &instr);
  ReadPair(&mt->toprx, &mt->topry, &instr);

  t->metrics_m = m;

  return ParseMetricsMore( t, instr );

}

static int ParseMetricsMore(t, instr) T *t; char *instr; {
  /* Parse subsequent line of QM directive. */
  int i;
  Metrics mt = (*t->metrics)[t->metrics_m];

  for ( i = 0; i < 256; i++ )
    ReadPair(&mt->chr[i].x, &mt->chr[i].y, &instr);

  /* End of directive. */

  /* If "FontBBox" was not specified, take a guess. */
  if (mt->blx == 0.0 && mt->bly == 0.0 && mt->toprx == 0.0 && mt->topry == 0.0) {
    for (i = 0; i<256; i++)
      if (mt->chr[i].x > mt->toprx) mt->toprx = mt->chr[i].x;
    mt->bly = GuessDescend * mt->toprx;
    mt->topry = GuessAscend * mt->toprx;
  }

  return 0;
}

static void Itransform(t, x1, y1, x0, y0) T *t; double *x1, *y1, x0, y0; {
/* Set (*x1, *y1) to (t->itransform) * (x0, y0). */
  *x1 = t->itransform[0]*x0 + t->itransform[2]*y0 + t->itransform[4];
  *y1 = t->itransform[1]*x0 + t->itransform[3]*y0 + t->itransform[5];
}


/* ********************************************************
*/

static void religature_word_in_buffer( char *buf,
                                       size_t len ) {
  static int n = BUFSIZE;
  char word[MAX_WORD_LEN + 1];
  int start = 0;

  // skip leading punctuation (but don't ignore UnknownGlyph)
  while ( start < len && ( ispunct( buf[start] ) &&
                           buf[start] != UnknownChar ) ) { ++start; }

  // ignore trailing punctuation (but don't ignore UnknownGlyph)
  while  ( len > 0 && ( ispunct( buf[len - 1] ) &&
                        buf[len - 1] != UnknownChar ) ) { --len; }

  int end = start + len;
  strncpy( word, buf + start, len );
  word[len] = 0; // null terminate

  int chars_added = 0;
  if ( chars_added = religature_word( word, MAX_WORD_LEN ) ) {
    memmove( buf + end + chars_added, buf + end, n - end - chars_added );
    memcpy( buf + start, word, len + chars_added );
  }
}


/* Output the next word. t->buf is copied to t->word and our 'word'
   param is set point to t->word.  If a new line, page, or font has
   occurred, we also update the 'xmlOutputBuffer' param with a block of
   XML output.  Finally, t->outputBuffer is appended with the next
   word. */
/* TODO: this function does a lot; candidate for refactoring? */
static void OutputWord( T *t, 
                        char **word, 
                        char *xmlOutputBuffer, 
                        int *llx, 
                        int *lly, 
                        int *urx, 
                        int *ury)
{
  double x0, y0, x1, y1, x2, y2, x3, y3;
  long blx, bly, toprx, topry, mid_y;
  
  *word = NULL;

  Font f;

  f = (*t->font)[t->f];

  /* Compute the corners of the parallelogram with width "(t->x0,t->y0)"
     to "(t->x1,t->y1)" and height "(f.bx,f.by)" to "(f.tx,f.ty)". Then
     compute the bottom left corner and the top right corner of the
     bounding box (rectangle with sides parallel to the coordinate
     system) of this rectangle. */
  /* f->bx,by,tx,ty == height of font in reporting coordinates 
     t->x0,y0 t->x1,y1 == inital and final currentpoint
   */
  x0 = t->x0 + f->bx; y0 = t->y0 + f->by;
  x1 = t->x1 + f->bx; y1 = t->y1 + f->by;
  x2 = t->x0 + f->tx; y2 = t->y0 + f->ty;
  x3 = t->x1 + f->tx; y3 = t->y1 + f->ty;

  blx = ceil(MIN(MIN(MIN(x0, x1), x2), x3));
  bly = ceil(MAX(MAX(MAX(y0, y1), y2), y3)); /* *** should this be floor? PMcJ 981002 */
  toprx = floor(MAX(MAX(MAX(x0, x1), x2), x3));
  topry = floor(MIN(MIN(MIN(y0, y1), y2), y3)); /* *** should this be ceil? PMcJ 981002 */


  BOOLEAN newPage = FALSE;
  BOOLEAN newLine = FALSE;
  BOOLEAN newTextBox = FALSE;

  if (blx!=toprx && bly!=topry) {
    if (t->nonEmptyPage) {
      mid_y = (topry+bly) / 2;
      if (blx<toprx && topry<bly
          && t->blx <= blx
          && t->topry <= mid_y
          && mid_y <= t->bly) {
        newLine = FALSE;
        // perform a spacing test, to see if this new word is
        // "significantly" horizontally separated from the previous
        // word, which may indicate that we've started a new logical
        // block of text; this is motivated by our need to separate
        // author names that appear on the same line, are spaced
        // apart, and have no punctuation that delimits the authors
        

/*         // t->toprx is the previous word's right-edge, extended by a space (font bbox width) */
/*         int lastWordRightX = t->toprx - ( f->tx - f->bx ); */
/*         // blx is the current word's left-edge */
/*         int currWordLeftX = blx; */
/*         int spaceWidth = ( f->tx - f->bx ); */
/*         if ( currWordLeftX > t->x1_prev + spaceWidth * 2 ) { */
        int curr_interword_space = blx - t->x1_prev;
        if ( t->last_interword_space != 0 &&
             curr_interword_space > t->last_interword_space * 2.5 ) {
          newTextBox = TRUE;
        } else {
          t->last_interword_space = curr_interword_space;
        }
      } else {
        newLine = TRUE;
      }
    }
    else {
      newPage = TRUE;
      newLine = TRUE;
    }

    if ( ! TokenStackHasToken( t, PAGE_TOKEN ) ) {
      newPage = TRUE;
    }	

    if ( newLine==FALSE && ! TokenStackHasToken( t, LINE_TOKEN ) ) {
      newLine = TRUE;
    }	

    /* Output elements "0" through "t->lbuf-1" of "t->buf". */
    t->buf[t->lbuf] = '\0';
    if ( t->fixMissingLigatures ) {
      // TODO: first try fixing based upon entire word, which may be a
      // compound hyphenated phrase; second, try each part of the
      // compound phrase separately; third, try removing the hypen and
      // correcting the conjoined word (this is for words that are
      // hyphenated across a line break

/*       // don't attempt to fix ligatures of current word if it or its */
/*       // previous "word" was hyphenated */
/*       if ( t->buf[t->lbuf - 1] != '-' &&  */
/*            t->word[strlen( t->word ) - 1] != '-' ) { */
        religature_word_in_buffer( t->buf, t->lbuf );
/*       } */
    }
    // TODO: remove UnknownGlyph marker (???); now that we've called
    // religature_word_in_buffer(), do we *want* to keep any remaining
    // UnknownGlyph markers in the word?  Any word containing this
    // marker is almost certainly not a real word (since one or more
    // of its glyphs are missing), so even if we removed the ugly
    // marker, it wouldn't help downstream text analysis or display;
    // probably better to keep it in for verification purposes (i.e.,
    // whether pstotext is converting all words correctly)
    strcpy( t->word, t->buf );
    *word = t->word;

    t->nonEmptyPage = TRUE;
    t->blx = blx; t->bly = bly; t->toprx = toprx; t->topry = topry;

    /* transform device units to default PostScript units */
    Itransform( t, &x1, &y1, (double)blx, (double)bly);
    blx = floor(x1); bly = floor(y1);
    Itransform( t, &x1, &y1, (double)toprx, (double)topry);
    toprx = ceil(x1); topry = ceil(y1);

    if (blx < toprx) {
      *llx = blx; 
      *urx = toprx; 
    }
    else {
      *llx = toprx; 
      *urx = blx; 
    }

    if (bly < topry) {
      *lly = bly; 
      *ury = topry; 
    }
    else {
      *lly = topry; 
      *ury = bly; 
    }

  } /*if (blx!=toprx && bly!=topry) { */

  // compute current page bbox
  if ( t->pageLLX < 0.0 ) {
    t->pageLLX = *llx;
    t->pageLLY = *lly;
    t->pageURX = *urx;
    t->pageURY = *ury;
  }
  else {
    t->pageLLX = MIN( t->pageLLX, *llx );
    t->pageLLY = MIN( t->pageLLY, *lly );
    t->pageURX = MAX( t->pageURX, *urx );
    t->pageURY = MAX( t->pageURY, *ury );
  }

#define LOCAL_BUFFER_SIZE 512

  char localXmlBuffer[LOCAL_BUFFER_SIZE];

  int width = *urx - *llx;
  int height = *ury - *lly;

  ZAP_BUFFER( localXmlBuffer );

  BOOLEAN newFont = t->f != t->lastFont;

  // Add a space character to end of output and update bbox:
  // NOTE: this is adding a space to the end of every block of text, 
  //	which is meant to make character offsets within the output file 
  //	easier to calculate
  AppendToOutputBuffer( t, " " );

  if ( newPage ) {
    // process anything on the output stack
    UnwindAndProcessTokenStack( t, PAGE_TOKEN, localXmlBuffer );
    ClearOutputBuffer( t );
    PushTokenStack( t, PAGE_TOKEN );	
    sprintf( localXmlBuffer + strlen( localXmlBuffer ), 
             "\n<page n=\"%d\">", ++t->pagenum );

    PushTokenStack( t, LINE_TOKEN );
    sprintf( localXmlBuffer + strlen( localXmlBuffer ), 
             "\n<line>" );
    t->linenum = 0;
  }
  else if ( newLine ) {
    // process anything on the output stack
    UnwindAndProcessTokenStack( t, LINE_TOKEN, localXmlBuffer );
    ClearOutputBuffer( t );
    PushTokenStack( t, LINE_TOKEN );
    sprintf( localXmlBuffer + strlen( localXmlBuffer ), 
             "\n<line>" );
    ++t->linenum;
  }
  else if ( newFont || newTextBox ) {
    // process anything on the output stack
    UnwindAndProcessTokenStack( t, TBOX_TOKEN, localXmlBuffer );
    ClearOutputBuffer( t );
  }
  
  // if we've encountered a new page, a new line, or a new font then
  // we copy our block of XML output to the xmlOutputBuffer parameter
  if ( strlen( localXmlBuffer ) > 0 ) {
    strcpy( xmlOutputBuffer, localXmlBuffer );
  }

  if ( PeekTokenStack( t ) != TBOX_TOKEN ) {
    PushTokenStack( t, TBOX_TOKEN );

    PushIntArgStack( t, t->f );
    PushDblArgStack( t, *lly );
    PushDblArgStack( t, *llx );
    PushDblArgStack( t, *ury );
    PushDblArgStack( t, *urx );
  }
  else {
    PopDblArgStack( t );
    PopDblArgStack( t );
    PushDblArgStack( t, *ury );
    PushDblArgStack( t, *urx );
  }

  if ( *word != NULL ) {
    // appends t->outputBuffer
    AppendToOutputBuffer( t, *word );
  }
  
  t->lastFont = t->f;
  t->lbuf = 0;
}


/* *******************
 */
static BOOLEAN SameDirection(x0, y0, x1, y1) double x0, y0, x1, y1; {
  return y0 == 0.0 && y1 == 0.0 && x0*x1 > 0.0
      || x0 == 0.0 && x1 == 0.0 && y0*y1 > 0.0
      || x0 * y1 == x1 * y0;
}

/* *******************
 */
static double OverlapRatio(ax0, ax1, bx0, bx1)
  double ax0, ax1, bx0, bx1; {
  double overlap = 0.0;
  double totalLength, sharedLength;
  if ( ax0 <= bx0 ) {
    totalLength = bx1-ax0;
    sharedLength = ax1-bx0;
  }
  else {
    totalLength = ax1-bx0;
    sharedLength = bx1-ax0;
  }
  double lengthA = ax0<ax1? ax1-ax0 : ax0 - ax1;
  double lengthB = bx0<bx1? bx1-bx0 : bx0 - bx1;
  if ( totalLength < lengthA+lengthB ) {
    overlap = sharedLength / totalLength;
  }
  return overlap;
}	

/* *******************
 */
static double OverlapLength(ax0, ax1, bx0, bx1)
  double ax0, ax1, bx0, bx1; {
  double  sharedLength;
  if ( ax0 <= bx0 ) {
    sharedLength = ax1-bx0;
  }
  else {
    sharedLength = bx1-ax0;
  }
  return sharedLength;
}	

/* *******************
 */
static double SegmentLength(x0, x1)
  double x0, x1; {
  return x0<x1? x1-x0 : x0-x1;
}	

static void outputUnknownGlyph( char* buf, int *l, GlyphIndex glyph ) {
  buf[(*l)++] = UnknownChar;
  // the following commented block can be used to insert the decimal
  // representation of the glyph code; this can be useful for
  // investigative purposes
/*   char glyphDecRep[4]; */
/*   sprintf( glyphDecRep, "%d", glyph ); */
/*   memcpy( buf + (*l), glyphDecRep, strlen( glyphDecRep ) ); */
/*   *l += strlen( glyphDecRep ); */
/*   buf[(*l)++] = UnknownChar; */
}

static int ParseString(T* t, 
                       char* instr, 
                       char** word, 
                       char* xmlOutputBuffer, 
                       int* llx, 
                       int* lly, 
                       int* urx, 
                       int* ury)
{
  /* Parse QS directive. */
#define spaceTol 0.30 /* fraction of average character width to signal word break */
  char buf[MAX_WORD_LEN];
  int n, ch, i, j, charCode, l;
  Font f;
  Encoding enc;
  GlyphIndex glyphIndex;
  double x0, y0, x1, y1, xsp, ysp, dx, dy, maxx, maxy;
 
#define SetBuf() \
   do { \
   strncpy(t->buf, buf, l); \
   t->lbuf = l; \
   t->f = n; \
   t->x1_prev = t->x1; \
   t->x0 = x0; t->y0 = y0; t->x1 = x1; t->y1 = y1; \
   } while(0)

  n = ReadInt(&instr); /* index in "t->font" */
  //fprintf( stderr, "using font #%d\n", n );
  f = (*t->font)[n];
  if (f == NULL) return PSTOTEXT_FILTER_BADFONTNUMBER;
  enc = (*t->encodings)[f->e];
  if (enc==NULL) return PSTOTEXT_FILTER_BADENCODINGNUMBER;
/*   fprintf( stderr, "font=%d, encoding=%d\n", n, f->e ); // DEBUG */

  ReadPair(&x0, &y0, &instr); /* initial currentpoint */
  j = ReadInt(&instr); /* length of string */
  ch = ReadChar(&instr);
  if (ch != ' ')
    return PSTOTEXT_FILTER_BADQS;
 
  l = 0; // [ant] next buffer element to fill
  for (i = 0; i<=j-1; i++) {
    // read the raw character code
    charCode = ReadChar(&instr);  // [ant] value range is 0-255
    /* if (charCode=='\0') return PSTOTEXT_FILTER_BADQS; */ /* TeX uses '\0' */
    // using the current encoding, lookup the glyphIndex for this character code
    glyphIndex = (*enc)[charCode];
 
    /* If "glyphIndex==GlyphNotDefined", then "charCode" mapped to the glyph ".notdef".  This
       is usually a mistake, but we check for several known cases: */
    if (glyphIndex == GlyphNotDefined) {
 
      /* If any element of the current encoding is in the range used
         by Microsoft TrueType, assume this character is, too. */
      int k; BOOLEAN tt = FALSE;
      for(k = 0; !tt && k < sizeof(*enc)/sizeof((*enc)[0]); k++) {
        if (FirstTT1 <= (*enc)[k] && (*enc)[k] <= LastTT2) tt = TRUE;
      }
      if (tt) { 
        glyphIndex = FirstTT1 + (int)charCode;
      }
      /* There are too many other exceptions to actually trap this:
         else if (charCode == '\r') ; // Adobe Illustrator does this...
         else if (charCode == '\t') ; // MacDraw Pro does this...
         else if (charCode == '\032') ; // MS Word on Mac does this...
         else return PSTOTEXT_FILTER_BADGLYPHINDEX;
      */
    }
    /* ----- This next section is all special case decoding for glyph types -----*/
    if (glyphIndex == GlyphNotDefined || glyphIndex == NonstandardGlyph) {
      // [ant]: for an unknown/unspecified character encoding, we
      // revert back to the original PS character code, but how/why
      // can this be the right thing to do?
      glyphIndex = charCode;
    }

    if (glyphIndex == GlyphNotDefined) {
      logGlyph( charCode, glyphIndex, "glyph == GlyphNotDefined" );

/*       // [ant: added for pedagogical reasons] */
/*       buf[l++] = UnknownChar; */
/*       outputUnknownGlyph( buf, &l, glyphIndex ); */

      /* skip */;
    }
    else if ( glyphIndex <= LASTAsciiCodeLow ) { 
      // [ant] a small sampling of ps/pdf documents suggests that
      // missing ligatures fall in 10-14 range
      logGlyph( charCode, glyphIndex, "glyph <= LASTAsciiCodeLow" );
      outputUnknownGlyph( buf, &l, glyphIndex );

      /* skip */;
    }
    else if (glyphIndex < 128) {
      logGlyph( charCode, glyphIndex, "glyph < 128" );
      buf[l] = (char)glyphIndex;
      l++;
    }
    else if (glyphIndex <= LastISOLatin1) {
      logGlyph( charCode, glyphIndex, "!!! glyph <= LastISOLatin1" );
      buf[l] = (char)glyphIndex;
      l++;
      // ACS :: modify here: 
      // if ( 0 < glyphIndex && glyphIndex <= LASTAsciiCodeLow ) {
      //   char *str = AsciiCodesLow[glyphIndex];
      //   int lstr = strlen(str);
      //   strncpy(&buf[l], str, lstr);
      //   l += lstr;
      //   // buf[l] = (char)glyphIndex;
      // }
      // else {
      //   buf[l] = (char)glyphIndex;
      // }      
      // /* *** if (glyphIndex IN ISOLatin1Gaps) buf[l] = UnknownChar; */
      // l++;
    }		
    else if (glyphIndex <= LASTSpecialGlyphs) {
      logGlyph( charCode, glyphIndex, "!!! glyph <= LASTSpecialGlyphs" );
      char *str = SpecialGlyphs[glyphIndex-FIRSTSpecialGlyphs];
      int lstr = strlen(str);
      strncpy(&buf[l], str, lstr);
      l += lstr;
    }
    else if (glyphIndex <= LastDvips) {
      logGlyph( charCode, glyphIndex, "!!! glyph <= LastDvips" );
      char *str; int lstr; char tempstr[2];
      if (t->dvipsIsCork) {
	if (glyphIndex <= LASTCorkSpecialGlyphs)
	  str = CorkSpecialGlyphs[glyphIndex-FIRSTCorkSpecialGlyphs];
	else if (glyphIndex == FIRSTCorkSpecialGlyphs+0337)
	  str = "SS";
	else if (glyphIndex == FIRSTCorkSpecialGlyphs+0377)
	  str = "\337";
	else {
	  tempstr[0] = glyphIndex-FIRSTCorkSpecialGlyphs; tempstr[1] = '\0';
	  str = &tempstr[0];
	}
      }
      else if (glyphIndex <= LASTDvipsGlyphs) {
        /* Assume old text layout (OT1?). */
        str = DvipsGlyphs[glyphIndex-FIRSTDvipsGlyphs];
      } else {
        tempstr[0] = UnknownChar; tempstr[1] = '\0';
        str = &tempstr[0];
      }
      lstr = strlen(str);
      strncpy(&buf[l], str, lstr);
      l += lstr;
    }
    else if (glyphIndex <= LastTT2) {
      logGlyph( charCode, glyphIndex, "!!! glyph <= LastTT2" );
      if (FirstTT2 <= glyphIndex) glyphIndex -= FirstTT2-FirstTT1;
      if (glyphIndex < FirstTT1+32) {
        outputUnknownGlyph( buf, &l, glyphIndex );
        /* 	buf[l] = UnknownChar; l++; */
      }
      else if (glyphIndex < FIRSTTTSpecialGlyphs ||
               LASTTTSpecialGlyphs < glyphIndex) {
        buf[l] = (char)(glyphIndex - FirstTT1); l++;
      }
      else {
        char *str = TTSpecialGlyphs[glyphIndex-FIRSTTTSpecialGlyphs];
        int lstr = strlen(str);
        strncpy(&buf[l], str, lstr);
        l += lstr;
      }
    }
    else if (glyphIndex <= LastOldDvips) {
      logGlyph( charCode, glyphIndex, "!!! glyph <= LastOldDvips" );
      char *str = DvipsGlyphs[glyphIndex-FirstOldDvips];
      int lstr = strlen(str);
      strncpy(&buf[l], str, lstr);
      l += lstr;
    }
    else if (glyphIndex == NonstandardGlyph) { /* not in StandardGlyphs */
      logGlyph( charCode, glyphIndex, "!!! glyph <= NonstandardGlyph" );
      // [ant] is this code even reachable? can only get here if
      // 'charCode == NonstandardGlyph', but NonstandardGlyph is an
      // aribtrary value that we assign to out-of-range encoding
      // elements, and doesn't seem to be a value we would get
      // directly from a PS string
/*       buf[l] = UnknownChar; */
/*       l++; */
      outputUnknownGlyph( buf, &l, glyphIndex );
    }
    else {
      logGlyph( charCode, glyphIndex, "!!! PSTOTEXT_FILTER_BADGLYPHINDEX !!!" );
      return PSTOTEXT_FILTER_BADGLYPHINDEX;
    }
 
    /* ------- End glyphIndex cases -------- */

    fixUnprintableLatin1CharCodes( buf, &l );
 
    /* substitute minus for hyphen. */
    if (buf[l-1] == '\255') buf[l-1] = '-';

    /* insert space after a ']' char so that it is valid char in xml CDATA section */
    if ( l>3 && buf[l-3] == ']' && buf[l-2] == ']' && buf[l-1] == '>') {
      buf[l-1] = '}';
    }
    else if ( buf[l-1] == '>') {
      buf[l-1] = '}';
    }
  }
 
  ReadPair(&x1, &y1, &instr); /* final currentpoint */
/*   if ( x1 < x0 ) { */
/*     fprintf( stderr, "Warning: x1 < x0!  x0=%f, x1=%f\n", x0, x1 ); */
/*     SetBuf(); */
/*     return 0; */
/*   } */
/*   fprintf( stderr, "x0=%f\ty0=%f\tx1=%f\ty1=%f\n", x0, y0, x1, y1 ); */

  /* Determine if there should this string specifies a distinct word,
     relative to the previous string. */

  if (l != 0) { /* "l==0" e.g., when Adobe Illustrator outputs "\r" */
    if (t->lbuf == 0) {SetBuf();}
    else {
      /* If the distance between this string and the previous one is
	 less than "spaceTol" times the minimum of the average
	 character widths in the two strings, and the two strings
	 are in the same direction, then append this string to the
	 previous one.  Otherwise, output the previous string and
	 then save the current one.
 
	 Sometimes this string overlaps the previous string, e.g.,
	 when TeX is overprinting an accent over another character.
	 So we make a special case for this (but only handle the
	 left-to-right orientation). */
 
      /* Set "(xsp,ysp)" to the reporting space coordinates of the
	 minimum of the average width of the characters in this
	 string and the previous one. */
 
      xsp = MIN((t->x1 - t->x0) / t->lbuf, (x1-x0) / l);
      ysp = MIN((t->y1 - t->y0) / t->lbuf, (y1-y0) / l);
 
      // [ant] dx and dy hold horiz and vertical space between the
      // previous and the current string
      dx = x0 - t->x1;
      dy = y0 - t->y1;
      // [ant] I believe maxx and maxy are the spacing thresholds used
      // to determine whether two sequential strings comprise separate
      // words
      maxx = spaceTol * xsp;
      maxy = spaceTol * ysp;
/*       fprintf( stderr, "xsp=%0.2f\t%.*s (%0.2f)\t%.*s (%0.2f)\t(%0.2f, %0.2f)\n",  */
/*                xsp, */
/*                t->lbuf, t->buf, (t->x1 - t->x0) / t->lbuf, */
/*                l, buf, (x1 - x0) / l, */
/*                x0, x1 ); */
      // [ant] determine if the separation between words *below* the
      // split threshold; or if the next string is directly below and
      // its left x coord is within the horiz extents of the last word
      if (dx*dx + dy*dy < maxx*maxx + maxy*maxy || 
          ( t->y1 == y0 && 
            t->x0 <= t->x1 && 
            t->x0 <= x0 && 
            x0 <= t->x1 && 
            SameDirection( t->x1 - t->x0, t->y1 - t->y0, x1 - x0, y1 - y0 ) ) ) {
        if (t->lbuf+l >= sizeof(t->buf)) {
          // [ant] handle potential buffer overflow by outputing word
          // prematurely (quite rare, I would imagine)
          OutputWord(t, word, xmlOutputBuffer, llx, lly, urx, ury);
          SetBuf();
        }
        else {
          // join this string fragment to the previous one (build up a
          // word)
          strncpy(&t->buf[t->lbuf], buf, l);
          t->lbuf += l;
          t->x1 = x1; t->y1 = y1;
          /* *** Merge font bounding boxes? */
        }
      }
      else {
        OutputWord(t, word, xmlOutputBuffer, llx, lly, urx, ury);
        SetBuf();
      }
    }
  }
 
  return 0;
}	
   
static void fixUnprintableLatin1CharCodes( unsigned char *buf, int *len ) 
{
  int i;
  for ( i = 0; i < *len; ++i ) {
    if ( FirstUnprintableISOLatin1Code <= buf[i] &&
         buf[i] <= LastUnprintableISOLatin1Code ) {
      int newChar = UnprintableISOLatin1Glyphs[buf[i] - FirstUnprintableISOLatin1Code];
      // can we substitue a printable char for this unprintable ISO Latin1 char?
      if ( newChar == GlyphNotDefined ) {
#if PEDAGOGICAL_DEBUG_OUTPUT
        fprintf( stderr, "!!! removing unprintable ISOLatin1 char code %d\n", buf[i] );
#endif
        // nope! so we eliminate unprintable char
        memmove( buf + i, buf + i + 1, *len - i );
        --(*len);
      } else {
        // yup! change it!
#if PEDAGOGICAL_DEBUG_OUTPUT
        fprintf( stderr, "!!! fixing unprintable ISOLatin1 char code %d ==> %d '%c'\n", buf[i], newChar, newChar );
#endif
        buf[i] = newChar;
      }
    }
  }
}

int DLLEXPORT pstotextClose (instance, word) 
  void *instance;
  char **word;
{
  T *t = (T *)instance;
/*   t->lbuf = 0; */
/*   t->buf[t->lbuf] = '\0'; */
  int buf_len = 0;
  int buf_capacity = BUFSIZE;
  char *buf = (char*) malloc( buf_capacity + 1 );
  char fontBuf[BUFSIZE];

  char *header = "\n<fonts>";
  int header_len = strlen( header );
  char *footer = "\n</fonts>";
  int footer_len = strlen( footer );

  UnwindAndProcessTokenStack( t, NULL_TOKEN, buf );
  buf_len = strlen( buf );

  sprintf( buf + buf_len, "\n<fonts>" );
  buf_len += header_len;

  int i=0; 
  for (;i<t->fontCapacity;i++) {
    Font f = (*t->font)[i];
    if ( f != NULL ) {
      double fontITransBY = 0.0; 
      double fontITransTY = 0.0; 
      double dummy = 0.0;
      Itransform( t, &dummy, &fontITransTY, 0.0, f->ty );
      Itransform( t, &dummy, &fontITransBY, 0.0, f->by );
      double fontITHeight = fontITransTY - fontITransBY;

      int len = strlen( f->fontName );
      int j=0; 
      for (;j<len;j++) {
        if ( *(f->fontName+j) == '"' ) {
          *(f->fontName+j) = '~';
        }
      }
      
      sprintf( fontBuf, "\n<font name=\"%s\" h=\"%#.2f\" n=\"%d\"/>", f->fontName, fontITHeight, i );
      if ( buf_len + strlen( fontBuf ) > buf_capacity - footer_len ) {
        buf_capacity *= 2;
        buf = (char*) realloc( (char*) buf, 
                               buf_capacity + 1 );
        if ( buf == NULL ) {
          return PSTOTEXT_INIT_MALLOC;
        }
      }
      strcpy( buf + buf_len, fontBuf );
      buf_len += strlen( fontBuf );
    }
  }

  sprintf( buf + buf_len, footer );
  
  *word = buf;
  return 0;	
}

int DLLEXPORT pstotextFilter(void *instance, 
                             char* instr, 
                             char** wordPtr, 
                             char* xmlOutputBuffer, 
                             int* llx, 
                             int* lly, 
                             int* urx, 
                             int* ury)
{
  T *t = (T *)instance;
  int c;
  *wordPtr = NULL;
  do {c = ReadChar(&instr); if (c=='\0') return 0;} while (c!='Q');
  c = ReadChar(&instr);
  switch (c) {
  case '#': return ParseLogComment(t, instr);
  case 'I': return ParseInverseTransform(t, instr);
  case 'M': return ParseMetrics(t, instr);
  case 'E': return ParseEncoding(t, instr);
  case 'F': return ParseFont(t, instr);
  case 'S': return ParseString(t, instr, wordPtr, xmlOutputBuffer, llx, lly, urx, ury);

  case 'C':
  case 'P': /* copypage, showpage */
    /* If any QS directives have been encountered on this page,
       t->buf will be nonempty now. */
    if (t->lbuf > 0) {
      OutputWord(t, wordPtr, xmlOutputBuffer, llx, lly, urx, ury);
    }
    else {
      *wordPtr = "";
      *llx = 0; *lly = 0; *urx = 0; *ury = 0;
    }
    t->nonEmptyPage = FALSE;
    t->blx = t->bly = t->toprx = t->topry = 0;
    break;
  case 'Z': /* erasepage */ /* skip */ break;
  case '\0': return 0;
    /* default: skip */
  }
  return 0;
}
