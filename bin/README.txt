Notes from Gavin on this part of the pipeline:

Run ./setup to recompile our hacked version of pstotext.

Starting with a directory tree of downloaded PDF/PS files:
 (e.g.   A/B/C/D/ABCDEF0123456etc.flag.1.document )

1. Run idftype on each of those; it will figure out the
file's type, and rename the files to *.pdf, *.ps, or *.html.
Example:
 find DOWNLOAD_DIRECTORY -type f -exec PATH_TO_TREE/pstotext/bin/idftype -v -file {} \;
PRODUCES: A/B/C/D/ABCDEF0123456etc.pdf, etc.

2. Next step is to run totext (which runs pstotext):

Example:
 find DOWNLOAD_DIRECTORY -name \*.pdf -exec PATH_TO_TREE/pstotext/bin/totext -nogzip -file {} \;
 find DOWNLOAD_DIRECTORY -name \*.ps -exec PATH_TO_TREE/pstotext/bin/totext -nogzip -file {} \;
PRODUCES:
 totext.log  : Records totext success/failure.
 A/B/C/D/ABCDEF0123456etc.pdf.pstotext.xml  :  Parsed file, ready for next stage of pipeline (if pstotext succeeds)
 A/B/C/D/ABCDEF0123456etc.pdf.stderr : stderr output from pstotext

NOTE: 'jasper', Gavin's Linux PC, can totext about 1,000 papers per hour.
This all parallelizes nicely, and can be run on the grid
engine (see sgeRunPstotext script).


