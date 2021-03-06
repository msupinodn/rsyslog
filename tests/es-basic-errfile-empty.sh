#!/bin/bash
# This file is part of the rsyslog project, released under ASL 2.0
echo ===============================================================================
echo \[es-basic-errfile-empty\]: basic test for elasticsearch functionality
. $srcdir/diag.sh init
. $srcdir/diag.sh es-init
. $srcdir/diag.sh startup es-basic-errfile-empty.conf
. $srcdir/diag.sh injectmsg  0 10000
. $srcdir/diag.sh shutdown-when-empty
. $srcdir/diag.sh wait-shutdown 
. $srcdir/diag.sh es-getdata 10000
if [ -f rsyslog.errorfile ]
then
    echo "error: error file exists!"
    exit 1
fi
. $srcdir/diag.sh seq-check  0 9999
. $srcdir/diag.sh exit
