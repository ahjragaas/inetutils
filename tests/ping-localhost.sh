#!/bin/sh

# Copyright (C) 2009-2025 Free Software Foundation, Inc.
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

# Prerequisites:
#
#  * Shell: SVR3 Bourne shell, or newer.
#
#  * cut(1), id(1), uname(1).

. ./tools.sh

PROTOCOLS=/etc/protocols
if test ! -r $PROTOCOLS; then
    cat <<-EOT >&2
	This test requires the availability of "$PROTOCOLS",
	a file which can not be found in the current system.
	Therefore skipping this test.
	EOT
    exit 77
fi

PING=${PING:-../ping/ping$EXEEXT}
TARGET=${TARGET:-127.0.0.1}

PING6=${PING6:-../ping/ping6$EXEEXT}
TARGET6=${TARGET6:-::1}

if [ ! -x $PING ]; then
    echo 'No executable "'$PING'" available.  Skipping test.' >&2
    exit 77
fi

if [ $VERBOSE ]; then
    set -x
    $PING --version
fi

if test "$TEST_IPV4" = "no" && test "$TEST_IPV6" = "no"; then
    echo >&2 "Inet socket testing is disabled.  Skipping test."
    exit 77
fi

have_privs='no'
test "`func_id_uid`" = 0 && have_privs='yes'

# some systems allow ping without root privileges
need_privs='yes'

# the kernel Linux requires specific configuration for unprivileged ping
PING_GROUP_RANGE=/proc/sys/net/ipv4/ping_group_range
if test "`uname -s`" = 'Linux' && test -f "$PING_GROUP_RANGE"
then
    low=`cut -f1 "$PING_GROUP_RANGE"`
    high=`cut -f2 "$PING_GROUP_RANGE"`
    for grp_id in `id -G`; do
        test "$low" -le "$grp_id" && test "$high" -ge "$grp_id" &&
            need_privs='no' && break
    done
fi

# macOS generally allows unprivileged ping
test "`uname -s`" = 'Darwin' && need_privs='no'

if test "$need_privs" = 'yes' && test "$have_privs" = 'no'; then
    echo >&2 "ping needs to run as root"
    exit 77
fi

# the ping6 program still requires privileges
test "$need_privs" = 'no' && test "$have_privs" = 'no' &&
    test "$TEST_IPV6" != 'no' && TEST_IPV6='no' &&
    echo >&2 'ping6 needs to run as root, disabling IPv6 test' &&
    test "$TEST_IPV4" = 'no' &&
    echo >&2 'Testing of IPv4 and IPv6 is disabled.  Skipping test.' &&
    exit 77

errno=0
errno2=0

test "$TEST_IPV4" != "no" && test -x $PING &&
    { $PING -n -c 1 $TARGET || errno=$?; }

test $errno -eq 0 || echo "Failed at pinging $TARGET." >&2

# Host might not have been built with IPv6 support.
test "$TEST_IPV6" != "no" && test -x $PING6 &&
    { $PING6 -n -c 1 $TARGET6 || errno2=$?; }

test $errno2 -eq 0 || echo "Failed at pinging $TARGET6." >&2

test $errno -eq 0 || exit $errno

exit $errno2
