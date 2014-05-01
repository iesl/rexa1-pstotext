#!/usr/bin/perl -w

# This script reads the DBLP XML database (across multiple XML files),
# extracts all distinct words from all PCDATA elements, and writes a
# sorted list to a file.

use strict;
use XML::Parser;

my $output_file = shift @ARGV || "dblp-words.txt";
my @input_xml_files = @ARGV;
@input_xml_files = glob( "x??" ) unless @input_xml_files;
my %tokens;

# generate a list of punctuation characters,
# excluding '-' and "'"
my @punctuation_subset;
for ( my $i = 32; $i < 256; ++$i ) {
  if ( chr( $i ) =~ /[[:punct:]]/ &&
       $i != ord( '-' ) &&
       $i != ord( '\'' ) ) {
    push @punctuation_subset, chr( $i )
  }
}
# '\' not considered a punctuation character, but we will treat it as
# such
push @punctuation_subset, '\\\\';
my $punctuation_chars = join( "", @punctuation_subset );


my $parser = XML::Parser->new( Handlers => { Start => \&start_handler,
                                             End =>   \&end_handler,
                                             Char =>  \&char_handler,
                                           },
                               # this option will read the external
                               # DTD and expand any entities it declares
                               ParseParamEnt => 1);


# parse the DBLP XML files
foreach my $input_xml_file ( @input_xml_files ) {
  print STDERR "processing $input_xml_file...\n";
  $parser->parsefile( $input_xml_file );
}

# write the sorted word list to a file
open WORDS, "> $output_file" or die;
print WORDS join( "\n", sort keys %tokens );
close WORDS;

exit 0;


##########################
### XML parse handlers ###
##########################

my $elt_str;

sub start_handler {
  my ( $expat, $elt ) = @_;
  $elt_str = "";
#  print "start $elt\n";
}

sub char_handler {
  my ( $expat, $str ) = @_;
  $elt_str .= $str;
}



sub end_handler {
  my ( $expat, $elt ) = @_;
  foreach my $token ( tokenize( $elt_str ) ) {
    $tokens{$token} = 1;
  }
}

sub tokenize {
  my $s = shift;
  chomp $s;
  # first, tokenize by splitting on all punctuation and whitespace,
  # except for "-" and "'"; we don't want to remove hyphens or dashes
  # that are between alpha characters; that is, we want to keep
  # possessive tokens such as "John's" and compound words such as
  # "object-oriented"; but we do not want the fragments of hyphenated
  # words (split across lines), or words flanked by apostrophes
  my @tokens1 = split /\s+|[$punctuation_chars]|^'|'$/, $s;
  my @tokens2;
  foreach my $token ( @tokens1 ) {
    if ( $token =~ /[-']/ ) {
      if ( !( $token =~ /^[-']|[-']$/ ) ) {
        push @tokens2, $token;
      } else {
        push @tokens2, split( /^[-']+|[-']+$/, $token );
      }
    } else {
      push @tokens2, $token;
    }
  }
  #remove empty and numeric-only tokens
  @tokens2 = grep( !/^$|\d+/, @tokens2 );
  # remove 2- and 3-letter words; such short words could not have
  # ligature problems, which is why we're creating this dictionary
  @tokens2 = grep { length( $_ ) > 3 } @tokens2;
  return @tokens2;
}
