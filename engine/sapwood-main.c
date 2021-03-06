/* GTK+ Sapwood Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
 * Copyright (C) 2005 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com> based on 
 * code by Owen Taylor <otaylor@redhat.com> and 
 * Carsten Haitzler <raster@rasterman.com>
 */
#include <config.h>

#include "theme-pixbuf.h"
#include "sapwood-style.h"
#include "sapwood-rc-style.h"
#include <gmodule.h>

G_GNUC_INTERNAL guint sapwood_debug_flags = 0;
gboolean sapwood_debug_scaling = FALSE;
gboolean sapwood_debug_xtraps = FALSE;

typedef enum {
  SAPWOOD_DEBUG_SCALING   = 1 << 0,
  SAPWOOD_DEBUG_XTRAPS    = 1 << 1
} SapwoodDebugFlag;

G_MODULE_EXPORT void
theme_init (GTypeModule *module)
{
  GDebugKey keys[] = {
    {"scaling", SAPWOOD_DEBUG_SCALING},
    {"xtraps", SAPWOOD_DEBUG_XTRAPS}
  };
  const gchar* debug;

  sapwood_rc_style_register_types (module);
  sapwood_style_register_types (module);

  debug = g_getenv ("SAPWOOD_DEBUG");
  if (debug)
    {
      sapwood_debug_flags = g_parse_debug_string (debug, keys, G_N_ELEMENTS (keys));
      sapwood_debug_scaling = sapwood_debug_flags & SAPWOOD_DEBUG_SCALING;
      sapwood_debug_xtraps = sapwood_debug_flags & SAPWOOD_DEBUG_XTRAPS;
    }
}

G_MODULE_EXPORT void
theme_exit (void)
{
}

G_MODULE_EXPORT GtkRcStyle *
theme_create_rc_style (void)
{
  return GTK_RC_STYLE (g_object_new (SAPWOOD_TYPE_RC_STYLE, NULL));  
}

/* The following function will be called by GTK+ when the module
 * is loaded and checks to see if we are compatible with the
 * version of GTK+ that loads us.
 */
G_MODULE_EXPORT const gchar* g_module_check_init (GModule *module);
const gchar*
g_module_check_init (GModule *module)
{
  return gtk_check_version (GTK_MAJOR_VERSION,
			    GTK_MINOR_VERSION,
			    GTK_MICRO_VERSION - GTK_INTERFACE_AGE);
}
