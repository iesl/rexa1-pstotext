[06/17/05]

/dev-util/convert-all.pl and /dev-util/convert.pl can be used to invoke
pstotext on a directory of ps/pdf files or a single ps/pdf file, respectively.
These scripts are basically intended to facilitate ad-hoc testing via manual
verification.  In particular, these scripts generate both ligature-corrected
and non-ligature-corrected versions of the processed documents.  This allows
the administrator (you!) to determine if ligature problems are being corrected
properly.  Note that output is converted to text (rather than being left in
the XML format that pstotext produces) to faciliate comparison (e.g., with
UNIX's diff command or emacs' ediff mode).

The lig-test-data directory contains a small set of files that have been (and
can be) used for manually verifying ligature-correction behavior.

For example:

$ perl dev-util/convert-all.pl lig-test-data
