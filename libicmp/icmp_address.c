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

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
/*#include <netinet/ip_icmp.h> -- deliberately not including this */
#include <arpa/inet.h>
#include <icmp.h>

int
icmp_address_encode (unsigned char *buffer, size_t bufsize, int ident,
		     int seqno)
{
  icmphdr_t *icmp;

  if (bufsize < ICMP_MASKLEN)
    return -1;

  icmp = (icmphdr_t *) buffer;
  icmp->icmp_mask = 0;
  icmp_generic_encode (buffer, bufsize, ICMP_ADDRESS, ident, seqno);
  return 0;
}
