#!/usr/bin/perl -w

# for TESTING totext (and pstotext) on a batch of input ps/pdf
# documents, which are specifed as a list of SHA hashes

use strict;
use Cwd qw( realpath );
use File::Copy qw( copy );
use File::Basename;

my $doc_hash_list_file = $ARGV[0] || "/m/vinci6/data2/atolopko/segmentation/pstotext_control/test_docs.txt";
my $output_dir = $ARGV[1] || "/m/vinci6/data2/atolopko/segmentation/pstotext_control";
my $log = $ARGV[2] || "$output_dir/run_totext.log";
my $totext_bin = "/m/vinci6/data2/atolopko/pstotext/bin/totext";
my $idftype_bin = "/m/vinci6/data2/atolopko/pstotext/bin/idftype";
my $pstotext_base = $ARGV[3] || "/m/vinci6/data2/atolopko/pstotext/pstotext.current";
my $pstotext_bin = "$pstotext_base/pstotext";
my $ligatures_dict = "$pstotext_base/ligatures.txt";
my $documents_dir = $ARGV[4] || "/m/vinci6/data2/atolopko/segmentation/textfiles";

unlink $log if -f $log;
open FH, "$doc_hash_list_file" or die;
DOC:
while ( <FH> ) {
  /((.)(.)(.)(.).*)/;
  my $dir_prefix = "$2/$3/$4/$5";
  my $doc_file = ( glob( "$documents_dir/$dir_prefix/$1*" ) )[0];
  my $dest_dir = "$output_dir/$dir_prefix";
  my $our_doc_file = "$dest_dir/" . basename( $doc_file );
  if ( -e $our_doc_file ) {
    printf "file '$our_doc_file' has already been processed by pstotext\n";
    next DOC;
  }

  system( "mkdir -p $dest_dir" ) >> 8 == 0 or die;

  my $orig_gzip_xml_file = ( glob( "$documents_dir/$dir_prefix/$1*.pstotext.xml.gz" ) )[0];
  my $our_orig_xml_file;
  if ( $orig_gzip_xml_file && -f $orig_gzip_xml_file ) {
    $our_orig_xml_file = $orig_gzip_xml_file;
    $our_orig_xml_file =~ s/xml\.gz$/orig.xml/;
    $our_orig_xml_file = "$dest_dir/" . basename( $our_orig_xml_file );
    # gunzip and copy the original pstotext xml output file to our directory
    my $result = system( "gunzip -c $orig_gzip_xml_file > $our_orig_xml_file" ) >> 8;
    warn "gunzip error $!\n" if $result;
  }

  # link the original ps/pdf to our directory
  system( "ln -s $doc_file $our_doc_file" );

  my ( undef, $new_doc_file ) =
   split /->/, `perl $idftype_bin --file $our_doc_file --verbose`;
  chomp $new_doc_file;
  my $status =
    system( "perl $totext_bin --file $new_doc_file --nogzip --timeout 60 --pstotext \"$pstotext_bin -ligatures $ligatures_dict\"  --log $log" ) >> 8;
  warn "$totext_bin returned status $status: $!\n" unless !$status;
}
close FH;
