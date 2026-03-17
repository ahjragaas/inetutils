/*
  Copyright (C) 1993-2026 Free Software Foundation, Inc.

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

#define TELOPTS
#define TELCMDS
#define SLC_NAMES
#include "telnetd.h"
#include <stdarg.h>
#ifdef HAVE_TERMIO_H
# include <termio.h>
#endif
#include <time.h>

#if defined AUTHENTICATION || defined ENCRYPTION
# include <libtelnet/misc.h>
# define NET_ENCRYPT net_encrypt
#else
# define NET_ENCRYPT()
#endif

#ifdef HAVE_TERMCAP_TGETENT
# include <termcap.h>
#elif defined HAVE_CURSES_TGETENT
# include <curses.h>
# ifndef _XOPEN_CURSES
#  include <term.h>
# endif
#endif

#if defined HAVE_STREAMSPTY && defined HAVE_GETMSG	\
  && defined HAVE_STROPTS_H
# include <stropts.h>
#endif

#include <attribute.h>

static char netobuf[BUFSIZ + NETSLOP], *nfrontp, *nbackp;
static char *neturg;		/* one past last byte of urgent data */
#ifdef  ENCRYPTION
static char *nclearto;
#endif

static char ptyobuf[BUFSIZ + NETSLOP], *pfrontp, *pbackp;

static char netibuf[BUFSIZ], *netip;
static int ncc;

static char ptyibuf[BUFSIZ], *ptyip;
static int pcc;

extern int not42;

static int
readstream (int p, char *ibuf, int bufsize)
{
#ifndef HAVE_STREAMSPTY
  return read (p, ibuf, bufsize);
#else
  int flags = 0;
  int ret = 0;
  struct termios *tsp;
# ifdef HAVE_TERMIO_H
  struct termio *tp;
# endif
  struct iocblk *ip;
  char vstop, vstart;
  int ixon;
  int newflow;
  struct strbuf strbufc, strbufd;
  unsigned char ctlbuf[BUFSIZ];
  static int flowstate = -1;

  strbufc.maxlen = BUFSIZ;
  strbufc.buf = (char *) ctlbuf;
  strbufd.maxlen = bufsize - 1;
  strbufd.len = 0;
  strbufd.buf = ibuf + 1;
  ibuf[0] = 0;

  ret = getmsg (p, &strbufc, &strbufd, &flags);
  if (ret < 0)			/* error of some sort -- probably EAGAIN */
    return -1;

  if (strbufc.len <= 0 || ctlbuf[0] == M_DATA)
    {
      /* data message */
      if (strbufd.len > 0)	/* real data */
	return strbufd.len + 1;	/* count header char */
      else
	{
	  /* nothing there */
	  errno = EAGAIN;
	  return -1;
	}
    }

  /*
   * It's a control message.  Return 1, to look at the flag we set
   */

  switch (ctlbuf[0])
    {
    case M_FLUSH:
      if (ibuf[1] & FLUSHW)
	ibuf[0] = TIOCPKT_FLUSHWRITE;
      return 1;

    case M_IOCTL:
      ip = (struct iocblk *) (ibuf + 1);

      switch (ip->ioc_cmd)
	{
	case TCSETS:
	case TCSETSW:
	case TCSETSF:
	  tsp = (struct termios *) (ibuf + 1 + sizeof (struct iocblk));
	  vstop = tsp->c_cc[VSTOP];
	  vstart = tsp->c_cc[VSTART];
	  ixon = tsp->c_iflag & IXON;
	  break;

# ifdef HAVE_TERMIO_H
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
	  tp = (struct termio *) (ibuf + 1 + sizeof (struct iocblk));
	  vstop = tp->c_cc[VSTOP];
	  vstart = tp->c_cc[VSTART];
	  ixon = tp->c_iflag & IXON;
	  break;
# endif/* HAVE_TERMIO_H */

	default:
	  errno = EAGAIN;
	  return -1;
	}

      newflow = (ixon && (vstart == 021) && (vstop == 023)) ? 1 : 0;
      if (newflow != flowstate)	/* it's a change */
	{
	  flowstate = newflow;
	  ibuf[0] = newflow ? TIOCPKT_DOSTOP : TIOCPKT_NOSTOP;
	  return 1;
	}
    }

  /* nothing worth doing anything about */
  errno = EAGAIN;
  return -1;
#endif /* HAVE_STREAMSPTY */
}

/* ************************************************************************* */
/* Net and PTY I/O functions */

void
io_setup (void)
{
  pfrontp = pbackp = ptyobuf;
  nfrontp = nbackp = netobuf;
#ifdef  ENCRYPTION
  nclearto = 0;
#endif
  netip = netibuf;
  ptyip = ptyibuf;
}

void
set_neturg (void)
{
  neturg = nfrontp - 1;
}

/* net-buffers */

void
net_output_byte (int c)
{
  *nfrontp++ = c;
}

int
net_output_data (const char *format, ...)
{
  va_list args;
  size_t remaining, ret;

  va_start (args, format);
  remaining = BUFSIZ - (nfrontp - netobuf);
  /* try a netflush() if the room is too low */
  if (strlen (format) > remaining || BUFSIZ / 4 > remaining)
    {
      netflush ();
      remaining = BUFSIZ - (nfrontp - netobuf);
    }
  ret = vsnprintf (nfrontp, remaining, format, args);
  nfrontp += ((ret < remaining - 1) ? ret : remaining - 1);
  va_end (args);
  return ret;
}

int
net_output_datalen (const void *buf, size_t l)
{
  size_t remaining;

  remaining = BUFSIZ - (nfrontp - netobuf);
  if (remaining < l)
    {
      netflush ();
      remaining = BUFSIZ - (nfrontp - netobuf);
    }
  if (remaining < l)
    return -1;
  memmove (nfrontp, buf, l);
  nfrontp += l;
  return (int) l;
}

int
net_input_level (void)
{
  return ncc;
}

int
net_output_level (void)
{
  return nfrontp - nbackp;
}

int
net_buffer_is_full (void)
{
  return (&netobuf[BUFSIZ] - nfrontp) < 2;
}

int
net_get_char (int peek)
{
  if (peek)
    return *netip;
  else if (ncc > 0)
    {
      ncc--;
      return *netip++ & 0377;
    }

  return 0;
}

int
net_read (void)
{
  ncc = read (net, netibuf, sizeof (netibuf));
  if (ncc < 0 && errno == EWOULDBLOCK)
    ncc = 0;
  else if (ncc == 0)
    {
      syslog (LOG_INFO, "telnetd:  peer died");
      cleanup (0);
      /* NOT REACHED */
    }
  else if (ncc > 0)
    {
      netip = netibuf;
    }
  return ncc;
}

/* PTY buffer functions */

int
pty_buffer_is_full (void)
{
  return (&ptyobuf[BUFSIZ] - pfrontp) < 2;
}

void
pty_output_byte (int c)
{
  *pfrontp++ = c;
}

void
pty_output_datalen (const void *data, size_t len)
{
  if ((size_t) (&ptyobuf[BUFSIZ] - pfrontp) > len)
    ptyflush ();
  memcpy (pfrontp, data, len);
  pfrontp += len;
}

int
pty_input_level (void)
{
  return pcc;
}

int
pty_output_level (void)
{
  return pfrontp - pbackp;
}

void
ptyflush (void)
{
  int n;

  if ((n = pfrontp - pbackp) > 0)
    n = write (pty, pbackp, n);

  if (n < 0)
    {
      if (errno == EWOULDBLOCK || errno == EINTR)
	return;
      cleanup (0);
      /* NOT REACHED */
    }

  pbackp += n;
  if (pbackp == pfrontp)
    pbackp = pfrontp = ptyobuf;
}

int
pty_get_char (int peek)
{
  if (peek)
    return *ptyip;
  else if (pcc > 0)
    {
      pcc--;
      return *ptyip++ & 0377;
    }

  return 0;
}

int
pty_input_putback (const char *str, size_t len)
{
  if (len > (size_t) (&ptyibuf[BUFSIZ] - ptyip))
    len = &ptyibuf[BUFSIZ] - ptyip;
  strncpy (ptyip, str, len);
  pcc += len;

  return 0;
}

/* pty_read()
 *
 * Read errors EWOULDBLOCK, EAGAIN, and EIO are
 * tweaked into reporting zero bytes input.
 * In particular, EIO is known to appear when
 * reading off the master side, before having
 * an active slave side.
 */
int
pty_read (void)
{
  pcc = readstream (pty, ptyibuf, BUFSIZ);
  if (pcc < 0 && (errno == EWOULDBLOCK
#ifdef	EAGAIN
		  || errno == EAGAIN
#endif
		  || errno == EIO))
    pcc = 0;
  ptyip = ptyibuf;

  return pcc;
}

/* ************************************************************************* */


/* io_drain ()
 *
 *
 *	A small subroutine to flush the network output buffer, get some data
 * from the network, and pass it through the telnet state machine.  We
 * also flush the pty input buffer (by dropping its data) if it becomes
 * too full.
 */

void
io_drain (void)
{
  fd_set rfds;

  if (nfrontp - nbackp > 0)
    netflush ();

  FD_ZERO (&rfds);
  FD_SET (net, &rfds);
  if (1 != select (net + 1, &rfds, NULL, NULL, NULL))
    {
      syslog (LOG_INFO, "ttloop:  select: %m\n");
      exit (EXIT_FAILURE);
    }

  ncc = read (net, netibuf, sizeof netibuf);
  if (ncc < 0)
    {
      syslog (LOG_INFO, "ttloop:  read: %m\n");
      exit (EXIT_FAILURE);
    }
  else if (ncc == 0)
    {
      syslog (LOG_INFO, "ttloop:  peer died: %m\n");
      exit (EXIT_FAILURE);
    }
  netip = netibuf;
  telrcv ();			/* state machine */
  if (ncc > 0)
    {
      pfrontp = pbackp = ptyobuf;
      telrcv ();
    }
}				/* end of ttloop */

/*
 * Check a descriptor to see if out of band data exists on it.
 */
/* int	s; socket number */
int
stilloob (int s)
{
  static struct timeval timeout = { 0, 0 };
  fd_set excepts;
  int value;

  do
    {
      FD_ZERO (&excepts);
      FD_SET (s, &excepts);
      value = select (s + 1, (fd_set *) 0, (fd_set *) 0, &excepts, &timeout);
    }
  while (value == -1 && errno == EINTR);

  if (value < 0)
    fatalperror (pty, "select");

  return FD_ISSET (s, &excepts);
}

/*
 * nextitem()
 *
 *	Return the address of the next "item" in the TELNET data
 * stream.  This will be the address of the next character if
 * the current address is a user data character, or it will
 * be the address of the character following the TELNET command
 * if the current address is a TELNET IAC ("I Am a Command")
 * character.
 */
char *
nextitem (char *current, const char *endp)
{
  if (current >= endp)
    return NULL;
  if ((*current & 0xff) != IAC)
    return current + 1;
  if (current + 1 >= endp)
    return NULL;

  switch (*(current + 1) & 0xff)
    {
    case DO:
    case DONT:
    case WILL:
    case WONT:
      return current + 3 <= endp ? current + 3 : NULL;

    case SB:			/* loop forever looking for the SE */
      {
	char *look = current + 2;

	while (look < endp)
	  if ((*look++ & 0xff) == IAC && look < endp
	      && (*look++ & 0xff) == SE)
	    return look;

	return NULL;
      }
    default:
      return current + 2 <= endp ? current + 2 : NULL;
    }
}				/* end of nextitem */


/*
 * netclear()
 *
 *	We are about to do a TELNET SYNCH operation.  Clear
 * the path to the network.
 *
 *	Things are a bit tricky since we may have sent the first
 * byte or so of a previous TELNET command into the network.
 * So, we have to scan the network buffer from the beginning
 * until we are up to where we want to be.
 *
 *	A side effect of what we do, just to keep things
 * simple, is to clear the urgent data pointer.  The principal
 * caller should be setting the urgent data pointer AFTER calling
 * us in any case.
 */
#define wewant(p)					\
  ((nfrontp > p) && ((*p & 0xff) == IAC) &&		\
   (nfrontp > p + 1 && (((*(p + 1) & 0xff) != EC) &&	\
                        ((*(p + 1) & 0xff) != EL))))


void
netclear (void)
{
  char *thisitem, *next;
  char *good;

#ifdef	ENCRYPTION
  thisitem = nclearto > netobuf ? nclearto : netobuf;
#else /* ENCRYPTION */
  thisitem = netobuf;
#endif /* ENCRYPTION */

  while ((next = nextitem (thisitem, nbackp)) != NULL && next <= nbackp)
    thisitem = next;

  /* Now, thisitem is first before/at boundary. */

#ifdef	ENCRYPTION
  good = nclearto > netobuf ? nclearto : netobuf;
#else /* ENCRYPTION */
  good = netobuf;		/* where the good bytes go */
#endif /* ENCRYPTION */

  while (thisitem != NULL && nfrontp > thisitem)
    {
      if (wewant (thisitem))
	{
	  int length;

	  for (next = thisitem;
	       next != NULL && wewant (next) && nfrontp > next;
	       next = nextitem (next, nfrontp))
	    ;
	  if (next == NULL)
	    next = nfrontp;

	  length = next - thisitem;
	  memmove (good, thisitem, length);
	  good += length;
	  thisitem = next;
	}
      else
	{
	  thisitem = nextitem (thisitem, nfrontp);
	}
    }

  nbackp = netobuf;
  nfrontp = good;		/* next byte to be sent */
  neturg = 0;
}				/* end of netclear */

/*
 *  netflush
 *		Send as much data as possible to the network,
 *	handling requests for urgent data.
 */
void
netflush (void)
{
  int n;

  if ((n = nfrontp - nbackp) > 0)
    {
      NET_ENCRYPT ();

      /*
       * if no urgent data, or if the other side appears to be an
       * old 4.2 client (and thus unable to survive TCP urgent data),
       * write the entire buffer in non-OOB mode.
       */
      if (!neturg || !not42)
	n = write (net, nbackp, n);	/* normal write */
      else
	{
	  n = neturg - nbackp;
	  /*
	   * In 4.2 (and 4.3) systems, there is some question about
	   * what byte in a sendOOB operation is the "OOB" data.
	   * To make ourselves compatible, we only send ONE byte
	   * out of band, the one WE THINK should be OOB (though
	   * we really have more the TCP philosophy of urgent data
	   * rather than the Unix philosophy of OOB data).
	   */
	  if (n > 1)
	    n = send (net, nbackp, n - 1, 0);	/* send URGENT all by itself */
	  else
	    n = send (net, nbackp, n, MSG_OOB);	/* URGENT data */
	}
    }
  if (n < 0)
    {
      if (errno == EWOULDBLOCK || errno == EINTR)
	return;
      cleanup (0);
      /* NOT REACHED */
    }

  nbackp += n;
#ifdef	ENCRYPTION
  if (nbackp > nclearto)
    nclearto = 0;
#endif /* ENCRYPTION */
  if (nbackp >= neturg)
    neturg = 0;

  if (nbackp == nfrontp)
    {
      nbackp = nfrontp = netobuf;
#ifdef	ENCRYPTION
      nclearto = 0;
#endif /* ENCRYPTION */
    }
}				/* end of netflush */

/*
 * miscellaneous functions doing a variety of little jobs follow ...
 */

void
fatal (int f, char *msg)
{
  char buf[BUFSIZ];

  snprintf (buf, sizeof buf, "telnetd: %s.\r\n", msg);
#ifdef	ENCRYPTION
  if (encrypt_output)
    {
      /*
       * Better turn off encryption first....
       * Hope it flushes...
       */
      encrypt_send_end ();
      netflush ();
    }
#endif /* ENCRYPTION */
  write (f, buf, (int) strlen (buf));
  sleep (1);
   /*FIXME*/ exit (EXIT_FAILURE);
}

void
fatalperror (int f, char *msg)
{
  char buf[BUFSIZ];

  snprintf (buf, sizeof buf, "%s: %s", msg, strerror (errno));
  fatal (f, buf);
}

/* ************************************************************************* */
/* Terminal determination */

static unsigned char ttytype_sbbuf[] = {
  IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE
};

static void
_gettermname (void)
{
  if (his_state_is_wont (TELOPT_TTYPE))
    return;
  settimer (baseline);
  net_output_datalen (ttytype_sbbuf, sizeof ttytype_sbbuf);
  ttloop (sequenceIs (ttypesubopt, baseline));
}

/*
 * Changes terminaltype.
 */
int
getterminaltype (char *uname, size_t len)
{
  int retval = -1;

  settimer (baseline);
#if defined AUTHENTICATION
  /*
   * Handle the Authentication option before we do anything else.
   * Distinguish the available modes by level:
   *
   *   off:                     Authentication is forbidden.
   *   none:                    Voluntary authentication.
   *   user, valid, other:      Mandatory authentication only.
   */
  if (auth_level < 0)
    send_wont (TELOPT_AUTHENTICATION, 1);
  else
    {
      if (auth_level > 0)
	send_do (TELOPT_AUTHENTICATION, 1);
      else
	send_will (TELOPT_AUTHENTICATION, 1);

      ttloop (his_will_wont_is_changing (TELOPT_AUTHENTICATION));

      if (his_state_is_will (TELOPT_AUTHENTICATION))
	retval = auth_wait (uname, len);
    }
#else /* !AUTHENTICATION */
  (void) uname;			/* Silence warning.  */
  (void) len;			/* Silence warning.  */
#endif

#ifdef	ENCRYPTION
  send_will (TELOPT_ENCRYPT, 1);
#endif /* ENCRYPTION */
  send_do (TELOPT_TTYPE, 1);
  send_do (TELOPT_TSPEED, 1);
  send_do (TELOPT_XDISPLOC, 1);
  send_do (TELOPT_NEW_ENVIRON, 1);
  send_do (TELOPT_OLD_ENVIRON, 1);

#ifdef ENCRYPTION
  ttloop (his_do_dont_is_changing (TELOPT_ENCRYPT)
	  || his_will_wont_is_changing (TELOPT_TTYPE)
	  || his_will_wont_is_changing (TELOPT_TSPEED)
	  || his_will_wont_is_changing (TELOPT_XDISPLOC)
	  || his_will_wont_is_changing (TELOPT_NEW_ENVIRON)
	  || his_will_wont_is_changing (TELOPT_OLD_ENVIRON));
#else
  ttloop (his_will_wont_is_changing (TELOPT_TTYPE)
	  || his_will_wont_is_changing (TELOPT_TSPEED)
	  || his_will_wont_is_changing (TELOPT_XDISPLOC)
	  || his_will_wont_is_changing (TELOPT_NEW_ENVIRON)
	  || his_will_wont_is_changing (TELOPT_OLD_ENVIRON));
#endif

#ifdef  ENCRYPTION
  if (his_state_is_will (TELOPT_ENCRYPT))
    encrypt_wait ();
#endif

  if (his_state_is_will (TELOPT_TSPEED))
    {
      static unsigned char sb[] =
	{ IAC, SB, TELOPT_TSPEED, TELQUAL_SEND, IAC, SE };

      net_output_datalen (sb, sizeof sb);
    }
  if (his_state_is_will (TELOPT_XDISPLOC))
    {
      static unsigned char sb[] =
	{ IAC, SB, TELOPT_XDISPLOC, TELQUAL_SEND, IAC, SE };

      net_output_datalen (sb, sizeof sb);
    }
  if (his_state_is_will (TELOPT_NEW_ENVIRON))
    {
      static unsigned char sb[] =
	{ IAC, SB, TELOPT_NEW_ENVIRON, TELQUAL_SEND, IAC, SE };

      net_output_datalen (sb, sizeof sb);
    }
  else if (his_state_is_will (TELOPT_OLD_ENVIRON))
    {
      static unsigned char sb[] =
	{ IAC, SB, TELOPT_OLD_ENVIRON, TELQUAL_SEND, IAC, SE };

      net_output_datalen (sb, sizeof sb);
    }
  if (his_state_is_will (TELOPT_TTYPE))
    net_output_datalen (ttytype_sbbuf, sizeof ttytype_sbbuf);

  if (his_state_is_will (TELOPT_TSPEED))
    ttloop (sequenceIs (tspeedsubopt, baseline));
  if (his_state_is_will (TELOPT_XDISPLOC))
    ttloop (sequenceIs (xdisplocsubopt, baseline));
  if (his_state_is_will (TELOPT_NEW_ENVIRON))
    ttloop (sequenceIs (environsubopt, baseline));
  if (his_state_is_will (TELOPT_OLD_ENVIRON))
    ttloop (sequenceIs (oenvironsubopt, baseline));

  if (his_state_is_will (TELOPT_TTYPE))
    {
      char *first = NULL, *last = NULL;

      ttloop (sequenceIs (ttypesubopt, baseline));
      /*
       * If the other side has already disabled the option, then
       * we have to just go with what we (might) have already gotten.
       */
      if (his_state_is_will (TELOPT_TTYPE) && !terminaltypeok (terminaltype))
	{
	  free (first);
	  first = xstrdup (terminaltype);
	  for (;;)
	    {
	      /* Save the unknown name, and request the next name. */
	      free (last);
	      last = xstrdup (terminaltype);
	      _gettermname ();
	      if (terminaltypeok (terminaltype))
		break;
	      if ((strcmp (last, terminaltype) == 0)
		  || his_state_is_wont (TELOPT_TTYPE))
		{
		  /*
		   * We've hit the end.  If this is the same as
		   * the first name, just go with it.
		   */
		  if (strcmp (first, terminaltype) == 0)
		    break;
		  /*
		   * Get the terminal name one more time, so that
		   * RFC1091 compliant telnets will cycle back to
		   * the start of the list.
		   */
		  _gettermname ();
		  if (strcmp (first, terminaltype) != 0)
		    {
		      free (terminaltype);
		      terminaltype = xstrdup (first);
		    }
		  break;
		}
	    }
	}
      free (first);
      free (last);
    }
  return retval;
}

/*
 * Exit status:
 *
 *  1   Accepted terminal type, or inconclusive,
 *  0   Explicitly unsupported type.
 */
int
terminaltypeok (char *s)
{
#ifdef HAVE_TGETENT
  char buf[2048];

  if (terminaltype == NULL)
    return 1;

  if (tgetent (buf, s) == 0)
    return 0;
#endif /* HAVE_TGETENT */

  return 1;
}

#if defined AUTHENTICATION || defined ENCRYPTION

/* libtelnet needs this function.  */

void
printsub (int direction MAYBE_UNUSED, unsigned char *pointer MAYBE_UNUSED,
	  int length MAYBE_UNUSED)
{
}

int
net_write (unsigned char *str, int len)
{
  return net_output_datalen (str, len);
}

void
net_encrypt (void)
{
# ifdef	ENCRYPTION
  char *s = (nclearto > nbackp) ? nclearto : nbackp;
  if (s < nfrontp && encrypt_output)
    (*encrypt_output) ((unsigned char *) s, nfrontp - s);
  nclearto = nfrontp;
# endif/* ENCRYPTION */
}

int
telnet_spin (void)
{
  io_drain ();
  return 0;
}


#endif

/* ************************************************************************* */
/* String expansion functions */

#define EXP_STATE_CONTINUE 0
#define EXP_STATE_SUCCESS  1
#define EXP_STATE_ERROR    2

struct line_expander
{
  int state;			/* Current state */
  int level;			/* The nesting level */
  char *source;			/* The source string */
  char *cp;			/* Current position in the source */
  struct obstack stk;		/* Obstack for expanded version */
};

static char *_var_short_name (struct line_expander *exp);
static char *_var_long_name (struct line_expander *exp,
			     char *start, int length);
static char *_expand_var (struct line_expander *exp);
static void _expand_cond (struct line_expander *exp);
static void _skip_block (struct line_expander *exp);
static void _expand_block (struct line_expander *exp);

static const char *
sanitize (const char *u)
{
  /* Ignore values starting with '-' or containing shell metachars, as
     they can cause trouble.  */
  if (u && *u != '-' && !u[strcspn (u, "\t\n !\"#$&'()*;<=>?[\\^`{|}~")])
    return u;
  else
    return "";
}

/* Expand a variable referenced by its short one-symbol name.
   Input: exp->cp points to the variable name.
   FIXME: not implemented */
char *
_var_short_name (struct line_expander *exp)
{
  char *q;
  char timebuf[64];
  time_t t;

  switch (*exp->cp++)
    {
    case 'a':
#ifdef AUTHENTICATION
      if (auth_level >= 0 && autologin == AUTH_VALID)
	return xstrdup ("ok");
#endif
      return NULL;

    case 'd':
      time (&t);
      strftime (timebuf, sizeof (timebuf),
		"%l:%M%p on %A, %d %B %Y", localtime (&t));
      return xstrdup (timebuf);

    case 'h':
      return xstrdup (sanitize (remote_hostname));

    case 'l':
      return xstrdup (sanitize (local_hostname));

    case 'L':
      return xstrdup (sanitize (line));

    case 't':
      q = strchr (line + 1, '/');
      if (q)
	q++;
      else
	q = line;
      return xstrdup (sanitize (q));

    case 'T':
      return terminaltype ? xstrdup (sanitize (terminaltype)) : NULL;

    case 'u':
      return user_name ? xstrdup (sanitize (user_name)) : NULL;

    case 'U':
      return xstrdup (sanitize (getenv ("USER")));

    default:
      exp->state = EXP_STATE_ERROR;
      return NULL;
    }
}

/* Expand a variable referenced by its long name.
   Input: exp->cp points to initial '('
   FIXME: not implemented */
char *
_var_long_name (struct line_expander *exp, char *start, int length)
{
  (void) start;			/* Silence warnings until implemented.  */
  (void) length;
  exp->state = EXP_STATE_ERROR;
  return NULL;
}

/* Expand a variable to its value.
   Input: exp->cp points one character _past_ % (or ?) */
char *
_expand_var (struct line_expander *exp)
{
  char *p;
  switch (*exp->cp)
    {
    case '{':
      /* Collect variable name */
      for (p = ++exp->cp; *exp->cp && *exp->cp != '}'; exp->cp++)
	;
      if (*exp->cp == 0)
	{
	  exp->cp = p;
	  exp->state = EXP_STATE_ERROR;
	  break;
	}
      p = _var_long_name (exp, p, exp->cp - p);
      exp->cp++;
      break;

    default:
      p = _var_short_name (exp);
      break;
    }
  return p;
}

/* Expand a conditional block. A conditional block is:
       %?<var>{true-stmt}[{false-stmt}]
   <var> may be either a one-symbol variable name or (string). The latter
   is not handled yet.
   On input exp->cp points to % character */
void
_expand_cond (struct line_expander *exp)
{
  char *p;

  if (*++exp->cp == '?')
    {
      /* condition */
      exp->cp++;
      p = _expand_var (exp);
      if (p)
	{
	  _expand_block (exp);
	  _skip_block (exp);
	}
      else
	{
	  _skip_block (exp);
	  _expand_block (exp);
	}
      free (p);
    }
  else
    {
      p = _expand_var (exp);
      if (p)
	obstack_grow (&exp->stk, p, strlen (p));
      free (p);
    }
}

/* Skip the block. If the exp->cp does not point to the beginning of a
   block ({ character), the function does nothing */
void
_skip_block (struct line_expander *exp)
{
  int level = exp->level;
  if (*exp->cp != '{')
    return;
  for (; *exp->cp; exp->cp++)
    {
      switch (*exp->cp)
	{
	case '{':
	  exp->level++;
	  break;

	case '}':
	  exp->level--;
	  if (exp->level == level)
	    {
	      exp->cp++;
	      return;
	    }
	}
    }
}

/* Expand a block within the formatted line. Stops either when end of source
   line was reached or the nesting reaches the initial value */
void
_expand_block (struct line_expander *exp)
{
  int level = exp->level;
  if (*exp->cp == '{')
    {
      exp->level++;
      exp->cp++;		/*FIXME? */
    }
  while (exp->state == EXP_STATE_CONTINUE)
    {
      for (; *exp->cp && *exp->cp != '%'; exp->cp++)
	{
	  switch (*exp->cp)
	    {
	    case '{':
	      exp->level++;
	      break;

	    case '}':
	      exp->level--;
	      if (exp->level == level)
		{
		  exp->cp++;
		  return;
		}
	      break;

	    case '\\':
	      exp->cp++;
	      break;
	    }
	  obstack_1grow (&exp->stk, *exp->cp);
	}

      if (*exp->cp == 0)
	{
	  obstack_1grow (&exp->stk, 0);
	  exp->state = EXP_STATE_SUCCESS;
	  break;
	}
      else if (*exp->cp == '%' && exp->cp[1] == '%')
	{
	  obstack_1grow (&exp->stk, *exp->cp);
	  exp->cp += 2;
	  continue;
	}

      _expand_cond (exp);
    }
}

/* Expand a format line */
char *
expand_line (const char *line)
{
  char *p = NULL;
  struct line_expander exp;

  exp.state = EXP_STATE_CONTINUE;
  exp.level = 0;
  exp.source = (char *) line;
  exp.cp = (char *) line;
  obstack_init (&exp.stk);
  _expand_block (&exp);
  if (exp.state == EXP_STATE_SUCCESS)
    p = xstrdup (obstack_finish (&exp.stk));
  else
    {
      syslog (LOG_ERR, "can't expand line: %s", line);
      syslog (LOG_ERR, "stopped near %s", exp.cp ? exp.cp : "(END)");
    }
  obstack_free (&exp.stk, NULL);
  return p;
}
