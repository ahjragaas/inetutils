#!/usr/bin/perl -w

# Copyright (C) 1998-2025 Free Software Foundation, Inc.
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

use strict;

while (<>) {
	chomp;
	s/^\s*(.*)\s*$/$1/;
	s/\s*#.*$//;
	next if /^$/;
	die "format error: $_" unless
		(my ($a, $b) = /^([\w\d\.-]+)\s+([\w\d\.:-]+|[A-Z]+\s+.*)$/);
	$b =~ s/^W(?:EB)?\s+/\\001/;
	$b =~ s/^M(?:SG)?\s+/\\002/;
	$b = "\\003" if ($b eq 'NONE');
	print "    \"$a\",\t\"$b\",\n";
}
