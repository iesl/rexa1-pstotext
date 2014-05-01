### This is the Rexa version 1 pstotext, adapted from the DEC utility pstotext.

#### Installing

    Run bin/setup to recompile pstotext. This will compile pstotext and place the executable in
    bin/. The file 'ligatures.txt', also in the bin directory, should either be in the same
    directory as pstotext, or else specified as a parameter:

        pstotext -ligatures /path/to/ligatures.txt

#### Provided utilities:

  **pstotext**:

  Extract text and positional information from pdf

  Usage: pstotext input-file.pdf > output.xml

```
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
```


  **run-pstotext.sh**:

  Wrapper for pstotext.

  Usage: run-pstotext.sh --file test-data/test.pdf --nogzip -timeout 30 --debug

  * Runs pstotext with a few extra features:
    * Specifies a timeout (some pdfs can cause pstotext to hang indefinitely),
      and kills it if necessary.
    * Outputs the results to the specified file, and optionally zips the output file.
    * Runs a simple test to determine if that paper is likely to be a research
      paper, and outputs the result of the test
    * Creates log files with the results of the process

```
    Options:
        --file somefile.pdf
        --nogzip
        --pstotext  path to pstotext (if not on the standard path)
        --timeout   time allowed before kill pstotext subprocess
        --debug     print extra info to stdout
        --log       name of logfile
        --logprefix string that will be prepended to all logging output for this process
```

  **idftype**

   Try to guess the file type of an unknown file, then rename the file with an
   appropriate extension. If the file is compressed, uncompress and identify
   the newly expanded file.

   Usage: idftype -file unknown-file


