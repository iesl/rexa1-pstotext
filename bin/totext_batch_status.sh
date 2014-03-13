# for reporting progress of totext_batch_status.sh
# nice to use as 'watch -n 60 totext_batch_status.sh'

gs_errors=$(( `cat *log | grep -c ghostscript` ))
ps_errors=$(( `cat *log | grep -c pstotext` ))
n=$(( `cat *log | wc -l` ))
echo n=$n
echo "ghostscript errors       = "$gs_errors $(( $gs_errors * 100 / $n ))% 
echo "pstotext internal errors = "$ps_errors $(( $ps_errors * 100 / $n ))% 
