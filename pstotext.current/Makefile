# Copyright (C) 1995, Digital Equipment Corporation.
# All rights reserved.
# See the file pstotext.txt for a full description.
# Last modified on Wed Oct 28 08:45:54 PST 1998 by mcjones

# pstotext now requires an ANSI-compatible C compiler, such as gcc.
# If you absolutely must use a pre-ANSI compiler, you can try
# commenting out the includes of ptotdll.h in ptotdll.c and main.c.
# PMcJ 6 Sep 96

CC=gcc -O 
## CC=gcc -g
## CC=gcc -ggdb


BUNDLE = ocr.h rot270.h rot90.h

all:	clean pstotext generate-ligatures

trie.o: trie.c trie.h

ligature.o: trie.h

ptotdll.o: ptotdll.c ptotdll.h ligature.h

main.o: main.c ptotdll.h ligature.h bundle.h $(BUNDLE)

pstotext: bundle.o main.o ptotdll.o ligature.o trie.o
	$(CC) -o pstotext $^ -lm

# target to generate a file of ligatures
ligatures: generate-ligatures
	perl dblp/extract-dblp-words.pl dblp-words.txt dblp/x??
	perl dblp/make-dictionary.pl dblp-words.txt /usr/share/dict/words > dictionary.txt
	./generate-ligatures dictionary.txt ligatures.txt 2>&1 | tee ligatures.log

# target to make the executable that generates ligatures
generate-ligatures: generate-ligatures.o ligature.o trie.o
	$(CC) -o $@ $^

trie-test: trie-test.o trie.o 
	$(CC) -o $@ $^



.SUFFIXES: .ps

.c.o:
	$(CC) -c $*.c

# "Bundle" an Ascii file.
.ps.h:
	echo "char *$*[] = {" > $*.h
	sed -e 's/"/\\"/g' -e 's/\(.*\)/  "\1\\n",/' $*.ps >> $*.h
	echo "  0" >> $*.h
	echo "};" >> $*.h

tidy:
	rm -f ,* .,* .emacs_[0-9]* core *~ 


clean:
	rm -f pstotext *.o core $(BUNDLE)

check-syntax :
	$(CC) -c ${CHK_SOURCES}
