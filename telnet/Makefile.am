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
	-DTERMCAP -DLINEMODE -DKLUDGELINEMODE -DENV_HACK \
	$(PATHDEF_BSHELL) \
	$(iu_INCLUDES) \
	-I$(top_srcdir) \
	$(NCURSES_INCLUDE) $(INCIDN)

LDADD = \
	$(top_builddir)/libtelnet/libtelnet.a \
	$(iu_LIBRARIES) \
	$(LIBTERMCAP) $(LIBCRYPT) $(LIBAUTH) $(LIBIDN)

bin_PROGRAMS = $(telnet_BUILD)

EXTRA_PROGRAMS = telnet

telnet_SOURCES = authenc.c commands.c main.c network.c ring.c sys_bsd.c \
	telnet.c terminal.c tn3270.c utilities.c
telnet_LDADD = $(LDADD) $(LIBSOCKET) $(HOSTENT_LIB) $(LIBINTL) \
	$(GETADDRINFO_LIB)

noinst_HEADERS = defines.h externs.h general.h ring.h types.h
