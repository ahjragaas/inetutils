/* hurd.c -- Code for ifconfig specific to GNU/Hurd.
  Copyright (C) 2015-2022 Free Software Foundation, Inc.

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

/* Mostly written by Marcus Brinkmann.
   Adaptions to GNU/Hurd by Mats Erik Andersson.  */

#include <config.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <hurd.h>
#include <hurd/paths.h>
#include <hurd/fsys.h>
#include <argz.h>
#include "../ifconfig.h"

#include <attribute.h>


/* Output format stuff.  */

const char *system_default_format = "gnu";


/* Check that pfinet is driving the given interface name.  */
static int
check_driving (const char *name)
{
  file_t node;
  fsys_t fsys;
  error_t err;

  char *argz = 0, *new_argz = 0;
  size_t argz_len = 0;
  char *entry = 0;
  const char *socket = _SERVERS_SOCKET "/2";

  int ret = 0;

  if (strcmp (name, "lo") == 0)
    /* Always configured.  */
    return 1;

  node = file_name_lookup (socket, 0, 0666);
  if (node == MACH_PORT_NULL)
    {
      error (0, 0, "Interface name %s does not exist", name);
      return 0;
    }

  file_get_fs_options (node, &argz, &argz_len);

  for (entry = argz; entry; entry = argz_next (argz, argz_len, entry))
    {
      if (strcmp (entry, "-i") == 0)
	{
	  char *ifname = argz_next (argz, argz_len, entry);

	  if (strcmp (ifname, name) == 0)
	    {
	      /* Already there.  */
	      ret = 1;
	      goto out;
	    }
	}

      else if (strncmp (entry, "--interface=", 12) == 0)
	{
	  if (strcmp (entry + 12, name) == 0)
	    {
	      /* Already there.  */
	      ret = 1;
	      goto out;
	    }
	}
    }

  /* Not already there.  */

  err = file_getcontrol (node, &fsys);
  if (err)
    {
      if (err == EPERM)
	error (0, err, "Could not make pfinet %s drive %s", socket, name);
      else
	error (0, err, "Could not get control of %s", socket);
      goto out;
    }

  new_argz = malloc (argz_len);
  memcpy (new_argz, argz, argz_len);

  err = argz_insert (&new_argz, &argz_len, new_argz, name);
  if (err)
    {
      error (0, err, "Could not prepend name %s to '%s' for %s", name, new_argz, socket);
      goto out;
    }

  err = argz_insert (&new_argz, &argz_len, new_argz, "-i");
  if (err)
    {
      argz_stringify (new_argz, argz_len, ' ');
      error (0, err, "Could not prepend -i to '%s' for %s", new_argz, socket);
      goto out;
    }

  err = fsys_set_options (fsys, new_argz, argz_len, 1);
  if (err)
    {
      argz_stringify (new_argz, argz_len, ' ');
      error (0, err, "Could not make pfinet %s drive %s with '%s'", socket, name, new_argz);
      goto out;
    }

  ret = 1;

out:
  free (new_argz);
  vm_deallocate (mach_task_self (), (vm_offset_t) argz, argz_len);
  mach_port_deallocate (mach_task_self (), node);

  return ret;
}

/* Argument parsing stuff.  */

const char *system_help = "NAME [ADDR]\
 [broadcast BRDADDR] [netmask MASK]\
 [mtu N] [up|down] [FLAGS]";

struct argp_child system_argp_child;

int
system_parse_opt (struct ifconfig **ifp MAYBE_UNUSED,
		  char option MAYBE_UNUSED,
		  char *optarg MAYBE_UNUSED)
{
  return 0;
}

int
system_parse_opt_rest (struct ifconfig **ifp, int argc, char *argv[])
{
  int i = 0, mask, rev;
  enum {
    EXPECT_NOTHING,
    EXPECT_AF,
    EXPECT_BROADCAST,
    EXPECT_NETMASK,
    EXPECT_METRIC,
    EXPECT_MTU
  } expect = EXPECT_AF;

  *ifp = parse_opt_new_ifs (argv[0]);

  while (++i < argc)
    {
      switch (expect)
	{
	case EXPECT_BROADCAST:
	  parse_opt_set_brdaddr (*ifp, argv[i]);
	  break;

	case EXPECT_NETMASK:
	  parse_opt_set_netmask (*ifp, argv[i]);
	  break;

	case EXPECT_MTU:
	  parse_opt_set_mtu (*ifp, argv[i]);
	  break;

	/* XXX: 2015-07-18, GNU/Hurd does not yet support
		ioctl(SIOCSIFMETRIC), but we let the code
		handle this standard ability anyway!
	 */
	case EXPECT_METRIC:
	  parse_opt_set_metric (*ifp, argv[i]);
	  break;

	case EXPECT_AF:
	  expect = EXPECT_NOTHING;
	  if (!strcmp (argv[i], "inet"))
	    continue;
	  else if (!strcmp (argv[i], "inet6"))
	    {
	      error (0, 0, "%s is not a supported address family", argv[i]);
	      return 0;
	    }
	  break;

	case EXPECT_NOTHING:
	  break;
	}

      if (expect != EXPECT_NOTHING)
	expect = EXPECT_NOTHING;
      else if (!strcmp (argv[i], "broadcast"))
	expect = EXPECT_BROADCAST;
      else if (!strcmp (argv[i], "netmask"))
	expect = EXPECT_NETMASK;
      else if (!strcmp (argv[i], "metric"))
	expect = EXPECT_METRIC;
      else if (!strcmp (argv[i], "mtu"))
	expect = EXPECT_MTU;
      else if (!strcmp (argv[i], "up"))
	parse_opt_set_flag (*ifp, IFF_UP | IFF_RUNNING, 0);
      else if (!strcmp (argv[i], "down"))
	parse_opt_set_flag (*ifp, IFF_UP, 1);
      else if (((mask = if_nameztoflag (argv[i], &rev))
		& ~IU_IFF_CANTCHANGE) != 0)
	parse_opt_set_flag (*ifp, mask, rev);
      else
	{
	  if (!((*ifp)->valid & IF_VALID_ADDR))
	    parse_opt_set_address (*ifp, argv[i]);
	  else if (!((*ifp)->valid & IF_VALID_DSTADDR))
	    parse_opt_set_dstaddr (*ifp, argv[i]);
	}
    }

  switch (expect)
    {
    case EXPECT_BROADCAST:
      error (0, 0, "option `broadcast' requires an argument");
      break;

    case EXPECT_NETMASK:
      error (0, 0, "option `netmask' requires an argument");
      break;

    case EXPECT_METRIC:
      error (0, 0, "option `metric' requires an argument");
      break;

    case EXPECT_MTU:
      error (0, 0, "option `mtu' requires an argument");
      break;

    case EXPECT_AF:
    case EXPECT_NOTHING:
      return 1;
    }

  return 0;
}

int
system_preconfigure (int sfd MAYBE_UNUSED,
		     struct ifreq *ifr MAYBE_UNUSED)
{
  if (!check_driving (ifr->ifr_name))
    return -1;
  return 0;
}

int
system_configure (int sfd MAYBE_UNUSED,
		  struct ifreq *ifr MAYBE_UNUSED,
		  struct system_ifconfig *ifs MAYBE_UNUSED)
{
  return 0;
}

struct if_nameindex* (*system_if_nameindex) (void) = if_nameindex;

static void
print_hwaddr_ether (format_data_t form MAYBE_UNUSED,
		    unsigned char *data)
{
  *column += printf ("%02X:%02X:%02X:%02X:%02X:%02X",
		     data[0], data[1], data[2], data[3], data[4], data[5]);
  had_output = 1;
}

/* GNU/Hurd and Mach are using a mixture of BSD definitions
 * and GNU/Linux interface headers, which in this situation
 * means that sa_family_t is an unsigned char, from BSD, while
 * all ARPHRD_* come from GNU/Linux and are thus 16 bits wide.
 * We must account for this.  The following bitmask will
 * adapt to any future change!
 */

#define _ARP_MASK ((sizeof (sa_family_t) == 1) ? 0xff : 0xffff)

struct arphrd_symbol
{
  const char *name;
  const char *title;
  int value;
  void (*print_hwaddr) (format_data_t form, unsigned char *data);
} arphrd_symbols[] =
  {
#ifdef ARPHRD_ETHER		/* Ethernet 10/100Mbps.  */
    { "ETHER", "Ethernet", ARPHRD_ETHER & _ARP_MASK, print_hwaddr_ether},
#endif
#ifdef ARPHRD_LOOPBACK		/* Loopback device.  */
    { "LOOPBACK", "Local Loopback", ARPHRD_LOOPBACK & _ARP_MASK, NULL},
#endif
    { NULL, NULL, 0, NULL}
  };

struct arphrd_symbol *
arphrd_findvalue (int value)
{
  struct arphrd_symbol *arp = arphrd_symbols;
  while (arp->name != NULL)
    {
      if (arp->value == value)
	break;
      arp++;
    }
  if (arp->name)
    return arp;
  else
    return NULL;
}

void
system_fh_hwaddr_query (format_data_t form, int argc, char *argv[])
{
#ifdef SIOCGIFHWADDR
  struct arphrd_symbol *arp;

  if (ioctl (form->sfd, SIOCGIFHWADDR, form->ifr) < 0)
    select_arg (form, argc, argv, 1);

  arp = arphrd_findvalue (form->ifr->ifr_hwaddr.sa_family);
  select_arg (form, argc, argv, (arp && arp->print_hwaddr) ? 0 : 1);
#else
  select_arg (form, argc, argv, 1);
#endif
}

void
system_fh_hwaddr (format_data_t form, int argc MAYBE_UNUSED,
		  MAYBE_UNUSED char *argv[])
{
#ifdef SIOCGIFHWADDR
  if (ioctl (form->sfd, SIOCGIFHWADDR, form->ifr) < 0)
    error (EXIT_FAILURE, errno,
	   "SIOCGIFHWADDR failed for interface `%s'",
	   form->ifr->ifr_name);
  else
    {
      struct arphrd_symbol *arp;

      arp = arphrd_findvalue (form->ifr->ifr_hwaddr.sa_family);
      if (arp && arp->print_hwaddr)
	arp->print_hwaddr (form,
			   (unsigned char *) form->ifr->ifr_hwaddr.sa_data);
      else
	put_string (form, "(hwaddr unknown)");
    }
#else
  *column += printf ("(not available)");
  had_output = 1;
#endif
}

void
system_fh_hwtype_query (format_data_t form, int argc, char *argv[])
{
#ifdef SIOCGIFHWADDR
  if (ioctl (form->sfd, SIOCGIFHWADDR, form->ifr) >= 0)
    select_arg (form, argc, argv, 0);
  else
#endif
    select_arg (form, argc, argv, 1);
}

void
system_fh_hwtype (format_data_t form, int argc MAYBE_UNUSED,
		  MAYBE_UNUSED char *argv[])
{
#ifdef SIOCGIFHWADDR
  if (ioctl (form->sfd, SIOCGIFHWADDR, form->ifr) < 0)
    error (EXIT_FAILURE, errno,
	   "SIOCGIFHWADDR failed for interface `%s'",
	   form->ifr->ifr_name);
  else
    {
      struct arphrd_symbol *arp;

      arp = arphrd_findvalue (form->ifr->ifr_hwaddr.sa_family);
      if (arp)
	put_string (form, arp->title);
      else
	put_string (form, "(hwtype unknown)");
    }
#else
  *column += printf ("(not available)");
  had_output = 1;
#endif
}
