/* Copyright (C) 1995-1998, Digital Equipment Corporation.    */
/* All rights reserved.                                       */
/* See the file pstotext.txt for a full description.          */
/* Last modified on Fri Oct 16 16:27:54 PDT 1998 by mcjones   */
/*      modified on Thu Nov 16 13:33:13 PST 1995 by deutsch   */
/*
 * Modified on 27-MAY-1998 13:08 by Hunter Goatley
 *      Ported to VMS.  Various minor changes to allow it to work on
 *      both VAX and Alpha (VAX C and DEC C).  VMS pipes don't work
 *      right, so the GS output is dumped to a temporary file that's
 *      read, instead of reading from pipes (which is, of course, how
 *      VMS implements pipes anyway).  Also added -output option.
 */


#ifdef VMS
#include "vms.h"
#else
#include <sys/param.h>
#include <sys/wait.h>
#endif

// [acs added]
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#ifndef _LIBC
# define __set_errno(e) errno = (e)
# ifndef ENAMETOOLONG
#  define ENAMETOOLONG EINVAL
# endif
#endif
/* uClinux-2.0 has vfork, but Linux 2.0 doesn't */
#include <sys/syscall.h>
#if ! defined __NR_vfork
#define vfork fork	
#endif
// [end acs added]




#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <locale.h>
#include <wchar.h>

#include <string.h>
#include "bundle.h"
#include "ocr.h"
#include "rot270.h"
#include "rot90.h"
#include "ptotdll.h"

#define BOOLEAN int
#define FALSE 0
#define TRUE 1

#define LINELEN 2000 /* longest allowable line from gs ocr.ps output */

extern BUNDLE ocr, rot270, rot90;

static BOOLEAN cork = FALSE;
static BOOLEAN debug = FALSE;
static BOOLEAN fix_missing_ligatures = FALSE;
static char *gs_cmd = "gs";
static char *outfile = "";
static char *lig_dictionary = "ligatures.txt";

static char *cmd; /* = argv[0] */

static enum {
  portrait,
  landscape,
  landscapeOther} orientation = portrait;

static BOOLEAN bboxes = FALSE;

static int explicitFiles = 0; /* count of explicit file arguments */

FILE *popenZ (const char *command, const char *mode, int *pid);

usage() {
  fprintf(stderr, "pstotext 1.8g of 25 January 2000\n");
  fprintf(stderr, "Copyright (C) 1995-1998, Digital Equipment Corporation.\n");
  fprintf(stderr, "Modified by Ghostgum Software Pty Ltd for Ghostscript 6.0.\n");
  fprintf(stderr, "Comments to {mcjones,birrell}@pa.dec.com\n\n");
#ifdef VMS
  fprintf(stderr, "VMS Comments to goathunter@madgoat.com\n\n");
#endif
  fprintf(stderr, "Usage: %s [option|file]...\n", cmd);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -cork               assume Cork encoding for dvips output\n");
  fprintf(stderr, "  -landscape          rotate 270 degrees\n");
#ifdef VMS
  fprintf(stderr, "  -landscapeother     rotate 90 degrees\n");
#else
  fprintf(stderr, "  -landscapeOther     rotate 90 degrees\n");
#endif
  fprintf(stderr, "  -portrait           don't rotate (default)\n");
  fprintf(stderr, "  -bboxes             output one word per line with bounding box\n");
  fprintf(stderr, "  -ligatures file     *attempt* to fix words with missing ligatures\n");
  fprintf(stderr, "                      using the specified ligature dictionary\n");
  fprintf(stderr, "  -debug              show Ghostscript output and error messages\n");
  fprintf(stderr, "  -gs \"command\"       Ghostscript command\n");
  fprintf(stderr, "  -                   read from stdin (default if no files specified)\n");
  fprintf(stderr, "  -output file        output results to \"file\" (default is stdout)\n");
}

#ifdef VMS
#define OCRPATH "pstotext_dir:pstotext-ocr.ps"
#define ROT270PATH "pstotext_dir:pstotext-rot270.ps"
#define ROT90PATH "pstotext_dir:pstotext-rot90.ps"
#else
#define OCRPATH "/tmp/,pstotext-ocr.ps"
#define ROT270PATH "/tmp/,pstotext-rot270.ps"
#define ROT90PATH "/tmp/,pstotext-rot90.ps"
#endif

// #define paranoid_fwprintf( format, args... ) \
//   if ( fwprintf( format, ## args ) == -1 ) { \
//     fprintf( stderr, "fwprintf failed! (is LANG env var set to \"en_US\"?)\n" ); \
//     perror(cmd); \
//     exit( 1 ); \
//   }

//#define paranoid_fwprintf( format, args... ) \
//  if ( fwprintf( format, ## args ) == -1 ) { \
//    fprintf( stderr, "fwprintf failed! (is LANG env var set to \"en_US\"?)\n" ); \
//  }

// #define paranoid_fwprintf( format, args... ) \
//   if ( fwprintf( format, ## args ) == -1 ) { \
//     fprintf( stderr, "fwprintf failed! (is LANG env var set to \"en_US\"?)\n" ); \
//   }

#define paranoid_fwprintf( format, args... ) fwprintf( format, ## args )

static char *make_temp(BUNDLE b) {
  /* Return pathname of temporary file containing bundle "b".  Caller
     should unlink file (and, technically, free pathname). */
  FILE *f;
  char *tmpFileTemplate;
#ifdef VMS
  //char *path = tempnam("SYS$SCRATCH:", ".ps2t");
  tmpFileTemplate = strdup( "SYS$SCRATCH:ps2tXXXXXX" );
#else
  tmpFileTemplate = strdup( "/tmp/ps2tXXXXXX" );
#endif
  int fd = mkstemp( tmpFileTemplate );
  if ( fd == -1 ) { perror(cmd); exit(1); }
  f = fdopen(fd, "w");
  if (f==NULL) {perror(cmd); exit(1);} 
  putbundle(b, f);
  fclose(f);
  return tmpFileTemplate;
}

static char *ocr_path = NULL, *rotate_path = NULL;
static FILE *gs = NULL;
static int gs_pid = 0;

static void *instance; /* pstotext state */
#ifdef VMS
static char *cmdfile = NULL, *gsoutfile = NULL;
#endif

static BOOLEAN cleaned = FALSE;

static void nullHandler(int x) {
  fprintf(stderr, "\n warning: unhandled sig %d", x);
}

static void sig11Handler(int x) {
  signal(SIGSEGV, nullHandler);
  fprintf(stderr, "\n warning: sig %d", x);
  if ( cleaned != TRUE ) {
    sleep( 2 );
    if ( cleaned != TRUE ) {
      fprintf(stderr, "\n warning: dirty exit()\n");
    }
  }
  exit(1);
}

///// 
//
static int cleanup() {
  //fprintf(stderr, "\n trace: cleanup()");
  int gsstatus, status = 0;
  signal(SIGINT, nullHandler);
  signal(SIGHUP, nullHandler);
  signal(SIGPIPE, nullHandler);
  signal(SIGALRM, nullHandler);
  signal(SIGTERM, nullHandler);
  signal(SIGBUS, nullHandler);
  signal(SIGSEGV, sig11Handler);

  if (gs!=NULL) {
#ifdef VMS
    gsstatus = fclose(gs);
#else
    gsstatus = pcloseZ(gs, gs_pid);
    gs_pid = 0;
#endif
    if (WIFEXITED(gsstatus)) {
      if (WEXITSTATUS(gsstatus)!=0) status = 3;
      else if (WIFSIGNALED(gsstatus)) status = 4;
    }
  }

  pstotextExit(instance);

  if (rotate_path!=NULL) {
    if (strcmp(rotate_path, "")!=0) {
      unlink(rotate_path);
    }
    free( rotate_path );
    rotate_path=NULL;
  }
  if (ocr_path!=NULL) {
    unlink(ocr_path);
    free( ocr_path );
    ocr_path=NULL;
  }
#ifdef VMS
  if (cmdfile!=NULL) unlink(cmdfile);  
  if (gsoutfile!=NULL) unlink(gsoutfile);
#endif

  cleaned = TRUE;
  return status;
}

static void handler(int x) {
  fprintf(stderr, "\n warning: got sig %d", x);
  int status = cleanup();
  fprintf(stderr, "\n warning: cleanup status=%d", status);
  if (status!=0) exit(status);
#ifdef VMS
  exit(1);
#else
  exit(2);
#endif
}

static do_it(path) char *path; {
  /* If "path" is NULL, then "stdin" should be processed. */
  char gs_cmdline[2*MAXPATHLEN];
  char input[MAXPATHLEN];
  int status;
  FILE *fileout;
#ifdef VMS
  FILE *cfile;
#endif

  fileout = stdout;
  if (strlen(outfile) != 0) {
#ifdef VMS
     fileout = fopen(outfile, "w", "rfm=var","rat=cr");
#else
     fileout = fopen(outfile, "w");
#endif /* VMS */
     if (fileout == NULL) {perror(cmd); exit(1);}
  }

  signal(SIGINT, handler);
  signal(SIGHUP, handler);
  signal(SIGPIPE, handler);
  signal(SIGALRM, handler);
  signal(SIGTERM, handler);
  signal(SIGBUS, handler);
  signal(SIGSEGV, handler);

  ocr_path = make_temp(ocr);

  switch (orientation) {
  case portrait: rotate_path = NULL; break;
  case landscape: rotate_path = make_temp(rot270); break;
  case landscapeOther: rotate_path = make_temp(rot90); break;
  }

  if (path==NULL) strcpy(input, "-_");
  else {strcpy(input, "-- "); strcat(input, path);}

  sprintf(
    gs_cmdline,
#ifdef VMS
    "%s -r72 \"-dNODISPLAY\" \"-dFIXEDMEDIA\" \"-dDELAYBIND\" \"-dWRITESYSTEMDICT\" %s \"-dNOPAUSE\" %s %s %s",
#else
    "%s -r72 -dNODISPLAY -dFIXEDMEDIA -dDELAYBIND -dWRITESYSTEMDICT -dBATCH -dSAFER %s -dNOPAUSE %s %s %s",
#endif
    gs_cmd,
    (debug ? "" : "-q"),
    rotate_path == NULL ? "" : rotate_path,
    ocr_path == NULL ? "" : ocr_path,
    input
    );
  if (debug) fprintf(stderr, "%s\n", gs_cmdline);
#ifdef VMS
  cmdfile = tempnam("SYS$SCRATCH:","PS2TGS");
  gsoutfile = tempnam("SYS$SCRATCH:","GSRES");
  if ((cfile = fopen(cmdfile,"w")) == NULL) {perror(cmd);exit(1);}
  fprintf (cfile, "$ define/user sys$output %s\n", gsoutfile);
  fprintf (cfile, "$ %s\n", gs_cmdline);
  fprintf (cfile, "$ deletee/nolog %s;*\n", cmdfile);
  fputs ("$ exit\n", cfile);
  fclose (cfile);
  sprintf(gs_cmdline, "@%s.", cmdfile);
  system(gs_cmdline);
  if ((gs = fopen(gsoutfile, "r")) == NULL) {
	fprintf(stderr, "Error opening output file %s from GS command\n", gsoutfile);
	perror(cmd);
	exit(1);
  }
#else
  gs = popenZ(gs_cmdline, "r", &gs_pid);
  if (gs==0) {perror(cmd); exit(1);}
#endif
  status = pstotextInit(&instance);
  if (status!=0) {
    fprintf(stderr, "%s: internal error %d\n", cmd, status);
    exit(5);
  }
  if (cork) {
    status = pstotextSetCork(instance, TRUE);
    if (status!=0) {
      fprintf(stderr, "%s: internal error %d\n", cmd, status);
      exit(5);
    }
  }
  if (fix_missing_ligatures) {
    status = pstotextSetFixMissingLigatures(instance, TRUE, lig_dictionary);
    if (status!=0) {
      fprintf(stderr, "%s: internal error %d\n", cmd, status);
      exit(5);
    }
  }

  paranoid_fwprintf( fileout, L"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<document>" );

#define ZAP_BUFFER(ZZBUF) do { int i=0; for(;i<sizeof(ZZBUF);i++) (ZZBUF)[i]=0; } while(0)

  char lookahead_line[LINELEN];
  char *lookahead_line_available = fgets( lookahead_line, LINELEN, gs );
  char q_cmd_buffer[10000];
  int q_cmd_buffer_len = 0;
  while ( lookahead_line_available ) {
/*     char line[LINELEN]; */
    char xmlOutputBuffer[LINELEN];

    ZAP_BUFFER( xmlOutputBuffer );

    // char *pre, *word, *post;
    char *word;
    int llx, lly, urx, ury;

    q_cmd_buffer[0] = 0; // reset
    q_cmd_buffer_len = 0; // reset;
    do {
      int line_len = strlen( lookahead_line );
      q_cmd_buffer_len += line_len;

      strncat( q_cmd_buffer, lookahead_line, line_len + 1 );

      // remove trailing newline
/*       if ( q_cmd_buffer[q_cmd_buffer_len - 1] == '\n' ) { */
/*         q_cmd_buffer[q_cmd_buffer_len - 1 ] = ' '; */
/*       } */
      if ( debug ) { fputs(lookahead_line, stderr); }
    } while ( ( lookahead_line_available = fgets( lookahead_line, LINELEN, gs ) ) &&
              lookahead_line[0] != 'Q' );

    // [ant] read in a word; 'word' will be populated with the most
    // recent string that was output by the postscript; if a line
    // break, page break, or new font has occurred (see
    // ptotdll.c:Output()), then 'xmlOutputBuffer' will hold the XML
    // output for the previous line
    status = pstotextFilter( instance, q_cmd_buffer, &word, xmlOutputBuffer, &llx, &lly, &urx, &ury );
    if (status!=0) {
      fprintf(stderr, "%s: internal error %d\n", cmd, status);
      exit(5);
    }
    if (word!=NULL) {
      if (!bboxes) { // bboxes is set via -bboxes command-line option
        // fputs(pre, fileout); fputs(word, fileout); fputs(post, fileout);
	if ( xmlOutputBuffer != NULL ) {
	  // fputs( xmlOutputBuffer, fileout );	  
	  paranoid_fwprintf(fileout, L"%s", xmlOutputBuffer);
	}
        if ( debug ) fputc('\n', stderr);
      }
      else {
        paranoid_fwprintf(fileout, L"%6d\t%6d\t%6d\t%6d\t%s\n", llx, lly, urx, ury, word);
      }
    }
  }

  char *closing;

  status = pstotextClose( instance, &closing );
  if (status!=0) {
    fprintf(stderr, "%s: internal error (close) %d\n", cmd, status);
    exit(5);
  }
  if (closing!=NULL) {
    if (!bboxes) {
      // fputs(closing,fileout);
      paranoid_fwprintf(fileout, L"%s", closing);
    }
    free( closing );
  }

  paranoid_fwprintf( fileout, L"\n</document>" );

  if (fileout != stdout) fclose(fileout);
  status = cleanup();
  if (status!=0) exit(status);
}

//// 
// adapted pipe-open that returns the forked pid 
FILE *popenZ (const char *command, const char *mode, int *pid)
{
  FILE *fp;
  int pipe_fd[2];
  int reading;
  int pr, pnr;

  reading = (mode[0] == 'r');
  if ((!reading && (mode[0] != 'w')) || mode[1]) {
    __set_errno(EINVAL);			/* Invalid mode arg. */
  } else if (pipe(pipe_fd) == 0) {
    pr = pipe_fd[reading];
    pnr = pipe_fd[1-reading];
    if ((fp = fdopen(pnr, mode)) != NULL) {
      if ((*pid = vfork()) == 0) {	/* vfork -- child */
	close(pnr);
	if (pr != reading) {
	  close(reading);
	  dup2(pr, reading);
	  close(pr);
	}
	execl("/bin/sh", "sh", "-c", command, (char *) 0);
	_exit(255);		/* execl failed! */
      } else {			/* vfork -- parent or failed */
	close(pr);
	if (*pid > 0) {	/* vfork -- parent */
	  return fp;
	} else {		/* vfork -- failed! */
	  fclose(fp);
	}
      }
    } else {				/* fdopen failed */
      close(pr);
      close(pnr);
    }
  }
  return NULL;
}

// adapted pipe-close that kills (SIGINT) the subprocess
int pcloseZ(FILE *fd, int pid)
{
  int waitstat;
  
/*   fprintf(stderr, "\n trace: point 0"); */
  if (fclose(fd) != 0) {
/*     fprintf(stderr, "\n trace: point EOF"); */
    return EOF;
  }
/*   fprintf(stderr, "\n trace: point pre-kill"); */
  kill( gs_pid, SIGINT );
/*   fprintf(stderr, "\n trace: point pre-wait"); */
  if (wait(&waitstat) == -1) {
/*     fprintf(stderr, "\n trace: point of -1 return"); */
    return -1;
  }
/*   fprintf(stderr, "\n trace: point of normal return"); */
  return waitstat;
}

main(argc, argv) int argc; char *argv[]; {
  int i;
  char *arg;

  // set all locale categories to the LANG environment variable
  // this allows wide-character output to work properly (fwprintf)
  setlocale( LC_ALL, "en_US.utf8" );

  cmd = argv[0];
  for (i = 1; i<argc; i++) {
    arg = argv[i];
    if (strcasecmp(arg, "-landscape")==0) orientation = landscape;
    else if (strcasecmp(arg, "-cork")==0) cork = TRUE;
    else if (strcasecmp(arg, "-landscapeOther")==0) orientation = landscapeOther;
    else if (strcasecmp(arg, "-portrait")==0) orientation = portrait;
    else if (strcasecmp(arg, "-bboxes")==0) bboxes = TRUE;
    else if (strcasecmp(arg, "-debug")==0) debug = TRUE;
    else if (strcasecmp(arg, "-gs")==0) {
      i++;
      if (i>=argc) {usage(); exit(1);}
      gs_cmd = argv[i];
    }
    else if (strcasecmp(arg, "-output")==0) {
      i++;
      if (i>=argc) {usage(); exit(1);}
      outfile = argv[i];
    }
    else if (strcasecmp(arg, "-ligatures")==0) {
      fix_missing_ligatures = TRUE;
      i++;
      if (i>=argc) {usage(); exit(1);}
      lig_dictionary = argv[i];
    }
    else if (strcmp(arg, "-")==0) do_it(NULL);
    else if (arg[0] == '-') {usage(); exit(1);}
    else /* file */ {
      explicitFiles++;
      do_it(arg);
    }
  }
  if (explicitFiles==0) do_it(NULL);
  exit(0);
}
