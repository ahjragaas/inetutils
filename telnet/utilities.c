/*
  Copyright (C) 1995-2025 Free Software Foundation, Inc.

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
 * Copyright (c) 1988, 1993
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

#include <config.h>

#include <stdlib.h>

#define TELOPTS
#define TELCMDS
#define SLC_NAMES
#include <arpa/telnet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

#include "general.h"
#include "ring.h"
#include "defines.h"
#include "externs.h"

#ifdef AUTHENTICATION
# include <libtelnet/auth.h>
#endif
#ifdef ENCRYPTION
# include <libtelnet/encrypt.h>
#endif

FILE *NetTrace = 0;		/* Not in bss, since needs to stay */
int prettydump;

/*
 * upcase()
 *
 *	Upcase (in place) the argument.
 */

void
upcase (char *argument)
{
  int c;

  while ((c = *argument) != 0)
    {
      if (islower (c))
	{
	  *argument = toupper (c);
	}
      argument++;
    }
}

/*
 * SetSockOpt()
 *
 * Compensate for differences in 4.2 and 4.3 systems.
 */

int
SetSockOpt (int fd, int level, int option, int yesno)
{
#ifndef NOT43
  return setsockopt (fd, level, option, (char *) &yesno, sizeof yesno);
#else /* NOT43 */
  if (yesno == 0)
    {				/* Can't do that in 4.2! */
      fprintf (stderr, "Error: attempt to turn off an option 0x%x.\n",
	       option);
      return -1;
    }
  return setsockopt (fd, level, option, 0, 0);
#endif /* NOT43 */
}

/*
 * The following are routines used to print out debugging information.
 */

unsigned char NetTraceFile[256] = "(standard output)";

void
SetNetTrace (const char *file)
{
  if (NetTrace && NetTrace != stdout)
    fclose (NetTrace);
  if (file && (strcmp (file, "-") != 0))
    {
      NetTrace = fopen (file, "w");
      if (NetTrace)
	{
	  strncpy ((char *) NetTraceFile, file, sizeof (NetTraceFile));
	  NetTraceFile[sizeof (NetTraceFile) - 1] = 0;
	  return;
	}
      fprintf (stderr, "Cannot open %s.\n", file);
    }
  NetTrace = stdout;
  strcpy ((char *) NetTraceFile, "(standard output)");
}

void
Dump (char direction, unsigned char *buffer, int length)
{
#define BYTES_PER_LINE	32
#define min(x,y)	((x<y)? x:y)
  unsigned char *pThis;
  int offset;

  offset = 0;

  while (length)
    {
      /* print one line */
      fprintf (NetTrace, "%c 0x%x\t", direction, offset);
      pThis = buffer;
      if (prettydump)
	{
	  buffer = buffer + min (length, BYTES_PER_LINE / 2);
	  while (pThis < buffer)
	    {
	      fprintf (NetTrace, "%c%.2x",
		       (((*pThis) & 0xff) == 0xff) ? '*' : ' ',
		       (*pThis) & 0xff);
	      pThis++;
	    }
	  length -= BYTES_PER_LINE / 2;
	  offset += BYTES_PER_LINE / 2;
	}
      else
	{
	  buffer = buffer + min (length, BYTES_PER_LINE);
	  while (pThis < buffer)
	    {
	      fprintf (NetTrace, "%.2x", (*pThis) & 0xff);
	      pThis++;
	    }
	  length -= BYTES_PER_LINE;
	  offset += BYTES_PER_LINE;
	}
      if (NetTrace == stdout)
	{
	  fprintf (NetTrace, "\r\n");
	}
      else
	{
	  fprintf (NetTrace, "\n");
	}
      if (length < 0)
	{
	  fflush (NetTrace);
	  return;
	}
      /* find next unique line */
    }
  fflush (NetTrace);
}


void
printoption (char *direction, int cmd, int option)
{
  if (!showoptions)
    return;
  if (cmd == IAC)
    {
      if (TELCMD_OK (option))
	fprintf (NetTrace, "%s IAC %s", direction, TELCMD (option));
      else
	fprintf (NetTrace, "%s IAC %d", direction, option);
    }
  else
    {
      char *fmt;
      fmt = (cmd == WILL)
	? "WILL" : (cmd == WONT)
	? "WONT" : (cmd == DO) ? "DO" : (cmd == DONT) ? "DONT" : 0;
      if (fmt)
	{
	  fprintf (NetTrace, "%s %s ", direction, fmt);
	  if (TELOPT_OK (option))
	    fprintf (NetTrace, "%s", TELOPT (option));
	  else if (option == TELOPT_EXOPL)
	    fprintf (NetTrace, "EXOPL");
	  else
	    fprintf (NetTrace, "%d", option);
	}
      else
	fprintf (NetTrace, "%s %d %d", direction, cmd, option);
    }
  if (NetTrace == stdout)
    {
      fprintf (NetTrace, "\r\n");
      fflush (NetTrace);
    }
  else
    {
      fprintf (NetTrace, "\n");
    }
  return;
}

void
optionstatus (void)
{
  int i;
  extern char will_wont_resp[], do_dont_resp[];

  for (i = 0; i < 256; i++)
    {
      if (do_dont_resp[i])
	{
	  if (TELOPT_OK (i))
	    printf ("resp DO_DONT %s: %d\n", TELOPT (i), do_dont_resp[i]);
	  else if (TELCMD_OK (i))
	    printf ("resp DO_DONT %s: %d\n", TELCMD (i), do_dont_resp[i]);
	  else
	    printf ("resp DO_DONT %d: %d\n", i, do_dont_resp[i]);
	  if (my_want_state_is_do (i))
	    {
	      if (TELOPT_OK (i))
		printf ("want DO   %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("want DO   %s\n", TELCMD (i));
	      else
		printf ("want DO   %d\n", i);
	    }
	  else
	    {
	      if (TELOPT_OK (i))
		printf ("want DONT %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("want DONT %s\n", TELCMD (i));
	      else
		printf ("want DONT %d\n", i);
	    }
	}
      else
	{
	  if (my_state_is_do (i))
	    {
	      if (TELOPT_OK (i))
		printf ("     DO   %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("     DO   %s\n", TELCMD (i));
	      else
		printf ("     DO   %d\n", i);
	    }
	}
      if (will_wont_resp[i])
	{
	  if (TELOPT_OK (i))
	    printf ("resp WILL_WONT %s: %d\n", TELOPT (i), will_wont_resp[i]);
	  else if (TELCMD_OK (i))
	    printf ("resp WILL_WONT %s: %d\n", TELCMD (i), will_wont_resp[i]);
	  else
	    printf ("resp WILL_WONT %d: %d\n", i, will_wont_resp[i]);
	  if (my_want_state_is_will (i))
	    {
	      if (TELOPT_OK (i))
		printf ("want WILL %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("want WILL %s\n", TELCMD (i));
	      else
		printf ("want WILL %d\n", i);
	    }
	  else
	    {
	      if (TELOPT_OK (i))
		printf ("want WONT %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("want WONT %s\n", TELCMD (i));
	      else
		printf ("want WONT %d\n", i);
	    }
	}
      else
	{
	  if (my_state_is_will (i))
	    {
	      if (TELOPT_OK (i))
		printf ("     WILL %s\n", TELOPT (i));
	      else if (TELCMD_OK (i))
		printf ("     WILL %s\n", TELCMD (i));
	      else
		printf ("     WILL %d\n", i);
	    }
	}
    }

}

/* char direction; '<' or '>' */
/* unsigned char *pointer; where suboption data sits */
/* int		  length; length of suboption data */
void
printsub (char direction, unsigned char *pointer, int length)
{
  int i;
  extern int want_status_response;

#if defined AUTHENTICATION || defined ENCRYPTION
  char buf[512];
#endif

  if (showoptions || direction == 0 ||
      (want_status_response && (pointer[0] == TELOPT_STATUS)))
    {
      if (direction)
	{
	  fprintf (NetTrace, "%s IAC SB ",
		   (direction == '<') ? "RCVD" : "SENT");
	  if (length >= 3)
	    {
	      int j;

	      i = pointer[length - 2];
	      j = pointer[length - 1];

	      if (i != IAC || j != SE)
		{
		  fprintf (NetTrace, "(terminated by ");
		  if (TELOPT_OK (i))
		    fprintf (NetTrace, "%s ", TELOPT (i));
		  else if (TELCMD_OK (i))
		    fprintf (NetTrace, "%s ", TELCMD (i));
		  else
		    fprintf (NetTrace, "%d ", i);
		  if (TELOPT_OK (j))
		    fprintf (NetTrace, "%s", TELOPT (j));
		  else if (TELCMD_OK (j))
		    fprintf (NetTrace, "%s", TELCMD (j));
		  else
		    fprintf (NetTrace, "%d", j);
		  fprintf (NetTrace, ", not IAC SE!) ");
		}
	    }
	  length -= 2;
	}
      if (length < 1)
	{
	  fprintf (NetTrace, "(Empty suboption??\?)");
	  if (NetTrace == stdout)
	    fflush (NetTrace);
	  return;
	}
      switch (pointer[0])
	{
	case TELOPT_TTYPE:
	  fprintf (NetTrace, "TERMINAL-TYPE ");
	  switch (pointer[1])
	    {
	    case TELQUAL_IS:
	      fprintf (NetTrace, "IS \"%.*s\"", length - 2,
		       (char *) pointer + 2);
	      break;
	    case TELQUAL_SEND:
	      fprintf (NetTrace, "SEND");
	      break;
	    default:
	      fprintf (NetTrace, "- unknown qualifier %d (0x%x).",
		       pointer[1], pointer[1]);
	    }
	  break;
	case TELOPT_TSPEED:
	  fprintf (NetTrace, "TERMINAL-SPEED");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  switch (pointer[1])
	    {
	    case TELQUAL_IS:
	      fprintf (NetTrace, " IS ");
	      fprintf (NetTrace, "%.*s", length - 2, (char *) pointer + 2);
	      break;
	    default:
	      if (pointer[1] == 1)
		fprintf (NetTrace, " SEND");
	      else
		fprintf (NetTrace, " %d (unknown)", pointer[1]);
	      for (i = 2; i < length; i++)
		fprintf (NetTrace, " ?%d?", pointer[i]);
	      break;
	    }
	  break;

	case TELOPT_LFLOW:
	  fprintf (NetTrace, "TOGGLE-FLOW-CONTROL");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  switch (pointer[1])
	    {
	    case LFLOW_OFF:
	      fprintf (NetTrace, " OFF");
	      break;
	    case LFLOW_ON:
	      fprintf (NetTrace, " ON");
	      break;
	    case LFLOW_RESTART_ANY:
	      fprintf (NetTrace, " RESTART-ANY");
	      break;
	    case LFLOW_RESTART_XON:
	      fprintf (NetTrace, " RESTART-XON");
	      break;
	    default:
	      fprintf (NetTrace, " %d (unknown)", pointer[1]);
	    }
	  for (i = 2; i < length; i++)
	    fprintf (NetTrace, " ?%d?", pointer[i]);
	  break;

	case TELOPT_NAWS:
	  fprintf (NetTrace, "NAWS");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  if (length == 2)
	    {
	      fprintf (NetTrace, " ?%d?", pointer[1]);
	      break;
	    }
	  fprintf (NetTrace, " %d %d (%d)",
		   pointer[1], pointer[2],
		   (int) ((((unsigned int) pointer[1]) << 8) |
			  ((unsigned int) pointer[2])));
	  if (length == 4)
	    {
	      fprintf (NetTrace, " ?%d?", pointer[3]);
	      break;
	    }
	  fprintf (NetTrace, " %d %d (%d)",
		   pointer[3], pointer[4],
		   (int) ((((unsigned int) pointer[3]) << 8) |
			  ((unsigned int) pointer[4])));
	  for (i = 5; i < length; i++)
	    fprintf (NetTrace, " ?%d?", pointer[i]);
	  break;

#if defined AUTHENTICATION
	case TELOPT_AUTHENTICATION:
	  fprintf (NetTrace, "AUTHENTICATION");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  switch (pointer[1])
	    {
	    case TELQUAL_REPLY:
	    case TELQUAL_IS:
	      fprintf (NetTrace, " %s ",
		       (pointer[1] == TELQUAL_IS) ? "IS" : "REPLY");
	      if (AUTHTYPE_NAME_OK (pointer[2]) && AUTHTYPE_NAME (pointer[2]))
		fprintf (NetTrace, "%s ", AUTHTYPE_NAME (pointer[2]));
	      else
		fprintf (NetTrace, "%d ", pointer[2]);
	      if (length < 3)
		{
		  fprintf (NetTrace, "(partial suboption??\?)");
		  break;
		}
	      fprintf (NetTrace, "%s|%s",
		       ((pointer[3] & AUTH_WHO_MASK) == AUTH_WHO_CLIENT)
		       ? "CLIENT" : "SERVER",
		       ((pointer[3] & AUTH_HOW_MASK) == AUTH_HOW_MUTUAL)
		       ? "MUTUAL" : "ONE-WAY");

	      auth_printsub (&pointer[1], length - 1, buf, sizeof (buf));
	      fprintf (NetTrace, "%s", buf);
	      break;

	    case TELQUAL_SEND:
	      i = 2;
	      fprintf (NetTrace, " SEND ");
	      while (i < length)
		{
		  if (AUTHTYPE_NAME_OK (pointer[i])
		      && AUTHTYPE_NAME (pointer[i]))
		    fprintf (NetTrace, "%s ", AUTHTYPE_NAME (pointer[i]));
		  else
		    fprintf (NetTrace, "%d ", pointer[i]);
		  if (++i >= length)
		    {
		      fprintf (NetTrace, "(partial suboption??\?)");
		      break;
		    }
		  fprintf (NetTrace, "%s|%s ",
			   ((pointer[i] & AUTH_WHO_MASK) == AUTH_WHO_CLIENT)
			   ? "CLIENT" : "SERVER",
			   ((pointer[i] & AUTH_HOW_MASK) == AUTH_HOW_MUTUAL)
			   ? "MUTUAL" : "ONE-WAY");
		  ++i;
		}
	      break;

	    case TELQUAL_NAME:
	      i = 2;
	      fprintf (NetTrace, " NAME \"");
	      while (i < length)
		putc (pointer[i++], NetTrace);
	      putc ('"', NetTrace);
	      break;

	    default:
	      for (i = 2; i < length; i++)
		fprintf (NetTrace, " ?%d?", pointer[i]);
	      break;
	    }
	  break;
#endif

#ifdef	ENCRYPTION
	case TELOPT_ENCRYPT:
	  fprintf (NetTrace, "ENCRYPT");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  switch (pointer[1])
	    {
	    case ENCRYPT_START:
	      fprintf (NetTrace, " START");
	      break;

	    case ENCRYPT_END:
	      fprintf (NetTrace, " END");
	      break;

	    case ENCRYPT_REQSTART:
	      fprintf (NetTrace, " REQUEST-START");
	      break;

	    case ENCRYPT_REQEND:
	      fprintf (NetTrace, " REQUEST-END");
	      break;

	    case ENCRYPT_IS:
	    case ENCRYPT_REPLY:
	      fprintf (NetTrace, " %s ",
		       (pointer[1] == ENCRYPT_IS) ? "IS" : "REPLY");
	      if (length < 3)
		{
		  fprintf (NetTrace, " (partial suboption??\?)");
		  break;
		}
	      if (ENCTYPE_NAME_OK (pointer[2]) && ENCTYPE_NAME (pointer[2]))
		fprintf (NetTrace, "%s ", ENCTYPE_NAME (pointer[2]));
	      else
		fprintf (NetTrace, " %d (unknown)", pointer[2]);

	      encrypt_printsub (&pointer[1], length - 1, buf, sizeof (buf));
	      fprintf (NetTrace, "%s", buf);
	      break;

	    case ENCRYPT_SUPPORT:
	      i = 2;
	      fprintf (NetTrace, " SUPPORT ");
	      while (i < length)
		{
		  if (ENCTYPE_NAME_OK (pointer[i])
		      && ENCTYPE_NAME (pointer[i]))
		    fprintf (NetTrace, "%s ", ENCTYPE_NAME (pointer[i]));
		  else
		    fprintf (NetTrace, "%d ", pointer[i]);
		  i++;
		}
	      break;

	    case ENCRYPT_ENC_KEYID:
	      fprintf (NetTrace, " ENC_KEYID ");
	      goto encommon;

	    case ENCRYPT_DEC_KEYID:
	      fprintf (NetTrace, " DEC_KEYID ");
	      goto encommon;

	    default:
	      fprintf (NetTrace, " %d (unknown)", pointer[1]);
	    encommon:
	      for (i = 2; i < length; i++)
		fprintf (NetTrace, " %d", pointer[i]);
	      break;
	    }
	  break;
#endif /* ENCRYPTION */

	case TELOPT_LINEMODE:
	  fprintf (NetTrace, "LINEMODE ");
	  if (length < 2)
	    {
	      fprintf (NetTrace, " (empty suboption??\?)");
	      break;
	    }
	  switch (pointer[1])
	    {
	    case WILL:
	      fprintf (NetTrace, "WILL ");
	      goto common;
	    case WONT:
	      fprintf (NetTrace, "WONT ");
	      goto common;
	    case DO:
	      fprintf (NetTrace, "DO ");
	      goto common;
	    case DONT:
	      fprintf (NetTrace, "DONT ");
	    common:
	      if (length < 3)
		{
		  fprintf (NetTrace, "(no option??\?)");
		  break;
		}
	      switch (pointer[2])
		{
		case LM_FORWARDMASK:
		  fprintf (NetTrace, "Forward Mask");
		  for (i = 3; i < length; i++)
		    fprintf (NetTrace, " %x", pointer[i]);
		  break;
		default:
		  fprintf (NetTrace, "%d (unknown)", pointer[2]);
		  for (i = 3; i < length; i++)
		    fprintf (NetTrace, " %d", pointer[i]);
		  break;
		}
	      break;

	    case LM_SLC:
	      fprintf (NetTrace, "SLC");
	      for (i = 2; i < length - 2; i += 3)
		{
		  if (SLC_NAME_OK (pointer[i + SLC_FUNC]))
		    fprintf (NetTrace, " %s",
			     SLC_NAME (pointer[i + SLC_FUNC]));
		  else
		    fprintf (NetTrace, " %d", pointer[i + SLC_FUNC]);
		  switch (pointer[i + SLC_FLAGS] & SLC_LEVELBITS)
		    {
		    case SLC_NOSUPPORT:
		      fprintf (NetTrace, " NOSUPPORT");
		      break;
		    case SLC_CANTCHANGE:
		      fprintf (NetTrace, " CANTCHANGE");
		      break;
		    case SLC_VARIABLE:
		      fprintf (NetTrace, " VARIABLE");
		      break;
		    case SLC_DEFAULT:
		      fprintf (NetTrace, " DEFAULT");
		      break;
		    }
		  fprintf (NetTrace, "%s%s%s",
			   (pointer[i + SLC_FLAGS] & SLC_ACK)
			   ? "|ACK" : "",
			   (pointer[i + SLC_FLAGS] & SLC_FLUSHIN)
			   ? "|FLUSHIN" : "",
			   (pointer[i + SLC_FLAGS] & SLC_FLUSHOUT)
			   ? "|FLUSHOUT" : "");
		  if (pointer[i + SLC_FLAGS] &
		      ~(SLC_ACK | SLC_FLUSHIN | SLC_FLUSHOUT | SLC_LEVELBITS))
		    fprintf (NetTrace, "(0x%x)", pointer[i + SLC_FLAGS]);
		  fprintf (NetTrace, " %d;", pointer[i + SLC_VALUE]);
		  if ((pointer[i + SLC_VALUE] == IAC) &&
		      (pointer[i + SLC_VALUE + 1] == IAC))
		    i++;
		}
	      for (; i < length; i++)
		fprintf (NetTrace, " ?%d?", pointer[i]);
	      break;

	    case LM_MODE:
	      fprintf (NetTrace, "MODE ");
	      if (length < 3)
		{
		  fprintf (NetTrace, "(no mode??\?)");
		  break;
		}
	      {
		char tbuf[64];

		snprintf (tbuf, sizeof (tbuf), "%s%s%s%s%s",
			  pointer[2] & MODE_EDIT ? "|EDIT" : "",
			  pointer[2] & MODE_TRAPSIG ? "|TRAPSIG" : "",
			  pointer[2] & MODE_SOFT_TAB ? "|SOFT_TAB" : "",
			  pointer[2] & MODE_LIT_ECHO ? "|LIT_ECHO" : "",
			  pointer[2] & MODE_ACK ? "|ACK" : "");
		fprintf (NetTrace, "%s", tbuf[0] ? &tbuf[1] : "0");
	      }
	      if (pointer[2] & ~(MODE_MASK))
		fprintf (NetTrace, " (0x%x)", pointer[2]);
	      for (i = 3; i < length; i++)
		fprintf (NetTrace, " ?0x%x?", pointer[i]);
	      break;
	    default:
	      fprintf (NetTrace, "%d (unknown)", pointer[1]);
	      for (i = 2; i < length; i++)
		fprintf (NetTrace, " %d", pointer[i]);
	    }
	  break;

	case TELOPT_STATUS:
	  {
	    char *cp;
	    int j, k;

	    fprintf (NetTrace, "STATUS");

	    switch (pointer[1])
	      {
	      default:
		if (pointer[1] == TELQUAL_SEND)
		  fprintf (NetTrace, " SEND");
		else
		  fprintf (NetTrace, " %d (unknown)", pointer[1]);
		for (i = 2; i < length; i++)
		  fprintf (NetTrace, " ?%d?", pointer[i]);
		break;
	      case TELQUAL_IS:
		if (--want_status_response < 0)
		  want_status_response = 0;
		if (NetTrace == stdout)
		  fprintf (NetTrace, " IS\r\n");
		else
		  fprintf (NetTrace, " IS\n");

		for (i = 2; i < length; i++)
		  {
		    switch (pointer[i])
		      {
		      case DO:
			cp = "DO";
			goto common2;
		      case DONT:
			cp = "DONT";
			goto common2;
		      case WILL:
			cp = "WILL";
			goto common2;
		      case WONT:
			cp = "WONT";
			goto common2;
		      common2:
			i++;
			if (TELOPT_OK ((int) pointer[i]))
			  fprintf (NetTrace, " %s %s", cp,
				   TELOPT (pointer[i]));
			else
			  fprintf (NetTrace, " %s %d", cp, pointer[i]);

			if (NetTrace == stdout)
			  fprintf (NetTrace, "\r\n");
			else
			  fprintf (NetTrace, "\n");
			break;

		      case SB:
			fprintf (NetTrace, " SB ");
			i++;
			j = k = i;
			while (j < length)
			  {
			    if (pointer[j] == SE)
			      {
				if (j + 1 == length)
				  break;
				if (pointer[j + 1] == SE)
				  j++;
				else
				  break;
			      }
			    pointer[k++] = pointer[j++];
			  }
			printsub (0, &pointer[i], k - i);
			if (i < length)
			  {
			    fprintf (NetTrace, " SE");
			    i = j;
			  }
			else
			  i = j - 1;

			if (NetTrace == stdout)
			  fprintf (NetTrace, "\r\n");
			else
			  fprintf (NetTrace, "\n");

			break;

		      default:
			fprintf (NetTrace, " %d", pointer[i]);
			break;
		      }
		  }
		break;
	      }
	    break;
	  }

	case TELOPT_XDISPLOC:
	  fprintf (NetTrace, "X-DISPLAY-LOCATION ");
	  switch (pointer[1])
	    {
	    case TELQUAL_IS:
	      fprintf (NetTrace, "IS \"%.*s\"", length - 2,
		       (char *) pointer + 2);
	      break;
	    case TELQUAL_SEND:
	      fprintf (NetTrace, "SEND");
	      break;
	    default:
	      fprintf (NetTrace, "- unknown qualifier %d (0x%x).",
		       pointer[1], pointer[1]);
	    }
	  break;

	case TELOPT_NEW_ENVIRON:
	  fprintf (NetTrace, "NEW-ENVIRON ");
#ifdef	OLD_ENVIRON
	  goto env_common1;
	case TELOPT_OLD_ENVIRON:
	  fprintf (NetTrace, "OLD-ENVIRON");
	env_common1:
#endif
	  switch (pointer[1])
	    {
	    case TELQUAL_IS:
	      fprintf (NetTrace, "IS ");
	      goto env_common;
	    case TELQUAL_SEND:
	      fprintf (NetTrace, "SEND ");
	      goto env_common;
	    case TELQUAL_INFO:
	      fprintf (NetTrace, "INFO ");
	    env_common:
	      {
		const char *quote = "";
#if defined ENV_HACK && defined OLD_ENVIRON
		extern int old_env_var, old_env_value;
#endif
		for (i = 2; i < length; i++)
		  {
		    switch (pointer[i])
		      {
		      case NEW_ENV_VALUE:
#ifdef OLD_ENVIRON
			/* case NEW_ENV_OVAR: */
			if (pointer[0] == TELOPT_OLD_ENVIRON)
			  {
# ifdef	ENV_HACK
			    if (old_env_var == OLD_ENV_VALUE)
			      fprintf (NetTrace, "%s(VALUE) ", quote);
			    else
# endif
			      fprintf (NetTrace, "%sVAR ", quote);
			  }
			else
#endif /* OLD_ENVIRON */
			  fprintf (NetTrace, "%sVALUE ", quote);
			quote = "";
			break;

		      case NEW_ENV_VAR:
#ifdef OLD_ENVIRON
			/* case OLD_ENV_VALUE: */
			if (pointer[0] == TELOPT_OLD_ENVIRON)
			  {
# ifdef	ENV_HACK
			    if (old_env_value == OLD_ENV_VAR)
			      fprintf (NetTrace, "%s(VAR) ", quote);
			    else
# endif
			      fprintf (NetTrace, "%sVALUE ", quote);
			  }
			else
#endif /* OLD_ENVIRON */
			  fprintf (NetTrace, "%sVAR ", quote);
			quote = "";
			break;

		      case ENV_ESC:
			fprintf (NetTrace, "%sESC ", quote);
			quote = "";
			break;

		      case ENV_USERVAR:
			fprintf (NetTrace, "%sUSERVAR ", quote);
			quote = "";
			break;

		      default:
			if (isprint (pointer[i]) && pointer[i] != '"')
			  {
			    if (quote[0] == '\0')
			      {
				putc ('"', NetTrace);
				quote = "\" ";
			      }
			    putc (pointer[i], NetTrace);
			  }
			else
			  {
			    fprintf (NetTrace, "%s%03o ", quote, pointer[i]);
			    quote = "";
			  }
			break;
		      }
		  }
		if (quote[0] != '\0')
		  putc ('"', NetTrace);
		break;
	      }
	    }
	  break;

	default:
	  if (TELOPT_OK (pointer[0]))
	    fprintf (NetTrace, "%s (unknown)", TELOPT (pointer[0]));
	  else
	    fprintf (NetTrace, "%d (unknown)", pointer[0]);
	  for (i = 1; i < length; i++)
	    fprintf (NetTrace, " %d", pointer[i]);
	  break;
	}
      if (direction)
	{
	  if (NetTrace == stdout)
	    fprintf (NetTrace, "\r\n");
	  else
	    fprintf (NetTrace, "\n");
	}
      if (NetTrace == stdout)
	fflush (NetTrace);
    }
}

/* EmptyTerminal - called to make sure that the terminal buffer is empty.
 *			Note that we consider the buffer to run all the
 *			way to the kernel (thus the select).
 */

void
EmptyTerminal (void)
{
#if defined unix || defined __unix || defined __unix__
  fd_set o;

  FD_ZERO (&o);
#endif /* unix || __unix || __unix__ */

  if (TTYBYTES () == 0)
    {
#if defined unix || defined __unix || defined __unix__
      FD_SET (tout, &o);
      select (tout + 1, (fd_set *) 0, &o, (fd_set *) 0, (struct timeval *) 0);	/* wait for TTLOWAT */
#endif /* unix || __unix || __unix__ */
    }
  else
    {
      while (TTYBYTES ())
	{
	  ttyflush (0);
#if defined unix || defined __unix || defined __unix__
	  FD_SET (tout, &o);
	  select (tout + 1, (fd_set *) 0, &o, (fd_set *) 0, (struct timeval *) 0);	/* wait for TTLOWAT */
#endif /* unix || __unix || __unix__ */
	}
    }
}

void
SetForExit (void)
{
  setconnmode (0);
#if defined TN3270
  if (In3270)
    {
      Finish3270 ();
    }
#else /* defined(TN3270) */
  do
    {
      telrcv ();		/* Process any incoming data */
      EmptyTerminal ();
    }
  while (ring_full_count (&netiring));	/* While there is any */
#endif /* defined(TN3270) */
  setcommandmode ();
  fflush (stdout);
  fflush (stderr);
#if defined TN3270
  if (In3270)
    {
      StopScreen (1);
    }
#endif /* defined(TN3270) */
  setconnmode (0);
  EmptyTerminal ();		/* Flush the path to the tty */
  setcommandmode ();
}

void
Exit (int returnCode)
{
  SetForExit ();
  exit (returnCode);
}

void
ExitString (char *string, int returnCode)
{
  fwrite (string, 1, strlen (string), stderr);
  exit (returnCode);
}
