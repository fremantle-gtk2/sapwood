/* This file is part of the GTK+ Sapwood Engine
 *
 * AUTHORS
 *     Tommi Komulainen
 *     Sven Herzberg
 *
 * Copyright (C) 2005  Nokia Corporation
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

#include "config.h"

#include "cache-node.h"

void
cache_node_free (CacheNode *self)
{
  g_free (self);
}

CacheNode*
cache_node_new (PixbufOpenResponse *rep)
{
  CacheNode* self = g_new0 (CacheNode, 1);
  self->rep = rep;
  self->refcnt = 1;
  return self;
}

void
cache_node_ref (CacheNode *self)
{
  self->refcnt++;
}

