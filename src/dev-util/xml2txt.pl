#!/usr/bin/perl -w

while ( <> ) {
  while ( $_ =~ /\G.*\[CDATA\[(.*?)\]\]/gc ) {
    print "$1\n";
  }
}
