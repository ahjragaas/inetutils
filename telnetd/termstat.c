/*
  Copyright (C) 1993-2025 Free Software Foundation, Inc.

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
 * Copyright (c) 1989, 1993
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

#include "telnetd.h"

#include <fcntl.h>		/* Solaris */

/*
 * local variables
 */
int def_tspeed = -1, def_rspeed = -1;
#ifdef	TIOCSWINSZ
int def_row = 0, def_col = 0;
#endif
static int _terminit = 0;

#if defined CRAY2 && defined UNICOS5
int newmap = 1;			/* nonzero if \n maps to ^M^J */
#endif

/*
 * localstat
 *
 * This function handles all management of linemode.
 *
 * Linemode allows the client to do the local editing of data
 * and send only complete lines to the server.  Linemode state is
 * based on the state of the pty driver.  If the pty is set for
 * external processing, then we can use linemode.  Further, if we
 * can use real linemode, then we can look at the edit control bits
 * in the pty to determine what editing the client should do.
 *
 * Linemode support uses the following state flags to keep track of
 * current and desired linemode state.
 *	alwayslinemode : true if -l was specified on the telnetd
 *	command line.  It means to have linemode on as much as
 *	possible.
 *
 *	lmodetype: signifies whether the client can
 *	handle real linemode, or if use of kludgeomatic linemode
 *	is preferred.  It will be set to one of the following:
 *		REAL_LINEMODE : use linemode option
 *		NO_KLUDGE : don't initiate kludge linemode.
 *		KLUDGE_LINEMODE : use kludge linemode
 *		NO_LINEMODE : client is ignorant of linemode
 *
 *	linemode, uselinemode : linemode is true if linemode
 *	is currently on, uselinemode is the state that we wish
 *	to be in.  If another function wishes to turn linemode
 *	on or off, it sets or clears uselinemode.
 *
 *	editmode, useeditmode : like linemode/uselinemode, but
 *	these contain the edit mode states (edit and trapsig).
 *
 * The state variables correspond to some of the state information
 * in the pty.
 *	linemode:
 *		In real linemode, this corresponds to whether the pty
 *		expects external processing of incoming data.
 *		In kludge linemode, this more closely corresponds to the
 *		whether normal processing is on or not.  (ICANON in
 *		system V, or COOKED mode in BSD.)
 *		If the -l option was specified (alwayslinemode), then
 *		an attempt is made to force external processing on at
 *		all times.
 *
 * The following heuristics are applied to determine linemode
 * handling within the server.
 *	1) Early on in starting up the server, an attempt is made
 *	   to negotiate the linemode option.  If this succeeds
 *	   then lmodetype is set to REAL_LINEMODE and all linemode
 *	   processing occurs in the context of the linemode option.
 *	2) If the attempt to negotiate the linemode option failed,
 *	   and the "-k" (don't initiate kludge linemode) isn't set,
 *	   then we try to use kludge linemode.  We test for this
 *	   capability by sending "do Timing Mark".  If a positive
 *	   response comes back, then we assume that the client
 *	   understands kludge linemode (ech!) and the
 *	   lmodetype flag is set to KLUDGE_LINEMODE.
 *	3) Otherwise, linemode is not supported at all and
 *	   lmodetype remains set to NO_LINEMODE (which happens
 *	   to be 0 for convenience).
 *	4) At any time a command arrives that implies a higher
 *	   state of linemode support in the client, we move to that
 *	   linemode support.
 *
 * A short explanation of kludge linemode is in order here.
 *	1) The heuristic to determine support for kludge linemode
 *	   is to send a do timing mark.  We assume that a client
 *	   that supports timing marks also supports kludge linemode.
 *	   A risky proposition at best.
 *	2) Further negotiation of linemode is done by changing the
 *	   the server's state regarding SGA.  If server will SGA,
 *	   then linemode is off, if server won't SGA, then linemode
 *	   is on.
 */
void
localstat (void)
{
  void netflush (void);
  int need_will_echo = 0;

#if defined CRAY2 && defined UNICOS5
  /*
   * Keep track of that ol' CR/NL mapping while we're in the
   * neighborhood.
   */
  newmap = tty_isnewmap ();
#endif /* defined(CRAY2) && defined(UNICOS5) */

  /*
   * Check for state of BINARY options.
   */
  if (tty_isbinaryin ())
    {
      if (his_want_state_is_wont (TELOPT_BINARY))
	send_do (TELOPT_BINARY, 1);
    }
  else
    {
      if (his_want_state_is_will (TELOPT_BINARY))
	send_dont (TELOPT_BINARY, 1);
    }

  if (tty_isbinaryout ())
    {
      if (my_want_state_is_wont (TELOPT_BINARY))
	send_will (TELOPT_BINARY, 1);
    }
  else
    {
      if (my_want_state_is_will (TELOPT_BINARY))
	send_wont (TELOPT_BINARY, 1);
    }

  /*
   * Check for changes to flow control if client supports it.
   */
  flowstat ();

  /*
   * Check linemode on/off state
   */
  uselinemode = tty_linemode ();

  /*
   * If alwayslinemode is on, and pty is changing to turn it off, then
   * force linemode back on.
   */
  if (alwayslinemode && linemode && !uselinemode)
    {
      uselinemode = 1;
      tty_setlinemode (uselinemode);
    }

#ifdef	ENCRYPTION
  /*
   * If the terminal is not echoing, but editing is enabled,
   * something like password input is going to happen, so
   * if we the other side is not currently sending encrypted
   * data, ask the other side to start encrypting.
   */
  if (his_state_is_will (TELOPT_ENCRYPT))
    {
      static int enc_passwd = 0;
      if (uselinemode && !tty_isecho () && tty_isediting ()
	  && (enc_passwd == 0) && !decrypt_input)
	{
	  encrypt_send_request_start ();
	  enc_passwd = 1;
	}
      else if (enc_passwd)
	{
	  encrypt_send_request_end ();
	  enc_passwd = 0;
	}
    }
#endif /* ENCRYPTION */

  /*
   * Do echo mode handling as soon as we know what the
   * linemode is going to be.
   * If the pty has echo turned off, then tell the client that
   * the server will echo.  If echo is on, then the server
   * will echo if in character mode, but in linemode the
   * client should do local echoing.  The state machine will
   * not send anything if it is unnecessary, so don't worry
   * about that here.
   *
   * If we need to send the WILL ECHO (because echo is off),
   * then delay that until after we have changed the MODE.
   * This way, when the user is turning off both editing
   * and echo, the client will get editing turned off first.
   * This keeps the client from going into encryption mode
   * and then right back out if it is doing auto-encryption
   * when passwords are being typed.
   */
  if (uselinemode)
    {
      if (tty_isecho ())
	send_wont (TELOPT_ECHO, 1);
      else
	need_will_echo = 1;
      if (lmodetype == KLUDGE_OK)
	lmodetype = KLUDGE_LINEMODE;
    }

  /*
   * If linemode is being turned off, send appropriate
   * command and then we're all done.
   */
  if (!uselinemode && linemode)
    {
      if (lmodetype == REAL_LINEMODE)
	{
	  send_dont (TELOPT_LINEMODE, 1);
	}
      else if (lmodetype == KLUDGE_LINEMODE)
	send_will (TELOPT_SGA, 1);
      send_will (TELOPT_ECHO, 1);
      linemode = uselinemode;
      goto done;
    }

  /*
   * If using real linemode check edit modes for possible later use.
   * If we are in kludge linemode, do the SGA negotiation.
   */
  if (lmodetype == REAL_LINEMODE)
    {
      useeditmode = 0;
      if (tty_isediting ())
	useeditmode |= MODE_EDIT;
      if (tty_istrapsig ())
	useeditmode |= MODE_TRAPSIG;
      if (tty_issofttab ())
	useeditmode |= MODE_SOFT_TAB;
      if (tty_islitecho ())
	useeditmode |= MODE_LIT_ECHO;
    }
  else if (lmodetype == KLUDGE_LINEMODE)
    {
      if (tty_isediting () && uselinemode)
	send_wont (TELOPT_SGA, 1);
      else
	send_will (TELOPT_SGA, 1);
    }

  /*
   * Negotiate linemode on if pty state has changed to turn it on.
   * Send appropriate command and send along edit mode, then all done.
   */
  if (uselinemode && !linemode)
    {
      if (lmodetype == KLUDGE_LINEMODE)
	{
	  send_wont (TELOPT_SGA, 1);
	}
      else if (lmodetype == REAL_LINEMODE)
	{
	  char data[8];

	  send_do (TELOPT_LINEMODE, 1);
	  /* send along edit modes */
	  sprintf (data, "%c%c%c%c%c%c%c",
		   IAC, SB, TELOPT_LINEMODE, LM_MODE, useeditmode, IAC, SE);
	  net_output_datalen (data, sizeof (data));
	  DEBUG (debug_options, 1,
		 printsub ('>', data + 2, sizeof (data) - 2));

	  editmode = useeditmode;
	}
      linemode = uselinemode;
      goto done;
    }

  /*
   * None of what follows is of any value if not using
   * real linemode.
   */
  if (lmodetype < REAL_LINEMODE)
    goto done;

  if (linemode && his_state_is_will (TELOPT_LINEMODE))
    {
      /*
       * If edit mode changed, send edit mode.
       */
      if (useeditmode != editmode)
	{
	  /*
	   * Send along appropriate edit mode mask.
	   */
	  char data[8];

	  sprintf (data, "%c%c%c%c%c%c%c",
		   IAC, SB, TELOPT_LINEMODE, LM_MODE, useeditmode, IAC, SE);
	  net_output_datalen (data, sizeof (data));
	  DEBUG (debug_options, 1,
		 printsub ('>', data + 2, sizeof (data) - 2));

	  editmode = useeditmode;
	}


      /*
       * Check for changes to special characters in use.
       */
      start_slc (0);
      check_slc ();
      end_slc (0);
    }

done:
  if (need_will_echo)
    send_will (TELOPT_ECHO, 1);
  /*
   * Some things should be deferred until after the pty state has
   * been set by the local process.  Do those things that have been
   * deferred now.  This only happens once.
   */
  if (_terminit == 0)
    {
      _terminit = 1;
      defer_terminit ();
    }

  netflush ();
  set_termbuf ();
  return;

}				/* end of localstat */

/*
 * flowstat
 *
 * Check for changes to flow control
 */
void
flowstat (void)
{
  if (his_state_is_will (TELOPT_LFLOW))
    {
      char data[7];

      if (tty_flowmode () != flowmode)
	{
	  flowmode = tty_flowmode ();
	  sprintf (data, "%c%c%c%c%c%c",
		   IAC, SB, TELOPT_LFLOW,
		   flowmode ? LFLOW_ON : LFLOW_OFF, IAC, SE);
	  net_output_datalen (data, sizeof (data));
	  DEBUG (debug_options, 1,
		 printsub ('>', data + 2, sizeof (data) - 2));
	}
      if (tty_restartany () != restartany)
	{
	  restartany = tty_restartany ();
	  sprintf (data, "%c%c%c%c%c%c",
		   IAC, SB, TELOPT_LFLOW,
		   restartany ? LFLOW_RESTART_ANY : LFLOW_RESTART_XON,
		   IAC, SE);
	  net_output_datalen (data, sizeof (data));
	  DEBUG (debug_options, 1,
		 printsub ('>', data + 2, sizeof (data) - 2));
	}
    }
}

/*
 * clientstat
 *
 * Process linemode related requests from the client.
 * Client can request a change to only one of linemode, editmode or slc's
 * at a time, and if using kludge linemode, then only linemode may be
 * affected.
 */
void
clientstat (int code, int parm1, int parm2)
{
  void netflush (void);

  /*
   * Get a copy of terminal characteristics.
   */
  init_termbuf ();

  /*
   * Process request from client. code tells what it is.
   */
  switch (code)
    {
    case TELOPT_LINEMODE:
      /*
       * Don't do anything unless client is asking us to change
       * modes.
       */
      uselinemode = (parm1 == WILL);
      if (uselinemode != linemode)
	{
	  /*
	   * If using kludge linemode, make sure that
	   * we can do what the client asks.
	   * We cannot turn off linemode if alwayslinemode
	   * and the ICANON bit is set.
	   */
	  if (lmodetype == KLUDGE_LINEMODE)
	    {
	      if (alwayslinemode && tty_isediting ())
		{
		  uselinemode = 1;
		}
	    }

	  /*
	   * Quit now if we can't do it.
	   */
	  if (uselinemode == linemode)
	    return;

	  /*
	   * If using real linemode and linemode is being
	   * turned on, send along the edit mode mask.
	   */
	  if (lmodetype == REAL_LINEMODE && uselinemode)
	    if (uselinemode)
	      {
		char data[8];

		useeditmode = 0;
		if (tty_isediting ())
		  useeditmode |= MODE_EDIT;
		if (tty_istrapsig ())
		  useeditmode |= MODE_TRAPSIG;
		if (tty_issofttab ())
		  useeditmode |= MODE_SOFT_TAB;
		if (tty_islitecho ())
		  useeditmode |= MODE_LIT_ECHO;

		sprintf (data, "%c%c%c%c%c%c%c",
			 IAC, SB, TELOPT_LINEMODE,
			 LM_MODE, useeditmode, IAC, SE);
		net_output_datalen (data, sizeof (data));
		DEBUG (debug_options, 1,
		       printsub ('>', data + 2, sizeof (data) - 2));

		editmode = useeditmode;
	      }


	  tty_setlinemode (uselinemode);

	  linemode = uselinemode;

	  if (!linemode)
	    send_will (TELOPT_ECHO, 1);
	}
      break;

    case LM_MODE:
      {
	int ack, changed;

	/*
	 * Client has sent along a mode mask.  If it agrees with
	 * what we are currently doing, ignore it; if not, it could
	 * be viewed as a request to change.  Note that the server
	 * will change to the modes in an ack if it is different from
	 * what we currently have, but we will not ack the ack.
	 */
	useeditmode &= MODE_MASK;
	ack = (useeditmode & MODE_ACK);
	useeditmode &= ~MODE_ACK;

	if ((changed = (useeditmode ^ editmode)))
	  {
	    /*
	     * This check is for a timing problem.  If the
	     * state of the tty has changed (due to the user
	     * application) we need to process that info
	     * before we write in the state contained in the
	     * ack!!!  This gets out the new MODE request,
	     * and when the ack to that command comes back
	     * we'll set it and be in the right mode.
	     */
	    if (ack)
	      localstat ();
	    if (changed & MODE_EDIT)
	      tty_setedit (useeditmode & MODE_EDIT);

	    if (changed & MODE_TRAPSIG)
	      tty_setsig (useeditmode & MODE_TRAPSIG);

	    if (changed & MODE_SOFT_TAB)
	      tty_setsofttab (useeditmode & MODE_SOFT_TAB);

	    if (changed & MODE_LIT_ECHO)
	      tty_setlitecho (useeditmode & MODE_LIT_ECHO);

	    set_termbuf ();

	    if (!ack)
	      {
		char data[8];

		sprintf (data, "%c%c%c%c%c%c%c",
			 IAC, SB, TELOPT_LINEMODE,
			 LM_MODE, useeditmode | MODE_ACK, IAC, SE);
		net_output_datalen (data, sizeof (data));
		DEBUG (debug_options, 1,
		       printsub ('>', data + 2, sizeof (data) - 2));
	      }

	    editmode = useeditmode;
	  }

	break;

      }				/* end of case LM_MODE */

    case TELOPT_NAWS:
#ifdef	TIOCSWINSZ
      {
	struct winsize ws;

	def_col = parm1;
	def_row = parm2;
	/*
	 * Defer changing window size until after terminal is
	 * initialized.
	 */
	if (terminit () == 0)
	  return;

	/*
	 * Change window size as requested by client.
	 */

	ws.ws_col = parm1;
	ws.ws_row = parm2;

# if !defined SOLARIS && !defined SOLARIS10
	ioctl (pty, TIOCSWINSZ, (char *) &ws);
# else/* SOLARIS || SOLARIS10 */
	{
	  int tty = pty;
	  char *name = ptsname (pty);

	  if (name)
	    tty = open (name, O_RDWR | O_NONBLOCK | O_NOCTTY);

	  ioctl (tty, TIOCSWINSZ, (char *) &ws);

	  if (name)
	    close (tty);
	}
# endif/* SOLARIS || SOLARIS10 */
      }
#endif /* TIOCSWINSZ */

      break;

    case TELOPT_TSPEED:
      {
	def_tspeed = parm1;
	def_rspeed = parm2;
	/*
	 * Defer changing the terminal speed.
	 */
	if (terminit () == 0)
	  return;
	/*
	 * Change terminal speed as requested by client.
	 * We set the receive speed first, so that if we can't
	 * store separate receive and transmit speeds, the transmit
	 * speed will take precedence.
	 */
	tty_rspeed (parm2);
	tty_tspeed (parm1);
	set_termbuf ();

	break;

      }				/* end of case TELOPT_TSPEED */

    default:
      /* What? */
      break;
    }				/* end of switch */

#if defined CRAY2 && defined UNICOS5
  /*
   * Just in case of the likely event that we changed the pty state.
   */
  rcv_ioctl ();
#endif /* defined(CRAY2) && defined(UNICOS5) */

  netflush ();

}				/* end of clientstat */

#if defined CRAY2 && defined UNICOS5
void
termstat (void)
{
  needtermstat = 1;
}

void
_termstat (void)
{
  needtermstat = 0;
  init_termbuf ();
  localstat ();
  rcv_ioctl ();
}
#endif /* defined(CRAY2) && defined(UNICOS5) */

/*
 * defer_terminit
 *
 * Some things should not be done until after the login process has started
 * and all the pty modes are set to what they are supposed to be.  This
 * function is called when the pty state has been processed for the first time.
 * It calls other functions that do things that were deferred in each module.
 */
void
defer_terminit (void)
{

  /*
   * local stuff that got deferred.
   */
  if (def_tspeed != -1)
    {
      clientstat (TELOPT_TSPEED, def_tspeed, def_rspeed);
      def_tspeed = def_rspeed = 0;
    }

#ifdef	TIOCSWINSZ
  if (def_col || def_row)
    {
      struct winsize ws;

      memset ((char *) &ws, 0, sizeof (ws));
      ws.ws_col = def_col;
      ws.ws_row = def_row;

# if !defined SOLARIS && !defined SOLARIS10
      ioctl (pty, TIOCSWINSZ, (char *) &ws);
# else/* SOLARIS || SOLARIS10 */
      {
	int tty = pty;
	char *name = ptsname (pty);

	if (name)
	  tty = open (name, O_RDWR | O_NONBLOCK | O_NOCTTY);

	ioctl (tty, TIOCSWINSZ, (char *) &ws);

	if (name)
	  close (tty);
      }
# endif/* SOLARIS || SOLARIS10 */
    }
#endif /* TIOCSWINSZ */

  /*
   * The only other module that currently defers anything.
   */
  deferslc ();

}				/* end of defer_terminit */

/*
 * terminit
 *
 * Returns true if the pty state has been processed yet.
 */
int
terminit (void)
{
  return (_terminit);

}				/* end of terminit */
