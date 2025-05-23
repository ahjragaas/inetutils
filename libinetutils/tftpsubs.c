/*
  Copyright (C) 1998-2025 Free Software Foundation, Inc.

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

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Simple minded read-ahead/write-behind subroutines for tftp user and
   server.  Written originally with multiple buffers in mind, but current
   implementation has two buffer logic wired in.

   Todo:  add some sort of final error check so when the write-buffer
   is finally flushed, the caller can detect if the disk filled up
   (or had an i/o error) and return a nak to the other side.

			Jim Guyton 10/85
 */

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#include <netinet/in.h>
#include <arpa/tftp.h>

#include <stdio.h>
#include <unistd.h>

#include "tftpsubs.h"

/* Some systems define PKTSIZE in <arpa/tftp.h>.  */
#ifndef PKTSIZE
# define PKTSIZE SEGSIZE+4	/* should be moved to tftp.h */
#endif

struct bf
{
  int counter;			/* size of data in buffer, or flag */
  char buf[PKTSIZE];		/* room for data packet */
} bfs[2];

				/* Values for bf.counter  */
#define BF_ALLOC -3		/* alloc'd but not yet filled */
#define BF_FREE  -2		/* free */
/* [-1 .. SEGSIZE] = size of data in the data buffer */

static int nextone;		/* index of next buffer to use */
static int current;		/* index of buffer in use */

				/* control flags for crlf conversions */
int newline = 0;		/* fillbuf: in middle of newline expansion */
int prevchar = -1;		/* putbuf: previous char (cr check) */

static struct tftphdr *rw_init (int);

struct tftphdr *
w_init (void)
{
  return rw_init (0);
}				/* write-behind */

struct tftphdr *
r_init (void)
{
  return rw_init (1);
}				/* read-ahead */

/* init for either read-ahead or write-behind */
/* zero for write-behind, one for read-head */
static struct tftphdr *
rw_init (int x)
{
  newline = 0;			/* init crlf flag */
  prevchar = -1;
  bfs[0].counter = BF_ALLOC;	/* pass out the first buffer */
  current = 0;
  bfs[1].counter = BF_FREE;
  nextone = x;			/* ahead or behind? */
  return (struct tftphdr *) bfs[0].buf;
}


/* Have emptied current buffer by sending to net and getting ack.
   Free it and return next buffer filled with data.
 */
/* if true, convert to ascii */
/* file opened for read */
int
readit (FILE *file, struct tftphdr **dpp, int convert)
{
  struct bf *b;

  bfs[current].counter = BF_FREE;	/* free old one */
  current = !current;		/* "incr" current */

  b = &bfs[current];		/* look at new buffer */
  if (b->counter == BF_FREE)	/* if it's empty */
    read_ahead (file, convert);	/* fill it */
  /*      assert(b->counter != BF_FREE); *//* check */
  *dpp = (struct tftphdr *) b->buf;	/* set caller's ptr */
  return b->counter;
}

/*
 * fill the input buffer, doing ascii conversions if requested
 * conversions are  lf -> cr,lf  and cr -> cr, nul
 */
/*	FILE *file;  file opened for read */
/*	int convert;  if true, convert to ascii */
void
read_ahead (FILE *file, int convert)
{
  int i;
  char *p;
  int c;
  struct bf *b;
  struct tftphdr *dp;

  b = &bfs[nextone];		/* look at "next" buffer */
  if (b->counter != BF_FREE)	/* nop if not free */
    return;
  nextone = !nextone;		/* "incr" next buffer ptr */

  dp = (struct tftphdr *) b->buf;

  if (convert == 0)
    {
      b->counter = read (fileno (file), dp->th_data, SEGSIZE);
      return;
    }

  p = dp->th_data;
  for (i = 0; i < SEGSIZE; i++)
    {
      if (newline)
	{
	  if (prevchar == '\n')
	    c = '\n';		/* lf to cr,lf */
	  else
	    c = '\0';		/* cr to cr,nul */
	  newline = 0;
	}
      else
	{
	  c = getc (file);
	  if (c == EOF)
	    break;
	  if (c == '\n' || c == '\r')
	    {
	      prevchar = c;
	      c = '\r';
	      newline = 1;
	    }
	}
      *p++ = c;
    }
  b->counter = (int) (p - dp->th_data);
}

/* Update count associated with the buffer, get new buffer
   from the queue.  Calls write_behind only if next buffer not
   available.
 */
int
writeit (FILE *file, struct tftphdr **dpp, int ct, int convert)
{
  bfs[current].counter = ct;	/* set size of data to write */
  current = !current;		/* switch to other buffer */
  if (bfs[current].counter != BF_FREE)	/* if not free */
    write_behind (file, convert);	/* flush it */
  bfs[current].counter = BF_ALLOC;	/* mark as alloc'd */
  *dpp = (struct tftphdr *) bfs[current].buf;
  return ct;			/* this is a lie of course */
}

/*
 * Output a buffer to a file, converting from netascii if requested.
 * CR,NUL -> CR  and CR,LF => LF.
 * Note spec is undefined if we get CR as last byte of file or a
 * CR followed by anything else.  In this case we leave it alone.
 */
int
write_behind (FILE *file, int convert)
{
  char *buf;
  int count;
  int ct;
  char *p;
  int c;			/* current character */
  struct bf *b;
  struct tftphdr *dp;

  b = &bfs[nextone];
  if (b->counter < -1)		/* anything to flush? */
    return 0;			/* just nop if nothing to do */

  count = b->counter;		/* remember byte count */
  b->counter = BF_FREE;		/* reset flag */
  dp = (struct tftphdr *) b->buf;
  nextone = !nextone;		/* incr for next time */
  buf = dp->th_data;

  if (count <= 0)
    return -1;			/* nak logic? */

  if (convert == 0)
    return write (fileno (file), buf, count);

  p = buf;
  ct = count;
  while (ct--)
    {				/* loop over the buffer */
      c = *p++;			/* pick up a character */
      if (prevchar == '\r')
	{			/* if prev char was cr */
	  if (c == '\n')	/* if have cr,lf then just */
	    fseeko (file, -1, 1);	/* smash lf on top of the cr */
	  else if (c == '\0')	/* if have cr,nul then */
	    goto skipit;	/* just skip over the putc */
	  /* else just fall through and allow it */
	}
      putc (c, file);
    skipit:
      prevchar = c;
    }
  return count;
}


/* When an error has occurred, it is possible that the two sides
 * are out of synch.  Ie: that what I think is the other side's
 * response to packet N is really their response to packet N-1.
 *
 * So, to try to prevent that, we flush all the input queued up
 * for us on the network connection on our host.
 *
 * We return the number of packets we flushed (mostly for reporting
 * when trace is active).
 */

/*int	f;socket to flush */
int
synchnet (int f)
{
  int i, j = 0;
  char rbuf[PKTSIZE];
  struct sockaddr_storage from;
  socklen_t fromlen;

  while (1)
    {
      ioctl (f, FIONREAD, &i);
      if (i)
	{
	  j++;
	  fromlen = sizeof from;
	  recvfrom (f, rbuf, sizeof (rbuf), 0,
		    (struct sockaddr *) &from, &fromlen);
	}
      else
	{
	  return (j);
	}
    }
}
