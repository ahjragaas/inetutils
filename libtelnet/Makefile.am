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

AM_CPPFLAGS = \
	$(INCAUTH) \
	$(iu_INCLUDES)

noinst_LIBRARIES = libtelnet.a

libtelnet_a_SOURCES = \
	auth.c enc_des.c encrypt.c forward.c genget.c \
	kerberos.c kerberos5.c misc.c read_passwd.c shishi.c

noinst_HEADERS = \
	auth-proto.h auth.h enc-proto.h encrypt.h genget.h key-proto.h \
	misc-proto.h misc.h
