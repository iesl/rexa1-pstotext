#! /usr/bin/perl -w
use strict;

use IPC::Open3;
use FindBin qw($Bin $RealBin);
use Getopt::Long;
use Symbol qw(gensym);
use IO::File;

# $|=1;                           # autoflush

my $bin = $RealBin;
$bin =~ s!/$!!;

#======================= [ Forward Decls ] =======================
sub tryToExtractText($$$);
sub postToTextResults($$$;$);
sub getToTextTask($);
sub extractPsPdfText($$$);
sub printPaperTestMsg($$$$);
sub getFormattedPaperTestString($);

#======================= [ syntax ] =======================
sub syntax () {
  print <<EOS;
  totext
EOS
}


#======================= [  ] =======================

my %options=();
GetOptions(
           "file=s"        => \$options{file},
           "nogzip"        => \$options{nogzip},
           "pstotext:s"    => \$options{pstotext},
           "timeout=i"     => \$options{timeout},
           "debug"         => \$options{debug},
           "log=s"         => \$options{log},
           "logprefix=s"   => \$options{logprefix},
          );

## Globals: 
my $UNDETERMINED = 0;
my $ISAPAPER = 1;
my $NOTAPAPER = 2;
my $GHOSTSCRIPT_ERROR = 3;
my $PSTOTEXT_ERROR = 4;
my $PAGEMARKER_RE = qr/^<page/o;
my $GHOSTSCRIPT_ERROR_RE = qr/Ghostscript.*Unrecoverable error/o;
my $PSTOTEXT_ERROR_RE = qr/pstotext: internal error/o;

# the number of pages inspected when running the "is a paper" test
# 7 is the new magic number, because McCallum's thesis has its abstract on page 7  :)
# (previous threshold was 5)
my $ISAPAPER_PAGE_THRESHOLD = 7;

# number of seconds before we assume pstotext is never going to
# terminate (causes a paper to be logged as "possibly a paper")
my $DEFAULT_TIMEOUT = 30;


{
  #======================= [ Main ] =======================
  {
    my @tmpfiles = ();

    ## defaults: 
    $options{pstotext} = "$bin/pstotext -ligatures $bin/ligatures.txt" unless defined $options{pstotext};
    $options{timeout} = $DEFAULT_TIMEOUT unless defined $options{timeout};
    $options{log} = "totext.log" unless defined $options{log};
    $options{logprefix} = "" unless defined $options{logprefix};

    my $logFH;
    open( $logFH, ">>$options{log}" ) or die "can't open log: $! \n";

    my $filename = $options{file};

    if (-f $filename) {
      print "Extracting $filename\n";
      print $logFH $options{logprefix};
      my ($isAPaper, $errstr, $outputFilename) = tryToExtractText( $filename, \&simplePaperTester, $options{pstotext} );

      printPaperTestMsg( $logFH, $filename, $isAPaper, $errstr );
      printPaperTestMsg( *STDOUT, $filename, $isAPaper, $errstr ) if $options{debug};

      `gzip $outputFilename` unless $options{nogzip} || (! -f $outputFilename) ;
      $outputFilename = "$outputFilename.gz";
    } else {
      print STDERR "can't find file $filename\n";
    }
    print $logFH " " . gmtime, "\n";

    close( $logFH ); 
    print "\n";
  }
  

  #======================= [ printPaperTestMsg ] =======================
  sub printPaperTestMsg($$$$) {
    my ($oHandle, $filename, $isAPaper, $errstr) = @_;
    print $oHandle "$filename ", getFormattedPaperTestString( $isAPaper );
    print $oHandle " ($errstr)" if defined( $errstr );
  }

  #======================= [ getFormattedPaperTestString ] =======================
  sub getFormattedPaperTestString($) {
    my ($isAPaper) = @_;
    if ( defined $isAPaper ) {
      if ( $isAPaper == $GHOSTSCRIPT_ERROR ) {
        return "ghostscript failure";
      }
      if ( $isAPaper == $PSTOTEXT_ERROR ) {
        return "pstotext failure";
      }
    }
    my @terms = ();
    push( @terms, "is" );
    push( @terms, "possibly" ) if !defined( $isAPaper );
    push( @terms, "not" ) if defined( $isAPaper ) && $isAPaper eq $NOTAPAPER;
    push( @terms, "a paper" );
    return join " ", @terms;
  }



  #======================= [ tryToExtractText ] =======================
  sub tryToExtractText($$$)
    {
      my ($filename,$paperTester, $pstotext) = @_;
      my ($isAPaper, $msg, $outputFilename);
      $msg = "";
      my $timedOut = 1;
      my ($outfileHandle, $extractHandle);

      $outputFilename = "$filename.pstotext.xml";

      open( $outfileHandle, ">$outputFilename" ) or die "$!\n";

      ## reset the process group of this script so that we can kill it without affecting
      ##  the parent process
      setpgrp( 0, 0 ) or print "setpgid: $!\n" and die "setpgid: $!\n";

      eval {
        local $SIG{ALRM} = sub { die "timeout" };
        alarm $options{timeout}; # schedule alarm

        eval {
          print "running $pstotext $filename\n";
          open( $extractHandle, "$pstotext $filename 2> $filename.pstotext.stderr |" ) or die "$!";
          binmode( $outfileHandle, ":encoding(UTF-8)" );
          binmode( $extractHandle, ":encoding(UTF-8)" );
          ($isAPaper, $msg) = extractPsPdfText( $extractHandle, $outfileHandle, $paperTester );
          # if a pstotext/ghostscript error was detected in stderr,
          # change isAPaper code to the error code
          my $error_code = pstotextError( "$filename.pstotext.stderr" );
          $isAPaper = $error_code if $error_code;

          $timedOut = 0;
          alarm 0;              # cancel the alarm
        };
        alarm 0;                # cancel the alarm
      };

      alarm 0;                  # race condition protection
      if ( $timedOut eq 1 ) {
        $msg = "timed out at $options{timeout}s";
      }

      ## This will make sure that any child processes are killed (if they
      ##  won't exit gracefully)
      my $closeSuccess = 0;
      eval { 
        local $SIG{ALRM} = sub { die "timeout" };
        alarm 1;                # schedule alarm
 
        eval { 
          close $extractHandle;
          close $outfileHandle;
          $closeSuccess  = 1;
          alarm 0;
        };
        alarm 0;
      };
      alarm 0;
      if ($closeSuccess == 0 ) {
        local $SIG{HUP} = 'IGNORE';
        kill HUP => -$$;
      }
      return ($isAPaper, $msg, $outputFilename);
    }

  #======================= [ pstotextError ] =========================
  sub pstotextError($) {
    my $stderr_filename = shift;
    my $error_code = 0;
    if ( -f "$stderr_filename" ) {
      open ERR, "$stderr_filename" or die "'$stderr_filename' not found: $!\n";
      my @errtext = <ERR>;
      if ( @errtext ) {

        # foreach my $line ( map { "[stderr] $_"; } @errtext ) {
        #   print $line;
        # }

        my $errtext = join( "", @errtext );
        if ( $errtext =~ /$GHOSTSCRIPT_ERROR_RE/ ) {
          $error_code = $GHOSTSCRIPT_ERROR;
        } elsif ( $errtext =~ /$PSTOTEXT_ERROR_RE/ ) {
          $error_code = $PSTOTEXT_ERROR;
        }
      }
      close ERR;
    }
    return $error_code;
  }


  #======================= [ isAPaperTester ] =======================
  sub simplePaperTester($$)
    {
      my ($page, $line) = @_;

      my @regexList = ( qr/\[[iI\d\.\s]{0,3}abstract/io,
                        qr/\[[iI\d\.\s]{0,3}introduction/io,
                      );

      if ($page <= $ISAPAPER_PAGE_THRESHOLD) {
        foreach my $regex (@regexList) {
          if ($line =~ $regex) {
            return $ISAPAPER;
          }
        }
        return $UNDETERMINED;
      }
      return $NOTAPAPER;
    }

  #======================= [ trueIsAPaperTester ] =======================
  sub isAlwaysAPaperTester($$)
    {
      my ($page, $line) = @_;
      return $ISAPAPER;
    }


  #======================= [ extractPSPDFText ] =======================
  sub extractPsPdfText($$$)
    {
      my ($inputHandle, $outputHandle, $paperTester) = @_;

      my ($line, $page) = (0, 0);
      my $isAPaper = $UNDETERMINED;
      my $returnMsg;

      my $linecount = 0;

      while ($line=<$inputHandle>) {
        $linecount++;
        if ( $isAPaper==$UNDETERMINED ) {
          $page++ if $line =~ m/$PAGEMARKER_RE/;
          $isAPaper = &$paperTester( $page, $line );
          if ($isAPaper==$NOTAPAPER) {
            $returnMsg = "nixed by paper test (p=$page, l=$linecount)";
            goto PAPER_DONE;
          }
        }
        print $outputHandle $line;
      }

      if ($isAPaper==$UNDETERMINED) {
        $isAPaper=$NOTAPAPER;
        $returnMsg = "nixed by paper test  (p=$page, l=$linecount)";
      }

    PAPER_DONE:
      return ($isAPaper, $returnMsg);
    }
}
