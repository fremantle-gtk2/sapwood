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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include  <stdint.h>

#include "theme-pixbuf.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef ENABLE_DEBUG
#define LOG(...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG(...)
#endif

static GHashTable *pixbuf_hash = NULL;

ThemePixbuf *
theme_pixbuf_new (void)
{
  ThemePixbuf *result = g_new0 (ThemePixbuf, 1);
  result->refcnt = 1;
  result->stretch = TRUE;

  return result;
}

static void
theme_pixbuf_destroy (ThemePixbuf *theme_pb)
{
  if (theme_pb->refcnt < 0)
      g_warning ("[%p] destroy: refcnt < 0", theme_pb);
  else if (theme_pb->refcnt > 1)
      g_warning ("[%p] destroy: refcnt > 1", theme_pb);

  if (theme_pb->shared)
    {
      g_hash_table_remove (pixbuf_hash, theme_pb);
      if (theme_pb->pixmap)
	sapwood_pixmap_free (theme_pb->pixmap);
    }
  if (theme_pb->basename)
    g_free (theme_pb->basename);
  g_free (theme_pb);
}

static ThemePixbuf *
theme_pixbuf_ref (ThemePixbuf *theme_pb)
{
  theme_pb->refcnt++;
  LOG ("[%p] ref: refcnt = %d", theme_pb, theme_pb->refcnt);
  g_assert (theme_pb->refcnt != 0);
  return theme_pb;
}

void
theme_pixbuf_unref (ThemePixbuf *theme_pb)
{
  theme_pb->refcnt--;
  LOG ("[%p] unref: refcnt = %d", theme_pb, theme_pb->refcnt);
  if (theme_pb->refcnt == 0)
    theme_pixbuf_destroy (theme_pb);
}

static guint
theme_pixbuf_hash (gconstpointer v)
{
  const ThemePixbuf *theme_pb = v;
  return (uintptr_t)theme_pb->dirname ^ g_str_hash (theme_pb->basename);
}

static gboolean
theme_pixbuf_equal (gconstpointer v1,
                    gconstpointer v2)
{
  const ThemePixbuf *a = v1;
  const ThemePixbuf *b = v2;

  if (a->dirname != b->dirname ||
      !g_str_equal (a->basename, b->basename))
    return FALSE;

  return TRUE;
}

ThemePixbuf *
theme_pixbuf_canonicalize (ThemePixbuf *theme_pb,
                           gboolean    *warn)
{
  ThemePixbuf *canon;

  g_assert (theme_pb->pixmap == NULL);

  *warn = FALSE;

  if (!pixbuf_hash)
    pixbuf_hash = g_hash_table_new (theme_pixbuf_hash, theme_pixbuf_equal);

  canon = g_hash_table_lookup (pixbuf_hash, theme_pb);
  if (!canon)
    {
      theme_pb->shared = 1;
      g_hash_table_insert (pixbuf_hash, theme_pb, theme_pb);
      canon = theme_pb;
    }
  else
    {
      if (theme_pb->border_bottom != canon->border_bottom ||
          theme_pb->border_top    != canon->border_top ||
          theme_pb->border_left   != canon->border_left ||
          theme_pb->border_right  != canon->border_right ||
          theme_pb->stretch       != canon->stretch)
        *warn = TRUE;

      theme_pixbuf_ref (canon);
      theme_pixbuf_unref (theme_pb);
    }

  return canon;
}

#ifdef ENABLE_DEBUG
static void
theme_pixbuf_check_borders (ThemePixbuf *theme_pb)
{
  gchar *filename;
  gint   width, height;

  if (!theme_pb->basename)
    return;

  filename = g_build_filename (theme_pb->dirname, theme_pb->basename, NULL);

  if (gdk_pixbuf_get_file_info (filename, &width, &height) != NULL)
    {
      if (theme_pb->border_left + theme_pb->border_right >= width ||
	  theme_pb->border_top + theme_pb->border_bottom >= height)
	g_warning ("%s: border values too big for the image", theme_pb->basename);
    }

  g_free (filename);
}
#else
#define theme_pixbuf_check_borders(x)	(void)0
#endif

void
theme_pixbuf_set_filename (ThemePixbuf *theme_pb,
			   const char  *filename)
{
  g_assert (theme_pb->pixmap == NULL);

  if (theme_pb->basename)
    g_free (theme_pb->basename);

  theme_pb->dirname  = NULL;
  theme_pb->basename = NULL;

  if (filename)
    {
      char abspath[PATH_MAX + 1];

      if (realpath (filename, abspath))
	{
	  char *dirname;
	  char *basename;

	  dirname  = g_path_get_dirname (abspath);
	  basename = g_path_get_basename (abspath);

	  theme_pb->dirname  = g_quark_to_string (g_quark_from_string (dirname));
	  theme_pb->basename = basename;

	  g_free (dirname);

	  theme_pixbuf_check_borders (theme_pb);
	}
      else
	g_warning ("%s: %s", filename, g_strerror (errno));
    }
}

void
theme_pixbuf_set_border (ThemePixbuf *theme_pb,
			 gint         left,
			 gint         right,
			 gint         top,
			 gint         bottom)
{
  g_return_if_fail (theme_pb->pixmap == NULL);

  theme_pb->border_left = left;
  theme_pb->border_right = right;
  theme_pb->border_top = top;
  theme_pb->border_bottom = bottom;

  theme_pixbuf_check_borders (theme_pb);
}

void
theme_pixbuf_set_stretch (ThemePixbuf *theme_pb,
			  gboolean     stretch)
{
  g_return_if_fail (theme_pb->pixmap == NULL);

  theme_pb->stretch = stretch;
}

static SapwoodPixmap *
theme_pixbuf_get_pixmap (ThemePixbuf *theme_pb)
{
  if (!theme_pb->pixmap)
    {
      char   *filename;
      GError *err = NULL;

      filename = g_build_filename (theme_pb->dirname, theme_pb->basename, NULL);
      theme_pb->pixmap = sapwood_pixmap_get_for_file (filename,
						      theme_pb->border_left,
						      theme_pb->border_right,
						      theme_pb->border_top,
						      theme_pb->border_bottom,
						      &err);
      if (!theme_pb->pixmap)
	{
	  g_warning ("sapwood-theme: Failed to load pixmap file %s: %s\n",
		     filename, err->message);
	  g_error_free (err);
	}

      g_free (filename);
    }
  return theme_pb->pixmap;
}

gboolean
theme_pixbuf_get_geometry (ThemePixbuf *theme_pb,
			   gint        *width,
			   gint        *height)
{
  if (!theme_pb)
    return FALSE;

  return sapwood_pixmap_get_geometry (theme_pixbuf_get_pixmap (theme_pb), 
				     width, height);
}

/* Scale the rectangle (src_x, src_y, src_width, src_height)
 * onto the rectangle (dest_x, dest_y, dest_width, dest_height)
 * of the destination, clip by clip_rect and render
 */
gboolean
theme_pixbuf_render (ThemePixbuf  *theme_pb,
		     GtkWidget    *widget,
		     GdkWindow    *window,
		     GdkBitmap    *mask,
		     GdkRectangle *clip_rect,
		     guint         component_mask,
		     gboolean      center,
		     gint          x,
		     gint          y,
		     gint          width,
		     gint          height)
{
  gint dest_x[4], dest_y[4];
  SapwoodPixmap *pixmap;
  gint pixbuf_width;
  gint pixbuf_height;
  gint draw_width;
  gint draw_height;
  SapwoodRect rect[9];
  gint       n_rect;
  gint       mask_x;
  gint       mask_y;
  gboolean   mask_required;

  if (width <= 0 || height <= 0)
    return FALSE;

  if (!theme_pixbuf_get_geometry (theme_pb, &pixbuf_width, &pixbuf_height))
    return FALSE;

  pixmap = theme_pixbuf_get_pixmap (theme_pb);

  if (theme_pb->stretch)
    {
      /* if we do scaling we want to draw at least the whole pixmap */
      draw_width  = MAX(width, pixbuf_width);
      draw_height = MAX(height, pixbuf_height);

      dest_x[0] = x;
      dest_x[1] = x + theme_pb->border_left;
      dest_x[2] = x + draw_width - theme_pb->border_right;
      dest_x[3] = x + draw_width;

      dest_y[0] = y;
      dest_y[1] = y + theme_pb->border_top;
      dest_y[2] = y + draw_height - theme_pb->border_bottom;
      dest_y[3] = y + draw_height;

      if (component_mask & COMPONENT_ALL)
	component_mask = (COMPONENT_ALL - 1) & ~component_mask;

#define RENDER_COMPONENT(X,Y) do {			           \
    sapwood_pixmap_get_pixmap (pixmap, X, Y, &rect[n_rect].pixmap, \
			       &rect[n_rect].pixmask);	           \
							           \
    rect[n_rect].dest.x = dest_x[X];			           \
    rect[n_rect].dest.y = dest_y[Y];			           \
    rect[n_rect].dest.width = dest_x[X+1] - dest_x[X];	           \
    rect[n_rect].dest.height = dest_y[Y+1] - dest_y[Y];	           \
							           \
    n_rect++;						           \
} while(0)

      n_rect = 0;
      if (component_mask & COMPONENT_NORTH_WEST) RENDER_COMPONENT (0, 0);
      if (component_mask & COMPONENT_NORTH)	 RENDER_COMPONENT (1, 0);
      if (component_mask & COMPONENT_NORTH_EAST) RENDER_COMPONENT (2, 0);
      if (component_mask & COMPONENT_WEST)	 RENDER_COMPONENT (0, 1);
      if (component_mask & COMPONENT_CENTER)	 RENDER_COMPONENT (1, 1);
      if (component_mask & COMPONENT_EAST)	 RENDER_COMPONENT (2, 1);
      if (component_mask & COMPONENT_SOUTH_WEST) RENDER_COMPONENT (0, 2);
      if (component_mask & COMPONENT_SOUTH)	 RENDER_COMPONENT (1, 2);
      if (component_mask & COMPONENT_SOUTH_EAST) RENDER_COMPONENT (2, 2);

#undef RENDER_COMPONENT

      if (!mask)
        {
	  gint mask_width;
	  gint mask_height;

	  mask_x = 0;
	  mask_y = 0;
	  mask_width = width;
	  mask_height = height;
	  mask_required = FALSE;

	  if (clip_rect)
	    {
	      /* limit the mask to clip_rect size to avoid allocating huge
	       * pixmaps and getting BadAlloc from X
	       */
	      if (clip_rect->width < mask_width)
		{
		  LOG ("width: %d -> %d, x: %d - %d", mask_width, clip_rect->width, x, clip_rect->x);
		  mask_x = x - clip_rect->x;
		  mask_width = clip_rect->width;
		}
	      if (clip_rect->height < height)
		{
		  LOG ("height: %d -> %d, y: %d - %d", mask_height, clip_rect->height, y, clip_rect->y);
		  mask_y = y - clip_rect->y;
		  mask_height = clip_rect->height;
		}
	    }

	  gdk_error_trap_push ();

	  mask = gdk_pixmap_new (NULL, mask_width, mask_height, 1);

          if (sapwood_debug_xtraps)
            gdk_flush ();

	  if (gdk_error_trap_pop ())
	    {
	      if (clip_rect)
		g_warning ("theme_pixbuf_render(clip_rect={x: %d,y: %d, width: %d, height: %d}: gdk_pixmap_new(width: %d, height: %d) failed",
			   clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height, mask_width, mask_height);
	      else
		g_warning ("theme_pixbuf_render(clip_rect=(null)}: gdk_pixmap_new(width: %d, height: %d) failed", mask_width, mask_height);

	      /* pretend that we drew things successfully, there should be a
	       * new paint call coming to allow us to paint the thing properly
	       */
	      return TRUE;
	    }
	}
      else
	{
	  g_object_ref (mask);
	  mask_x = x;
	  mask_y = y;
	  mask_required = TRUE;
	}

      sapwood_pixmap_render_rects (pixmap, widget,
                                   window, x, y, width, height,
                                   mask, mask_x, mask_y, mask_required,
                                   clip_rect, n_rect, rect);

      g_object_unref (mask);
    }
  else if (center)
    {
      /* when centering don't expand beyond pixbuf size */
      draw_width = MIN(width, pixbuf_width);
      draw_height = MIN(height, pixbuf_height);

      x += (width - draw_width) / 2;
      y += (height - draw_height) / 2;

      sapwood_pixmap_get_pixmap (pixmap, 1, 1,
                                 &rect[0].pixmap, &rect[0].pixmask);
      rect[0].dest.x = x;
      rect[0].dest.y = y;
      rect[0].dest.width = pixbuf_width;
      rect[0].dest.height = pixbuf_height;

      /* need to ensure mask is available if the pixmap has one */
      mask_x = x;
      mask_y = y;
      if (rect[0].pixmask && !mask)
	{
	  mask = gdk_pixmap_new (NULL, pixbuf_width, pixbuf_height, 1);
	  mask_x = 0;
	  mask_y = 0;
	}
      else if (mask)
	g_object_ref (mask);

      sapwood_pixmap_render_rects (pixmap, widget,
                                   window, x, y, draw_width, draw_height,
                                   mask, mask_x, mask_y, FALSE,
                                   clip_rect, 1, rect);

      if (mask)
	g_object_unref (mask);
    }
  else /* tile? */
    {
      return FALSE;
    }

  return TRUE;
}
