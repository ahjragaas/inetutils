#!/bin/sh

# Copyright (C) 2022 Free Software Foundation, Inc.
#
# This file is part of GNU Inetutils.
#
# GNU Inetutils is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
#
# GNU Inetutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see `http://www.gnu.org/licenses/'.

# Regression tests for the FTP client "ftp".

# Set tool variables (e.g., SED).
. ./tools.sh

# Handle VERBOSE environment variable.
if test -z "${VERBOSE+set}"; then
    silence=:
    bucket='>/dev/null'
fi
if test -n "$VERBOSE"; then
    set -x
fi

# This script tests and thus requires the FTP client
FTP="${FTP:-../ftp/ftp$EXEEXT}"
if [ ! -x $FTP ]; then
    echo "No FTP client '$FTP' present.  Skipping test" >&2
    exit 77
fi

# Print version of FTP client in VERBOSE mode.
if [ "$VERBOSE" ]; then
    "$FTP" --version | $SED '1q'
fi

# Initialize test statistics.
SUCCESSES=0
EFFORTS=0
RESULT=0

# Check regression of crash reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2021-12/msg00003.html
# This bug is caused by signed integer overflow and insufficient bounds
# checking when using this integer for indexing into an array.
# First, use the original reproducer from the problem report.
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash bug from message 2021-12/03...' >&2
"$FTP" < "$srcdir"/crash-ftp-msg2021-12_03.bin >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of ftp crash bug from message 2021-12/03.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Input from message 2021-12/03 did not crash ftp.' >&2
fi
# Second, use a simple reproducer for systems with 32 bit integers.
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash bug from 32 bit integer overflow...' >&2
tell='macdef x
$2147483648

$ x'
echo "$tell" | "$FTP" >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of 32 bit integer overflow crash bug in ftp.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo '32 bit integer overflow did not crash ftp.' >&2
fi

# Check regression of crash reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2021-12/msg00016.html
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash bug from message 2021-12/16...' >&2
"$FTP" < "$srcdir"/crash-ftp-msg2021-12_16.bin >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of ftp crash bug from message 2021-12/16.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Input from message 2021-12/16 did not crash ftp.' >&2
fi

# Check regression of crash reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2021-12/msg00004.html
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash bug from message 2021-12/04...' >&2
"$FTP" < "$srcdir"/crash-ftp-msg2021-12_04.bin >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of ftp crash bug from message 2021-12/04.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Input from message 2021-12/04 did not crash ftp.' >&2
fi

# Check regression of crash reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2021-12/msg00005.html
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash bug from message 2021-12/05...' >&2
"$FTP" < "$srcdir"/crash-ftp-msg2021-12_05.bin >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of ftp crash bug from message 2021-12/05.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Input from message 2021-12/05 did not crash ftp.' >&2
fi
# Check trivial infinite macro recursion, too.
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking ftp crash via infinite macro recursion...' >&2
tell='macdef X
$ X

$ X'
echo "$tell" | "$FTP" >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of infinite macro recursion crash bug in ftp.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Infinite macro recursion did not crash ftp.' >&2
fi

# Print test statistics.
$silence echo
test "$RESULT" -eq 0 && test "$SUCCESSES" -eq "$EFFORTS" && $silence false \
    || echo "Test had $SUCCESSES successes out of $EFFORTS cases".

# Report test result.
exit $RESULT
