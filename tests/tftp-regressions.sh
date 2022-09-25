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

# Regression tests for the TFTP client "tftp".

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

# This script tests and thus requires the TFTP client
TFTP="${TFTP:-../src/tftp$EXEEXT}"
if [ ! -x $TFTP ]; then
    echo "No TFTP client '$TFTP' present.  Skipping test" >&2
    exit 77
fi

# Print version of TFTP client in VERBOSE mode.
if [ "$VERBOSE" ]; then
    "$TFTP" --version | $SED '1q'
fi

# Initialize test statistics.
SUCCESSES=0
EFFORTS=0
RESULT=0

# Check regression of crash reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2021-12/msg00018.html
EFFORTS=`expr $EFFORTS + 1`
$silence echo 'Checking crash bug from message 2021-12/18...' >&2
"$TFTP" < $srcdir/crash-tftp-msg2021-12_18.bin >/dev/null 2>&1
if test $? -ne 0; then
    $silence echo 'Regression of tftp crash bug from message 2021-12/18.' >&2
    RESULT=1
else
    SUCCESSES=`expr $SUCCESSES + 1`
    $silence echo 'Input from message 2021-12/18 did not crash tftp.' >&2
fi

# Print test statistics.
$silence echo
test "$RESULT" -eq 0 && test "$SUCCESSES" -eq "$EFFORTS" && $silence false \
    || echo "Test had $SUCCESSES successes out of $EFFORTS cases".

# Report test result.
exit $RESULT
