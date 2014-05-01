#!/usr/bin/perl -w

# testing script that runs pstotext on all ps/pdf files in the
# specified directory; runs pstotext twice: once with ligature
# correction enabled, once with it disabled, allowing manual
# comparison of the effects; XML output of pstotext is converted to
# ASCII output; output files are written to same directory with ".txt"
# and ".lig.txt" suffixes.
#
# also supports regression testing, by comparing the converted output
# to a directory that contains the output of this script that was run
# at an earlier point in time; use the --regression-test-dir option to
# specify this directory

use strict;
use Getopt::Long;
use File::Basename;

my $usage = "usage: $0 [--regression-test-dir <dir>] {--document-directory <dir> | <dir>}\n";
my $doc_dir;
my $output_dir;
my $regression_test_dir;
my $fix_ligatures = 0;
my $gs_executable = "";
my $debug_pstotext;
my $pstotextRoot = dirname( $0 );

my $opt_result = GetOptions( "regression-test-dir|regression-dir|reg-dir=s" => \$regression_test_dir,
                             "directory|document-directory|doc-dir|docdir=s" => \$doc_dir,
                             "output-directory|out-directory=s" => \$output_dir,
                             "fix-ligatures|ligatures!" => \$fix_ligatures,
                             "gs|ghostscript=s" => \$gs_executable,
                             "debug-pstotext!" => \$debug_pstotext );
die "$usage" unless $opt_result;
$doc_dir = $ARGV[0] unless $doc_dir;
die "invalid directory in --document-directory option"
  unless -d $doc_dir;
die "invalid directory in --regression-test-dir option"
  unless !$regression_test_dir || -d $regression_test_dir;
my $convert_exec = "$pstotextRoot/convert.pl";

my @files_with_errors;
foreach my $file ( glob( "$doc_dir/*ps" ), glob( "$doc_dir/*pdf" ), glob( "$doc_dir/*document" ) ) {
  my $file_base = basename( $file );
  my $convert_result = system( "perl $convert_exec " .
                               ( $gs_executable ? "--ghostscript=$gs_executable " : "" ) .
                               ( $fix_ligatures ? "--fix-ligatures " : "" ) .
                               ( $debug_pstotext ? "--debug-pstotext " : "" ) .
                               ( $regression_test_dir ? "--regression-test-dir $regression_test_dir " : "" ) .
                               ( $output_dir ? "--output-file $output_dir/$file_base " : "" ) .
                               "--document $file" );
  push @files_with_errors, $file if $convert_result;
}

print STDERR "\nregression test failure summary:\n" if @files_with_errors;
foreach my $file_with_error ( @files_with_errors ) {
  print STDERR "\t$file_with_error\n";
}
exit ( @files_with_errors > 0 );

