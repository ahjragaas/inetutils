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
	$(INCIDN) \
	$(PATHDEF_TMP) $(PATHDEF_BSHELL)

LDADD = \
	$(iu_LIBRARIES) \
	$(LIBREADLINE) $(LIBHISTORY) \
	$(LIBIDN) \
	$(CLOCK_TIME_LIB)

bin_PROGRAMS = $(ftp_BUILD)

EXTRA_PROGRAMS = ftp

ftp_SOURCES = cmds.c cmdtab.c domacro.c ftp.c main.c ruserpass.c
ftp_LDADD = $(LDADD) $(LIBSOCKET) $(LIBINTL) $(GETADDRINFO_LIB)

noinst_HEADERS = extern.h ftp_var.h
