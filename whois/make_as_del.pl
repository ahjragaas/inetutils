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
	die "format error: $_" unless (/^([\d\.]+)\s+([\d\.]+)\s+([\w\.]+)$/);
	my $f=$1; my $l=$2; my $s=$3;
	print "{ ${f}, ${l}, \"";
	if ($s =~ /\./) {
		print "$s";
	} else {
		print "whois.$s.net";
	}
	print "\" },\n";
}
