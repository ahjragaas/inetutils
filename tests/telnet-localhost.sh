#!/bin/sh

# Copyright (C) 2011-2025 Free Software Foundation, Inc.
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

# Test the TELNET client as connection vehicle!
#
# Written by Mats Erik Andersson.

# Prerequisites:
#
#  * Shell: SVR4 Bourne shell, or newer.
#
#  * id(1), kill(1), mktemp(1), tty(1).
#
#  * Accessed by launched Inetd:
#      /etc/nsswitch.conf, /etc/passwd, /etc/protocols.
#
#    OpenBSD uses /etc/services directly, not via /etc/nsswitch.conf.

# Is usage explanation in demand?
#
if test "$1" = "-h" || test "$1" = "--help" || test "$1" = "--usage"; then
    cat <<HERE
Minimal test for TELNET client.

The following environment variables are used:

VERBOSE		Be verbose, if set.
TARGET		Receiving IPv4 address.
TARGET6		Receiving IPv6 address.
TARGET46	Receiving IPv4-mapped-IPV6 address.

HERE
    exit 0
fi

. ./tools.sh

$need_mktemp || exit_no_mktemp

# The executables under test.
#
INETD=${INETD:-../src/inetd$EXEEXT}
TELNET=${TELNET:-../telnet/telnet$EXEEXT}
ADDRPEEK=${ADDRPEEK:-./addrpeek$EXEEXT}

# Selected targets.
TARGET=${TARGET:-127.0.0.1}
TARGET6=${TARGET6:-::1}
TARGET46=${TARGET46:-::ffff:$TARGET}

# Step into `tests/', should the invokation
# have been made outside of it.
#
test -d src && test -f tests/telnet-localhost.sh && cd tests/

if test -n "$VERBOSE"; then
    set -x
    $INETD --version | $SED '1q'
    $TELNET --version | $SED '1q'
fi

# The use of telnet is portable only with a connected TTY.
if tty >/dev/null; then
    :
else
    echo 'No TTY assigned to this process.  Skipping test.' >&2
    exit 77
fi

if test ! -x $INETD; then
    echo "Missing executable '$INETD'.  Skipping test." >&2
    exit 77
fi

if test ! -x $TELNET; then
    echo "Missing executable '$TELNET'.  Skipping test." >&2
    exit 77
fi

if test ! -x $ADDRPEEK; then
    echo "Missing executable '$ADDRPEEK'.  Skipping test." >&2
    exit 77
fi

if test "$TEST_IPV4" = "no" && test "$TEST_IPV6" = "no"; then
    echo >&2 "Inet socket testing is disabled.  Skipping test."
    exit 77
fi

# Check regression of crash reported here:
# https://lists.gnu.org/archive/html/bug-inetutils/2022-02/msg00014.html
echo 'help z ! ? z ! ? z ! ? z ! ? z ! ? z ! ? z !' | $TELNET > /dev/null 2>&1
if test $? -ne 0; then
    echo "Regression of telnet crash bug." >&2
    exit 1
fi

# Check regression of crash reported here:
# https://lists.gnu.org/archive/html/bug-inetutils/2022-02/msg00009.html
echo 'help help' | $TELNET > /dev/null 2>&1
if test $? -ne 0; then
    echo "Regression of telnet second crash bug." >&2
    exit 1
fi

# Check regression of crash reported here:
# https://lists.gnu.org/archive/html/bug-inetutils/2022-02/msg00007.html
echo "unset ' '" | $TELNET > /dev/null 2>&1
if test $? -ne 0; then
    echo "Regression of telnet third crash bug." >&2
    exit 1
fi

# Check regression of crash reported here:
# https://lists.gnu.org/archive/html/bug-inetutils/2022-02/msg00010.html
echo "set ' ' foo" | $TELNET > /dev/null 2>&1
if test $? -ne 0; then
    echo "Regression of telnet fourth crash bug." >&2
    exit 1
fi

errno=0

# In non-verbose mode the variables `display' and `display_err'
# redirect output streams to `/dev/null'.

test -n "${VERBOSE+yes}" || display='>/dev/null'
test -n "${VERBOSE+yes}" || display_err='2>/dev/null'

# Check regression of integer overflow check reported in:
# https://lists.gnu.org/archive/html/bug-inetutils/2024-08/msg00007.html
for value in -2147483649 -1 256 2147483648 9223372036854775808; do
    for cmd in "do" "dont" "will" "wont"; do
        output=`echo "send $cmd $value" | $TELNET 2>&1`
        echo "$output" | eval "$GREP ': bad value ' $display"
        if test $? -ne 0; then
            errno=1
            echo "Failed to catch bad value '$value'." >&2
        fi
    done
done

# Portability fix for SVR4
PWD="${PWD:-`pwd`}"

# For file creation below IU_TESTDIR.
umask 0077

USER=`func_id_user`

# Random base directory at testing time.
TMPDIR=`$MKTEMP -d $PWD/tmp.XXXXXXXXXX` ||
    {
	echo 'Failed at creating test directory.  Aborting.' >&2
	exit 1
    }

INETD_CONF="$TMPDIR/inetd.conf"
INETD_PID="$TMPDIR/inetd.pid.$$"

posttesting () {
    if test -n "$TMPDIR" && test -f "$INETD_PID" \
	&& test -r "$INETD_PID" \
	&& kill -0 "`cat $INETD_PID`" >/dev/null 2>&1
    then
	kill "`cat $INETD_PID`" >/dev/null 2>&1 ||
	kill -9 "`cat $INETD_PID`" >/dev/null 2>&1
    fi
    test -n "$TMPDIR" && test -d "$TMPDIR" \
	&& rm -rf "$TMPDIR" $FILELIST
}

trap posttesting EXIT HUP INT QUIT TERM

PORT=`expr 4973 + ${RANDOM:-$$} % 973`

# Create an empty configuration file for inetd.
: > "$INETD_CONF" 2>/dev/null ||
    {
	echo 'Could not create configuration file for Inetd.  Aborting.' >&2
	exit 1
    }

# This target is used for plain IPv4 and for IPv4-mapped addressing.
if test "$TEST_IPV4" != "no"; then
    cat >> "$INETD_CONF" <<-EOF
	$TARGET:$PORT stream tcp4 nowait $USER $ADDRPEEK addrpeek addr
EOF
fi

if test "$TEST_IPV6" != "no"; then
    cat >> "$INETD_CONF" <<-EOF
	$TARGET6:$PORT stream tcp6 nowait $USER $ADDRPEEK addrpeek addr
EOF
fi

# Must use '-d' consistently to prevent daemonizing, but we
# would like to suppress the verbose output.

eval "$INETD -d -p'$INETD_PID' '$INETD_CONF' $display_err &"

# Debug mode allows the shell to recover PID of Inetd.
spawned_pid=$!

# Wait somewhat for the service to settle.
sleep 2

inetd_pid="`cat $INETD_PID 2>/dev/null`" ||
    {
	cat <<-EOT >&2
		Inetd did not create a PID-file.  Aborting test,
		but loosing control whether an Inetd process is
		still around.
	EOT
	exit 1
    }

test -z "$VERBOSE" || echo "Launched Inetd as process $inetd_pid." >&2

telnet_opts="--no-rc --no-escape --no-login"

if test "$TEST_IPV4" != "no" && test -n "$TARGET"; then
    output=`$TELNET $telnet_opts $TARGET $PORT 2>/dev/null`
    echo "$output" | eval "$GREP 'Your address is $TARGET.' $display"
    if test $? -ne 0; then
	errno=1
	echo "Failed at '$TARGET'." >&2
    fi
fi # TEST_IPV4 && TARGET

if test "$TEST_IPV6" != "no" && test -n "$TARGET6"; then
    output=`$TELNET $telnet_opts $TARGET6 $PORT 2>/dev/null`
    echo "$output" | eval "$GREP 'Your address is $TARGET6.' $display"
    if test $? -ne 0; then
	errno=1
	echo "Failed at '$TARGET6'." >&2
    fi
fi # TEST_IPV6 && TARGET6

if test "$TEST_IPV4" != "no" && test "$TEST_IPV6" != "no" \
   && test -n "$TARGET46"; then
    output=`$TELNET $telnet_opts $TARGET46 $PORT 2>/dev/null`
    echo "$output" | eval "$GREP 'Your address is .*$TARGET.' $display"
    if test $? -ne 0; then
	echo "Informational: Unsuccessful with mapped address '$TARGET46'." >&2
    fi
fi # TEST_IPV4 && TEST_IPV6 && TARGET46

exit $errno
