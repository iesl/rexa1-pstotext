[06/17/05]

pstotext can be asked to correct words that have missing ligatures ("ff",
"fl", "fi", "ffi", "ffl") via the -ligatures command-line option.  The
-ligatures option takes a file name that must be a "ligature dictionary".
This file contains lines of tab-separated pairs of words, where the first word
is the "de-ligatured" word (i.e., it is missing ligatures) and the
"re-ligatured" word (i.e., the correct spelling of the word).  

(See ligature-writeup.txt for details on how ligatures are corrected.)

This ligature dictionary file is created by running the generate-ligatures
program.  This program requires a dictionary input file, which simply contains
a "universal" list of words.  generate-ligatures will determine the
deligatured version of each word and create a ligature dictionary file (e.g.,
ligatures.txt), which can used with pstotext's -ligatures option.

Currently (as of 06/17/05!), our universal word list is a combination of
/usr/share/dict/words and all of the words found in the DBLP database.
dblp/extract-dblp-words.pl is used for this purpose.  It takes an XML version
of DBLP and outputs a file ("dblp-words.txt", by default) containing a
distinct list of words from DBLP.  dblp/make-dictionary.pl is then used to
combine dblp-words.txt and /usr/share/dict/words to form a single input file
that can be fed to generate-ligatures.

Note that the Makefile's "ligatures" target performs all of these steps
automatically.  That is, it extracts words from a DBLP database (but *you*
must first make sure the DBLP XML files are in place!), combines DBLP words
with /usr/share/dict/words, and then generates the ligatures.txt file by
running generate-ligatures.
