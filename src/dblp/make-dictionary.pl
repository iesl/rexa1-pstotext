#!/usr/bin/perl -w

# combines multiple dictionary files (i.e., lists of words, 1 per
# line) and outputs a sorted, unique list of words

# example usage: "make-dictionary.pl /usr/share/dict/words dblp-words.txt"

use strict;

my %words;

while ( <> ) {
  chomp;
  $words{$_} = 1;
}

foreach my $word ( sort keys %words ) {
  print "$word\n";
}
