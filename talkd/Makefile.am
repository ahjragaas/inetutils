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
	$(PATHDEF_DEV) \
	$(PATHDEF_TTY_PFX) $(PATHDEF_UTMP) $(PATHDEF_UTMPX)

LDADD = $(iu_LIBRARIES) $(LIBTHREAD) $(LIB_MBRTOWC) $(READUTMP_LIB)

inetdaemondir = @inetdaemondir@

inetdaemon_PROGRAMS = $(talkd_BUILD)

EXTRA_PROGRAMS = talkd

talkd_SOURCES = \
 acl.c\
 announce.c\
 print.c\
 process.c\
 table.c\
 talkd.c\
 intalkd.h

talkd_LDADD = $(LDADD) $(HOSTENT_LIB)
