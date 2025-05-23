#
# Copyright (C) 2007-2025 Free Software Foundation, Inc.
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

AM_CPPFLAGS = $(iu_INCLUDES)

LDADD = $(iu_LIBRARIES)

EXTRA_DIST = tools.sh.in ifconfig_modes.sh crash-tftp-msg2021-12_18.bin \
	crash-ftp-msg2021-12_03.bin crash-ftp-msg2021-12_16.bin \
	crash-ftp-msg2021-12_04.bin crash-ftp-msg2021-12_05.bin

noinst_PROGRAMS = identify
identify_LDADD = $(top_builddir)/lib/libgnu.a $(LIBUTIL) $(PTY_LIB)

check_PROGRAMS = localhost readutmp runtime-ipv6 test-snprintf waitdaemon

localhost_LDADD = $(LDADD) $(LIBSOCKET) $(LIBINTL) $(GETADDRINFO_LIB)
readutmp_LDADD = $(iu_LIBRARIES) $(READUTMP_LIB)
runtime_ipv6_LDADD = $(LDADD) $(LIBSOCKET) $(LIBINTL) $(GETADDRINFO_LIB)

dist_check_SCRIPTS = utmp.sh

if ENABLE_inetd
check_PROGRAMS += addrpeek tcpget
addrpeek_LDADD = $(LDADD) $(LIBSOCKET) $(LIBINTL) $(GETADDRINFO_LIB)
tcpget_LDADD = $(LDADD) $(LIBSOCKET) $(LIBINTL) $(GETADDRINFO_LIB)
endif

if ENABLE_libls
noinst_PROGRAMS += ls
ls_LDADD = $(LIBLS) $(iu_LIBRARIES)
dist_check_SCRIPTS += libls.sh
endif

if ENABLE_ping
dist_check_SCRIPTS += ping-localhost.sh
endif
if ENABLE_traceroute
dist_check_SCRIPTS += traceroute-localhost.sh
endif
if ENABLE_tftpd
dist_check_SCRIPTS += tftp-regressions.sh
endif
if ENABLE_inetd
if ENABLE_tftpd
if ENABLE_tftp
dist_check_SCRIPTS += tftp.sh
endif
endif
endif
if ENABLE_logger
if ENABLE_syslogd
dist_check_SCRIPTS += syslogd.sh
endif
endif

if ENABLE_ftp
dist_check_SCRIPTS += ftp-parser.sh ftp-regressions.sh
endif

if ENABLE_inetd
if ENABLE_ftp
if ENABLE_ftpd
dist_check_SCRIPTS += ftp-localhost.sh
endif
endif
endif

if ENABLE_inetd
if ENABLE_telnet
dist_check_SCRIPTS += inetd.sh telnet-localhost.sh
endif
endif

if ENABLE_hostname
dist_check_SCRIPTS += hostname.sh
endif

if ENABLE_dnsdomainname
dist_check_SCRIPTS += dnsdomainname.sh
endif

if ENABLE_ifconfig
dist_check_SCRIPTS += ifconfig.sh
endif

TESTS = localhost test-snprintf waitdaemon $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT = EXEEXT=$(EXEEXT)

BUILT_SOURCES = tools.sh

CLEANFILES = tools.sh

tools_subst = sed -e 's,[@]GREP[@],$(GREP),g' \
	      -e 's,[@]EGREP[@],$(EGREP),g' \
	      -e 's,[@]FGREP[@],$(FGREP),g' \
	      -e 's,[@]SED[@],$(SED),g' \
	      -e 's,[@]DD[@],$(DD),g' \
	      -e 's,[@]MKTEMP[@],$(MKTEMP),g' \
	      -e 's,[@]NETSTAT[@],$(NETSTAT),g' \
	      -e 's,[@]TARGET[@],$(TARGET),g' \
	      -e 's,[@]TARGET6[@],$(TARGET6),g' \
	      -e 's,[@]TEST_IPV4[@],$(TEST_IPV4),g' \
	      -e 's,[@]TEST_IPV6[@],$(TEST_IPV6),g'

tools.sh: tools.sh.in Makefile
	$(tools_subst) < $(srcdir)/tools.sh.in > $@
