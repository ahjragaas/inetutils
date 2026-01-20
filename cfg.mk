#
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

manual_title = GNU Networking Utilities

guix = $(shell command -v guix > /dev/null && echo ,guix)
bootstrap-tools = gnulib,autoconf,automake,bison,m4,makeinfo,help2man,make,gzip,tar$(guix)

old_NEWS_hash = fee7edf0654ac08ffc242144bbb51975

translation_project_ =

_makefile_at_at_check_exceptions = ' && !/PATHDEFS_MAKE/'

announce_gen_args = --cksum-checksums
DIST_ARCHIVES += $(shell \
	if test -e $(srcdir)/.git && command -v git > /dev/null; then \
		echo $(PACKAGE)-v$(VERSION)-src.tar.gz; \
	fi)

VC_LIST_ALWAYS_EXCLUDE_REGEX = ^doc/fdl-1.3.texi$$
update-copyright-env = \
  UPDATE_COPYRIGHT_FORCE=1 \
  UPDATE_COPYRIGHT_USE_INTERVALS=2 \
  UPDATE_COPYRIGHT_MAX_LINE_LENGTH=79

TAR_OPTIONS += --mode=go+u,go-w --mtime=$(abs_top_srcdir)/NEWS.md

# maint.mk's public-submodule-commit breaks on shallow gnulib
submodule-checks =
gl_public_submodule_commit =

local-checks-to-skip = \
	sc_assignment_in_if \
	sc_bindtextdomain \
	sc_cast_of_x_alloc_return_value \
	sc_error_message_period \
	sc_error_message_uppercase \
	sc_m4_quote_check \
	sc_program_name \
	sc_prohibit_always_true_header_tests \
	sc_prohibit_assert_without_use \
	sc_prohibit_atoi_atof \
	sc_prohibit_doubled_word \
	sc_prohibit_error_without_use \
	sc_prohibit_gnu_make_extensions \
	sc_prohibit_magic_number_exit \
	sc_prohibit_stat_st_blocks \
	sc_prohibit_strcmp \
	sc_prohibit_strncpy \
	sc_prohibit_undesirable_word_seq \
	sc_prohibit_xalloc_without_use \
	sc_unmarked_diagnostics \

codespell_ignore_words_list = dont,wont,allo,servent,ointer,te,dout,merchantibility,rsource,nott,synching,fo,ans,inport,siz,flushin,nd,indx

exclude_file_name_regexp--sc_obsolete_symbols = \
	^tests/identify.c$$

exclude_file_name_regexp--sc_trailing_blank = \
	^(tests/crash-.*-msg.*.bin|gl/top/README-release.diff)$$

exclude_file_name_regexp--sc_prohibit_empty_lines_at_EOF = \
	^tests/crash-.*-msg.*.bin$$

exclude_file_name_regexp--sc_unportable_grep_q = \
	^gl/top/README-release.diff$$

sc_bsd_caddr:
	@prohibit=caddr''_t \
	halt='don'\''t use caddr''_t; instead use a standard pointer' \
	  $(_sc_search_regexp)

sc_assignment_in_if:
	prohibit='\<if *\(.* = ' halt="don't use assignments in if statements"	\
	  $(_sc_search_regexp)
