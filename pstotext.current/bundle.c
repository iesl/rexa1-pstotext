/* Copyright (C) 1995, Digital Equipment Corporation.         */
/* All rights reserved.                                       */
/* See the file pstotext.txt for a full description.          */
/* Last modified on Thu Aug  1 11:32:09 PDT 1996 by mcjones   */

#include <stdio.h>
#include "bundle.h"

void putbundle(b, f) BUNDLE b; FILE *f; {
  char **ppLine = b;
  for (ppLine = b; *ppLine!=NULL; ppLine++) {
    fputs(*ppLine, f);
  }
}
