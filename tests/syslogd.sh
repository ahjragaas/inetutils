#!/bin/sh

# Copyright (C) 2011 Free Software Foundation, Inc.
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

# Tests to establish functionality of SYSLOG daemon.
#

# Keep any external assignment.
#
: ${IU_TESTDIR:=/tmp/iu_syslog}

mkdir -p $IU_TESTDIR

CONF=$IU_TESTDIR/syslog.conf
PID=$IU_TESTDIR/syslogd.pid
OUT=$IU_TESTDIR/messages
: ${SOCKET:=$IU_TESTDIR/log}

# For testing of critical lengths for UNIX socket names,
# we need a well defined base directory; choose "/tmp/".
IU_TEN=0123456789
IU_TWENTY=${IU_TEN}${IU_TEN}
IU_FORTY=${IU_TWENTY}${IU_TWENTY}
IU_EIGHTY=${IU_FORTY}${IU_FORTY}

# This good name base consumes twentythree chracters.
IU_GOOD_BASE=/tmp/$(date +%y-%m-%d)_socket_iu

# Add a single character to violate the size condition.
IU_BAD_BASE=/tmp/X$(date +%y-%m-%d)_socket_iu

IU_OS=$(uname -s)
if [ "${IU_OS}" != "OpenBSD" -a "${IU_OS}" != "FreeBSD" ]; then
	# Aim at the boundary of 108 characters.
	IU_GOOD_BASE=${IU_GOOD_BASE}_lnx
	IU_BAD_BASE=${IU_BAD_BASE}_lnx
fi

# Establish largest possible socket name.  The long
# name consists of 103 or 107 non-NUL characters,
# where the excessive string contains 104 or 108.
# BSD allocates only 104, whereas GLIBC and Solaris
# admits 108 characters in "sun_path", including NUL.
IU_LONG_SOCKET=${IU_GOOD_BASE}${IU_EIGHTY}
IU_EXCESSIVE_SOCKET=${IU_BAD_BASE}${IU_EIGHTY}

# All messages intended for post-detection are
# to be uniformly tagged.
TAG="syslogd-test"

# The executables under test.

IU_SYSLOGD=./src/syslogd$EXEEXT
IU_LOGGER=./src/logger$EXEEXT

# Step out of `tests/', should the invokation
# have been made there.
#
[ -d ../src ] && cd ..

if [ ! -x $IU_SYSLOGD ]; then
	echo "Missing executable 'syslogd'. Failing."
	exit 1;
fi

if [ ! -x $IU_LOGGER ]; then
	echo "Missing executable 'logger'. Failing."
	exit 1
fi

# Remove old messages.
rm -f $OUT $PID

# Full testing needs a superuser.  Report this.
if [ "$USER" != "root" ]; then
	cat <<-EOT
	WARNING!!
	Disabling INET server tests since you seem
	to be underprivileged.
	EOT
fi

# A minimal, catch-all configuration.
#
cat > $CONF <<-EOT
	*.*	$OUT
	# Test incorrect forwarding.
	*.*	@not.in.existence
	# Recover from missing action field and short selector.
	12345
	*.*
	*.	/dev/null
EOT

# Set REMOTE_LOGHOST to activate forwarding
#
if [ -n "$REMOTE_LOGHOST" ]; then
	# Append a forwarding stanza.
	cat >> $CONF <<-EOT
		# Forwarding remotely
		*.*	@$REMOTE_LOGHOST
	EOT
fi

# Attempt to start the server after first
# building the desired option list.
#
## Base configuration.
IU_OPTIONS="--rcfile=$CONF --pidfile=$PID --socket=$SOCKET"
IU_OPTIONS="$IU_OPTIONS -a $IU_LONG_SOCKET -a $IU_EXCESSIVE_SOCKET"

## Enable INET service when running as root.
if [ "$USER" = "root" ]; then
	IU_OPTIONS="$IU_OPTIONS --ipany --inet --hop"
fi
## Bring in additional options from command line.
## Disable kernel messages otherwise.
: OPTIONS=${OPTIONS:=--no-klog}
IU_OPTIONS="$IU_OPTIONS $OPTIONS"

$IU_SYSLOGD $IU_OPTIONS

# Wait a moment in order to avoid an obvious
# race condition with the server daemon on
# slow systems.
#
sleep 1

# Test to see whether the service got started.
#
if [ ! -r $PID ]; then
	echo "The service daemon never started. Failing."
	exit 1
fi

# Declare the number of implemented tests,
# as well as an exit code.
#
TESTCASES=0
SUCCESSES=0
EXITCODE=1

# Check that the excessively long UNIX socket name was rejected.
TESTCASES=$((TESTCASES + 1))
if grep -q "UNIX socket name too long.*${IU_BAD_BASE}" $OUT; then
	SUCCESSES="$((SUCCESSES + 1))"
fi

# Send messages on two sockets: IPv4 and UNIX.
#
TESTCASES=$((TESTCASES + 2))
$IU_LOGGER -h $SOCKET -p user.info -t $TAG "Sending BSD message. (pid $$)"
$IU_LOGGER -h $IU_LONG_SOCKET -p user.info -t $TAG "Sending via long socket name. (pid $$)"

if [ "$USER" = "root" ]; then
	TESTCASES=$((TESTCASES + 2))
	$IU_LOGGER -4 -h localhost -p user.info -t $TAG "Sending IPv4 message. (pid $$)"
	$IU_LOGGER -6 -h localhost -p user.info -t $TAG "Sending IPv6 message. (pid $$)"
fi

# Detection of registered messages.
#
SUCCESSES="$((SUCCESSES + $(grep $TAG $OUT | wc -l) ))"

if [ -n "${VERBOSE+yes}" ]; then
	grep $TAG $OUT
fi

echo "Registered $SUCCESSES successes out of $TESTCASES."

if [ "$SUCCESSES" -eq "$TESTCASES" ]; then
	echo "Successful testing."
	EXITCODE=0
else
	echo "Failing some tests."
fi

# Remove the daemon process.
[ -r $PID ] && kill "$(cat $PID)"

exit $EXITCODE