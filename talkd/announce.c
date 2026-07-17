/*
  Copyright (C) 1994-2026 Free Software Foundation, Inc.

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

#include <intalkd.h>
#include <stdarg.h>
#include <sys/uio.h>

#undef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

#include <libinetutils.h>

#include "intprops.h"
#include "inttostr.h"

static int
print_mesg (char *tty, CTL_MSG *request, char *remote_machine)
{
  time_t t;
  time (&t);
  struct tm *tm = localtime (&t);

  char tm_hour_buf[INT_BUFSIZE_BOUND (int)];
  char const *tm_hour_str = inttostr (tm->tm_hour, tm_hour_buf);
  idx_t tm_hour_len = strlen (tm_hour_str);

  char tm_min_buf[INT_BUFSIZE_BOUND (int)];
  char *tm_min_str = inttostr (tm->tm_min, tm_min_buf);
  if (0 <= tm->tm_min && tm->tm_min < 10)
    *--tm_min_str = '0';
  idx_t tm_min_len = strlen (tm_min_str);

  /* "Message from Talk_Daemon@%s at %d:%02d ..."  */
  static const char line1_prefix[] = "Message from Talk_Daemon@";
  static const char line1_part2[] = " at ";
  static const char line1_part3[] = " ...";

  /* Don't subtract the NUL byte from LINE1_PART to make up for the
     ':' character.  */
  idx_t line1_len = ((sizeof line1_prefix - 1) + hostname_len
		     + sizeof line1_part2 + tm_hour_len + tm_min_len
		     + (sizeof line1_part3 - 1));

  idx_t l_name_len = strlen (request->l_name);
  idx_t remote_machine_len = strlen (remote_machine);

  /* "talk: connection requested by %s@%s"  */
  static const char line2_prefix[] = "talk: connection requested by ";

  /* Don't subtract the NUL byte to make up for the '@' character.  */
  idx_t line2_len = sizeof line2_prefix + l_name_len + remote_machine_len;

  /* "talk: respond with:  talk %s@%s"  */
  static const char line3_prefix[] = "talk: respond with:  talk ";

  /* Don't subtract the NUL byte to make up for the '@' character.  */
  idx_t line3_len = sizeof line3_prefix + l_name_len + remote_machine_len;

  /* Get the maximum line length.  */
  idx_t max_line_len = MAX (line1_len, MAX (line2_len, line3_len));

  /* Begin with an alarm followed by CRLF.  Each of the 5 lines is filled
     to MAX_LINE_LEN + 2 with spaces followed by a CRLF.  */
  idx_t size = ((max_line_len + 2 + 2) * 5) + 3;
  char *buf = malloc (size);
  if (!buf)
    {
      syslog (LOG_ERR, "Out of memory");
      exit (EXIT_FAILURE);
    }

  char *p = buf;

  *p++ = '\a';
  *p++ = '\r';
  *p++ = '\n';

  /* Line  0.  */
  memset (p, ' ', max_line_len + 2);
  p += max_line_len + 2;
  *p++ = '\r';
  *p++ = '\n';

  /* Line 1.  */
  memcpy (p, line1_prefix, sizeof line1_prefix - 1);
  p += sizeof line1_prefix - 1;
  memcpy (p, hostname, hostname_len);
  p += hostname_len;
  memcpy (p, line1_part2, sizeof line1_part2 - 1);
  p += sizeof line1_part2 - 1;
  memcpy (p, tm_hour_str, tm_hour_len);
  p += tm_hour_len;
  *p++ = ':';
  memcpy (p, tm_min_str, tm_min_len);
  p += tm_min_len;
  memcpy (p, line1_part3, sizeof line1_part3 - 1);
  p += sizeof line1_part3 - 1;
  idx_t line1_spaces = (max_line_len + 2) - line1_len;
  memset (p, ' ', line1_spaces);
  p += line1_spaces;
  *p++ = '\r';
  *p++ = '\n';

  /* Line 2.  */
  memcpy (p, line2_prefix, sizeof line2_prefix - 1);
  p += sizeof line2_prefix - 1;
  memcpy (p, request->l_name, l_name_len);
  p += l_name_len;
  *p++ = '@';
  memcpy (p, remote_machine, remote_machine_len);
  p += remote_machine_len;
  idx_t line2_spaces = (max_line_len + 2) - line2_len;
  memset (p, ' ', line2_spaces);
  p += line2_spaces;
  *p++ = '\r';
  *p++ = '\n';
  memcpy (p, line3_prefix, sizeof line3_prefix - 1);
  p += sizeof line3_prefix - 1;
  memcpy (p, request->l_name, l_name_len);
  p += l_name_len;
  *p++ = '@';
  memcpy (p, remote_machine, remote_machine_len);
  p += remote_machine_len;

  /* Line 3.  */
  idx_t line3_spaces = (max_line_len + 2) - line3_len;
  memset (p, ' ', line3_spaces);
  p += line3_spaces;
  *p++ = '\r';
  *p++ = '\n';

  /* Line 4.  */
  memset (p, ' ', max_line_len + 2);
  p += max_line_len + 2;
  *p++ = '\r';
  *p++ = '\n';

  struct iovec iovec = {.iov_base = buf,.iov_len = size };

  char *cp;
  if ((cp = inetutils_ttymsg (&iovec, 1, tty, RING_WAIT - 5)) != NULL)
    {
      syslog (LOG_ERR, "%s", cp);
      free (buf);
      return FAILED;
    }
  free (buf);
  return SUCCESS;
}

/* See if the user is accepting messages. If so, announce that
   a talk is requested. */
int
announce (CTL_MSG *request, char *remote_machine)
{
  char *ttypath;
  int len;
  struct stat st;
  int rc;

  len = sizeof (PATH_TTY_PFX) + strlen (request->r_tty) + 2;
  ttypath = malloc (len);
  if (!ttypath)
    {
      syslog (LOG_ERR, "Out of memory");
      exit (EXIT_FAILURE);
    }
  sprintf (ttypath, "%s/%s", PATH_TTY_PFX, request->r_tty);
  rc = stat (ttypath, &st);
  free (ttypath);
  if (rc < 0 || (st.st_mode & S_IWGRP) == 0)
    return PERMISSION_DENIED;
  return print_mesg (request->r_tty, request, remote_machine);
}
