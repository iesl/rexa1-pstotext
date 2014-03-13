#!/usr/bin/perl -w

my %ligatures = ( "fl" => 1,
                  "fi" => 1,
                  "ff" => 1,
                  "ffi" => 1,
                  "ffl" => 1,
                );
my @ligatures = keys %ligatures;

while ( <> ) {
  chomp;
  my $word = $_;
  foreach my $ligature ( @ligatures ) {
    if ( $word =~ /$ligature/ ) {
      print "$word\n";
    }
  }
}
