/* Copyright (C) 1995, Digital Equipment Corporation.         */
/* All rights reserved.                                       */
/* See the file pstotext.txt for a full description.          */
/* Last modified on Fri Oct 11 15:35:24 PDT 1996 by mcjones   */

typedef char *BUNDLE[];

#ifdef NEED_PROTO
extern void putbundle(BUNDLE b, FILE *f);
#else
extern void putbundle(/* BUNDLE b, FILE *f */);
#endif
/* Write bundle "b" to file "f".

   "b" should have been constructed from "b.ps" by the ".ps.h" rule in
   the pstotext Makefile. */
