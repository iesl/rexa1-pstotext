This is the Rexa version 1 pstotext, adapted from the DEC utility pstotext.
===

Installing
====

    Run bin/setup to recompile our hacked version of pstotext. This will compile pstotext and place
    the executable in bin/. The file 'ligatures.txt', also in the bin directory, should either be in
    the same directory as pstotext, or else specified as a parameter:
        pstotext -ligatures /path/to/ligatures.txt

Provided utilities:
====

  *pstotext*: extract text and positional information from pdf

    Basic usage: pstotext input-file.pdf > output.xml

        pstotext --help: 
          Usage: bin/pstotext [option|file]...
          Options:
            -cork               assume Cork encoding for dvips output
            -landscape          rotate 270 degrees
            -landscapeOther     rotate 90 degrees
            -portrait           don't rotate (default)
            -bboxes             output one word per line with bounding box
            -ligatures file     *attempt* to fix words with missing ligatures
                                using the specified ligature dictionary
            -debug              show Ghostscript output and error messages
            -gs "command"       Ghostscript command
            -                   read from stdin (default if no files specified)
            -output file        output results to "file" (default is stdout)


  *run-pstotext.sh*
    wrapper for pstotext


  *idftype*

    Run idftype on each of those; it will figure out the file's type, and rename the files to *.pdf, *.ps, or *.html.
    Example:
        find DOWNLOAD_DIRECTORY -type f -exec PATH_TO_TREE/pstotext/bin/idftype -v -file {} \;
    PRODUCES: A/B/C/D/ABCDEF0123456etc.pdf, etc.







