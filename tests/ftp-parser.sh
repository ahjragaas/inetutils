#!/bin/sh

# Copyright (C) 2014-2022 Free Software Foundation, Inc.
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

# Exercise the internal command interpreter of `ftp',
# without any networking.  Intended for code coverage
# testing.

set -u

: ${EXEEXT:=}

. ./tools.sh

silence=
bucket=

# Executable under test.
#
FTP=${FTP:-../ftp/ftp$EXEEXT}

if test ! -x "$FTP"; then
    echo >&2 "Missing executable '$FTP'.  Skipping test."
    exit 77
fi

if test -z "${VERBOSE+set}"; then
    silence=:
    bucket='>/dev/null'
fi

if test -n "${VERBOSE:-}"; then
    set -x
    $FTP --version | $SED '1q'
fi

errno=0

# Check that a client is not connected.
#
reply=`echo 'cd /tmp' | $FTP | $GREP -cv 'Not connected\.'`
test $reply -eq 0 || { errno=1
  echo >&2 'Failed respond to missing connection.'; }

# Check that help is plentiful.
#
reply=`echo help | $FTP | $SED -n '$='`
test $reply -ge 18 || { errno=1
  echo >&2 'Unexpectedly short help listing.'; }

# Check change from passive mode to active mode.
#
reply=`echo passive | $FTP -p | $GREP -cv 'Passive mode off\.'`
test $reply -eq 0 || { errno=1
  echo >&2 'Failed while switching back to active mode.'; }

# Check address mode.
reply=`echo ipv4 | $FTP --ipv6 | $GREP -cv 'Selecting addresses: IPv4'`
test $reply -eq 0 || { errno=1
  echo >&2 'Failed to reset address family by command.'; }

# Step size for hash markers.
#
tell='hash 7M
hash 12k'
reply=`echo "$tell" | $FTP -v | $EGREP -c '12288|7340032'`

test $reply -eq 2 || { errno=1
  echo >&2 'Failed to parse step sizes for hash printing.'; }

# A set of mixed commands for the sake of code coverage.
#
tell='bell
case
hash
nmap
proxy nmap
ntrans
proxy ntrans
runique
sunique
epsv4
lcd /tmp
lpwd'
reply=`echo "$tell" | $FTP`

test `echo "$reply" | $SED -n '$='` \
     -eq `echo "$tell" | $SED -n '$='` \
|| { errno=1; echo >&2 'Some command in mixed list produced no response.'; }

# At least Darwin has been known to prepend a directory stem.
DIR_STEM=`echo "$reply" | $SED -n 's,Local directory now \([^ ]*\)/tmp$,\1,p'`

test -z "$DIR_STEM" \
|| $silence echo "This system prepends a directory stem: $DIR_STEM"

test `echo "$reply" | $GREP -c "Local directory is $DIR_STEM/tmp"` -eq 1 \
|| { errno=1; echo >&2 'Failed to set local directory.'; }

# File name mappings can be given in several ways.
#
# One line giving both patterns.
tell='nmap a B
status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) a (out) B'` -eq 1 \
|| { errno=1; echo >&2 'Failed to set file name mapping using single line.'; }

# One line giving both patterns with tab between arguments to nmap.
tell='nmap x	Y
status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) x (out) Y'` -eq 1 || { errno=1
  echo >&2 'Failed to set file name mapping using tab between arguments.'; }

# Second pattern on a line of its own.
tell='nmap A
b
status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) A (out) b'` -eq 1 \
|| { errno=1; echo >&2 'Failed to set file name mapping using two lines.'; }

# The proxy connection also has a file name mapping.
#
# One line giving both patterns.
tell='proxy nmap c D
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) c (out) D'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name mapping using single line.'; }

# One line giving both patterns with tab between arguments to nmap.
tell='proxy nmap X	y
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) X (out) y'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name mapping (one line, tab b/w args).'; }

# Second pattern on a line of its own.
tell='proxy nmap C
d
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) C (out) d'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name mapping using two lines.'; }

# Proxy command on a separate line, tabs between arguments of nmap line.
tell='proxy
nmap	u	V
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) u (out) V'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name mapping (two lines, tab b/w args).'; }

# Both proxy command and second pattern on a line of its own.
tell='proxy
nmap e
F
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Nmap: (in) e (out) F'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name mapping using three lines.'; }

# File name translation configuration.
#
tell='ntrans a B
status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Ntrans: (in) a (out) B'` -eq 1 || { errno=1
  echo >&2 'Failed to set file name translation (space between arguments).'; }

# Arguments can be separated with space and/or tab characters, so test a tab.
tell='ntrans A	b
status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Ntrans: (in) A (out) b'` -eq 1 || { errno=1
  echo >&2 'Failed to set file name translation (tab between arguments).'; }

# File name translation can be set for the proxy connection.
tell='proxy ntrans c D
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Ntrans: (in) c (out) D'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name translation.'; }

# File name translation can be set for the proxy connection (tab b/w arguments).
tell='proxy ntrans C	d
proxy status'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Ntrans: (in) C (out) d'` -eq 1 || { errno=1
  echo >&2 'Failed to set proxy file name translation (tab b/w arguments).'; }

# Test nested macro execution.
#
tell='macdef A
$ B

macdef B
$ C

macdef C
$ D

macdef D
hash

$ A'
reply=`echo "$tell" | $FTP`
test `echo "$reply" | $FGREP -c 'Hash mark printing on'` -eq 1 || { errno=1
  echo >&2 'Failed to execute nested macros.'; }

# Summary of work.
#
test $errno -ne 0 || $silence echo "Successful testing".

exit $errno
