/*
  Copyright (C) 2001-2025 Free Software Foundation, Inc.

  This file is part of GNU Inetutils.

  GNU Inetutils is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or (at
  your option) any later version.

  GNU Inetutils is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see `http://www.gnu.org/licenses/'. */

#define PING_MAX_DATALEN (65535 - MAXIPLEN - MAXICMPLEN)

extern unsigned options;
#if !USE_IPV6
extern unsigned int suboptions;
#endif
extern PING *ping;
extern unsigned char *data_buffer;
extern size_t data_length;

extern int ping_run (PING * ping, int (*finish) (void));
extern int ping_finish (void);
extern void print_icmp_header (struct sockaddr_in *from,
			       struct ip *ip, icmphdr_t * icmp, int len);
