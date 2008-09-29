/* This file is part of GTK+ Sapwood Engine
 *
 * AUTHORS
 *     Sven Herzberg
 *
 * Copyright (C) 2008  Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sapwood-client.h"

GQuark
sapwood_client_get_error_quark (void)
{
  static GQuark q = 0;
  if (!q)
    q = g_quark_from_static_string ("sapwood-client-error-quark");
  return q;
}

int
pixbuf_proto_get_socket (GError **err)
{
  struct sockaddr_un  sun;
  const char         *sock_path;
  int                 fd;

  fd = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0)
    {
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "socket: %s", strerror (errno));
      return -1;
    }

  sock_path = sapwood_socket_path_get_default ();

  memset (&sun, '\0', sizeof(sun));
  sun.sun_family = AF_LOCAL;
#ifdef HAVE_ABSTRACT_SOCKETS
  strcpy (&sun.sun_path[1], sock_path);
#else
  strcpy (&sun.sun_path[0], sock_path);
#endif
  if (connect (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
    {
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "Failed to connect to sapwood server using `%s': %s\n\n"
		   "\t`%s' MUST be started before applications",
		   sock_path, strerror (errno),
		   SAPWOOD_SERVER);
      close (fd);
      return -1;
    }

  return fd;
}

