#
# Copyright (C) 1997-2025 Free Software Foundation, Inc.
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

@PATHDEFS_MAKE@

AM_CPPFLAGS = \
	$(iu_INCLUDES) \
	-I$(top_srcdir) \
	$(INCAUTH) $(NCURSES_INCLUDE) \
	$(PATHDEF_DEV) $(PATHDEF_TTY) $(PATHDEF_TTY_PFX) $(PATHDEF_LOGIN)

LDADD = \
	$(top_builddir)/libtelnet/libtelnet.a \
	$(iu_LIBRARIES) \
	$(LIBTERMCAP) $(LIBUTIL) $(PTY_LIB) $(LIBCRYPT) $(LIBAUTH)

if ENABLE_hstrerror
LDADD += $(LIBRESOLV)
endif

inetdaemondir = @inetdaemondir@

inetdaemon_PROGRAMS = $(telnetd_BUILD)

EXTRA_PROGRAMS = telnetd

telnetd_SOURCES = telnetd.c pty.c term.c utility.c state.c termstat.c slc.c
telnetd_LDADD = $(LDADD) $(HOSTENT_LIB) $(LIBINTL) $(GETADDRINFO_LIB) \
  $(LIBRESOLV)

noinst_HEADERS = telnetd.h
