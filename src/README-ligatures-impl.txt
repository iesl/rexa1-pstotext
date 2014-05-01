Intially, I spent a good bit of time trying to wrap my head around
PostScript's character-to-glyph encoding system.  For many documents, it seems
that the necessary ligature-encoding information exists, because the
'ps2ascii' program can often recreate the ligatures that pstotext cannot
handle.  This is definitely worth looking into more, but I abandoned this
effort, seeing that I might not make any usable progress on this front, in the
time alotted.

So I ended up creating a hack for ligature corrections.  The
ligature-correction code operates by generating and remembering all
"deligatured" versions of words found in /usr/share/dict/words.  By this, I
mean that all words that could contain ligatures are "deligatured" and stored
in trie data structure.  Each deligatured word is mapped to exactly one "real"
word. (Note there are some cases where a deligatured word can originate from
multiple real words; when multiple possibilities exist, the word to be used is
arbitray, but a warning is written to STDERR indicating the choice.)  The
ligature-correction code then processes each word by seeing if the trie
contains the deligatured version, and if so, the real word is substituted in
its place.

When generating the set of deligatured words, if 2 ligatures exist in the same
range, the longest ligature is assumed.  For example, "affluent", which
arguably contains both "fl" and "ffl", will be deligaturized to "auent" and
not "aluent", which could, I suppose, might be worth checking for (but we
don't).  The longest ligature starting at a given position is preferred.

Currently, any deligatured words that collide with real words are eliminated
from consideration.  This avoids "false positives", such as "red" being
erroneously fixed to become "fired".  We could avoid this issue altogether if
we allow the pstotext to insert a special character-missing code into any word
for which a glyph encoding is not known.  I did not want to modify the code
that enable this, as the impact of the required modifications was not obvious
to me.  I can look into this further, however.

The generation of deligatured words is done each time pstotext is executed.
This adds between 1.5 seconds to execution time (on my 1.7Ghz Intel laptop).
To eliminate this overhead, we could consider modifying pstotext to run in a
batch mode.

Some additional comments about ligature corrections:

- Ligatures considered include: ff, fl, fi, ffl, ffi

- A deligatured word with an initial capitalized letter is handled properly
(capitalization is maintained).  Furthermore, since some words can begin with
a ligature, the capitalized and non-capitalized versions are handled
independently (consider that "fluffy" might show up as "uy", but that "Fluffy"
would should up as "Fluy", because "Fl" is not a ligature, while "fl" is).

- Hyphentated words (that are split across lines) are not corrected (neither the
prefix or suffix).  Doing so would introduce more erroneous corrections than
it would fix.  For example "ef-\ncient" *should* be corrected to
"ef-\nficient", but we don't go there. :)

- A hyphentated word that appears on a single line will only have its first
portion corrected.  ("uy-eects" would be changed to "fluffy-eects", not
"fluffy-effects").

- Trailing punctuation is preserved.  That is, if a word happens to have
trailing punctuation, the ligature correction is performed correctly (this
required some special handling).  For example, "Vericiation," would become
"Verification,".


---

Use pstotext's new "-ligature" command-line option to invoke missing ligature
corrections.

ligature.{c,h} contain nearly all of the ligature-correction code.

main.c has been updated to accommodate the new -ligature command-line option
(note: locale initialization code should be uncommented; it was causing errors
on my machine).

ptotdll.c has been updated mostly with new comments, some variable renamings
(in ParseString() for code readability), and the ligature correction code
hooks.

The Makefile has been updated appropriately (note: with compiler optimizations
off, debug info on)

I created a dev-util directory and placed existing and some new development
scripts there.

---

The code has been tested on only 6 documents.  It was time-consuming to find
appropriate documents, because many of the ps/pdf docs I found online actually
had their ligatures handled properly by pstotext, even when corresponding text
on rexa exhibited problems (in the title and abstract).  I attribute this to:
1) the pstotext code has been improved since rexa's data was last generated or
2) the ps/pdf documents I was retrieving were different versions than the one
processed by rexa.

The point is that the code works for these 6 documents only, and I will not
guarantee that it is segfault-free!  My testing strategy was ad hoc, as I did
not have time to generate a proper test suite.
