#
# Copyright (C) 1997-2026 Free Software Foundation, Inc.
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

EXTRA_DIST = paths summary.sh.in CHECKLIST

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
	$(AM_V_GEN)if test -e $(srcdir)/.git; then			\
	  LC_ALL=en_US.UTF-8 TZ=UTC0					\
	    $(top_srcdir)/build-aux/gitlog-to-changelog			\
	      --srcdir=$(srcdir) --					\
	      v$(PREV_VERSION)~.. > $(distdir)/cl-t &&			\
	    { printf '\n\nSee the source repo for older entries\n'	\
	      >> $(distdir)/cl-t &&					\
	      rm -f $(distdir)/ChangeLog &&				\
	      mv $(distdir)/cl-t $(distdir)/ChangeLog; }		\
	fi

GIT_NEWS_MTIME_CMD = git log -1 --format=%cd --date=format-local:%Y%m%d%H%M.%S
dist-hook: mtime-NEWS-to-git-HEAD
.PHONY: mtime-NEWS-to-git-HEAD
mtime-NEWS-to-git-HEAD:
	$(AM_V_GEN)if test -e $(srcdir)/.git \
			&& command -v git > /dev/null; then \
		touch -m -t "$$($(GIT_NEWS_MTIME_CMD))" $(srcdir)/NEWS; \
	fi

dist-hook: texi-stamps-to-mtime-NEWS
.PHONY: texi-stamps-to-mtime-NEWS
texi-stamps-to-mtime-NEWS: mtime-NEWS-to-git-HEAD
	$(AM_V_GEN)touch -m -r $(srcdir)/NEWS $(srcdir)/doc/$(PACKAGE).texi

dist-hook: srcdist
.PHONY: srcdist
srcdist:
	$(AM_V_GEN)if test -e $(srcdir)/.git \
			&& command -v git > /dev/null; then \
	 cd $(srcdir) && \
	   git archive --prefix=$(PACKAGE)-v$(VERSION)/ \
	       -o $(abs_builddir)/$(PACKAGE)-v$(VERSION)-src.tar.gz HEAD; \
	fi

distclean-local: distclean-local-srcdist
.PHONY: distclean-local-srcdist
distclean-local-srcdist:
	-rm -f $(builddir)/$(PACKAGE)-*-src.tar.gz
