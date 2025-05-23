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

/* Written by Marcus Brinkmann.  */

#ifndef IFCONFIG_IFCONFIG_H
# define IFCONFIG_IFCONFIG_H

# include "flags.h"
# include "options.h"
# include "printif.h"
# include "system.h"
# include <progname.h>
# include <error.h>
# include <argp.h>
# define obstack_chunk_alloc malloc
# define obstack_chunk_free free
# include <obstack.h>
# include <libinetutils.h>

/* XXX */
extern int configure_if (int sfd, struct ifconfig *ifp);

#endif
