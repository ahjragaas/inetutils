#
# Copyright (C) 1997-2024 Free Software Foundation, Inc.
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

ACLOCAL_AMFLAGS = -I am -I m4

EXTRA_DIST = paths ChangeLog.0 ChangeLog.1 summary.sh.in CHECKLIST

EXTRA_DIST += bootstrap bootstrap.conf bootstrap-funclib.sh

SUBDIRS = lib \
	libinetutils libtelnet libicmp libls \
	src telnet telnetd ftp ftpd talk talkd whois ping ifconfig \
	doc man \
	tests

DISTCLEANFILES = pathdefs.make paths.defs

# git-version-gen
EXTRA_DIST += $(top_srcdir)/.version
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook: gen-ChangeLog
	echo $(VERSION) > $(distdir)/.tarball-version

.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then				\
		$(top_srcdir)/build-aux/gitlog-to-changelog		\
			-- v2.0~.. > $(distdir)/cl-t &&			\
		{ rm -f $(distdir)/ChangeLog &&				\
		  mv $(distdir)/cl-t $(distdir)/ChangeLog; }		\
	fi

# dist

dist-hook: mtime-NEWS-to-git-HEAD

.PHONY: mtime-NEWS-to-git-HEAD
mtime-NEWS-to-git-HEAD:
	$(AM_V_GEN)if test -e $(srcdir)/.git \
			&& command -v git > /dev/null; then \
		touch -m -d @"$$(git log -1 --format=%ct)" $(srcdir)/NEWS; \
	fi
