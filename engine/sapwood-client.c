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

#include "sapwood-client.h"

GQuark
sapwood_client_get_error_quark (void)
{
  static GQuark q = 0;
  if (!q)
    q = g_quark_from_static_string ("sapwood-client-error-quark");
  return q;
}

