/*
  Copyright (C) 1996-2025 Free Software Foundation, Inc.

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
 * Copyright (c) 1991, 1993
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

/*
 * Copyright (C) 1990 by the Massachusetts Institute of Technology
 *
 * Export of this software from the United States of America is assumed
 * to require a specific license from the United States Government.
 * It is the responsibility of any person or organization contemplating
 * export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#ifdef	ENCRYPTION
Encryptions *findencryption (int);
Encryptions *finddecryption (int);
int EncryptStart (char *);
int EncryptStartInput (void);
int EncryptStartOutput (void);
int EncryptStatus (void);
int EncryptStop (char *);
int EncryptStopInput (void);
int EncryptStopOutput (void);
int EncryptType (char *, char *);

void encrypt_init (char *, int);
Encryptions *findencryption (int);
void encrypt_send_supprt (void);
void encrypt_auto (int);
void decrypt_auto (int);
void encrypt_is (unsigned char *, int);
void encrypt_reply (unsigned char *, int);
void encrypt_start (unsigned char *, int);
void encrypt_start_input (int);
void encrypt_session_key (Session_Key *, int);
void encrypt_end (void);
void encrypt_end_input (void);
void encrypt_start_output (int);
void encrypt_end_output (void);
void encrypt_request_start (unsigned char *, int);
void encrypt_send_request_start (void);
void encrypt_request_end (void);
void encrypt_send_request_end (void);
void encrypt_send_end (void);
void encrypt_wait (void);
void encrypt_support (unsigned char *, int);
void encrypt_send_support (void);
void encrypt_dec_keyid (unsigned char *, int);
void encrypt_enc_keyid (unsigned char *, int);
void encrypt_send_keyid (int, unsigned char *, int, int);
void encrypt_printsub (unsigned char *, int, char *, int);
int net_write (unsigned char *, int);

# ifdef	TELNETD
void encrypt_wait (void);
# else
int encrypt_cmd (int, char **);
void encrypt_display (void);
# endif

void krbdes_encrypt (unsigned char *, int);
int krbdes_decrypt (int);
int krbdes_is (unsigned char *, int);
int krbdes_reply (unsigned char *, int);
void krbdes_init (int);
int krbdes_start (int, int);
void krbdes_session (Session_Key *, int);
void krbdes_printsub (unsigned char *, int, char *, int);

void cfb64_encrypt (unsigned char *, int);
int cfb64_decrypt (int);
void cfb64_init (int);
int cfb64_start (int, int);
int cfb64_is (unsigned char *, int);
int cfb64_reply (unsigned char *, int);
void cfb64_session (Session_Key *, int);
int cfb64_keyid (int, unsigned char *, int *);
void cfb64_printsub (unsigned char *, int, char *, int);

void ofb64_encrypt (unsigned char *, int);
int ofb64_decrypt (int);
void ofb64_init (int);
int ofb64_start (int, int);
int ofb64_is (unsigned char *, int);
int ofb64_reply (unsigned char *, int);
void ofb64_session (Session_Key *, int);
int ofb64_keyid (int, unsigned char *, int *);
void ofb64_printsub (unsigned char *, int, char *, int);

int des_new_random_key (Block);
void des_set_random_generator_seed (Block);
void des_key_sched (Block, Schedule);
void des_ecb_encrypt (Block, Block, Schedule, int);
int des_string_to_key (char *, Block);
#endif /* ENCRYPTION */
