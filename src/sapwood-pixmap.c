/* GTK+ Sapwood Engine
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
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com>
 */
#include <config.h>

#include "sapwood-pixmap.h"
#include "sapwood-proto.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

struct _SapwoodPixmap {
  guint32    id;
  gint       width;
  gint       height;
  GdkPixmap *pixmap[3][3];
  GdkBitmap *pixmask[3][3];
};

static GQuark
sapwood_pixmap_error_get_quark ()
{
  static GQuark q = 0;
  if (!q)
    q = g_quark_from_static_string ("sapwood-pixmap-error-quark");
  return q;
}

static int
pixbuf_proto_get_socket (GError **err)
{
  struct sockaddr_un  sun;
  const char         *sock_path;
  int                 fd;

  fd = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "socket: %s", strerror (errno));
      return -1;
    }

  sock_path = sapwood_socket_path_get_default ();

  memset(&sun, '\0', sizeof(sun));
  sun.sun_family = AF_LOCAL;
#ifdef HAVE_ABSTRACT_SOCKETS
  strcpy (&sun.sun_path[1], sock_path);
#else
  strcpy (&sun.sun_path[0], sock_path);
#endif
  if (connect (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "Failed to connect to sapwood server using `%s': %s\n\n"
		   "\t`%s' MUST be started before applications",
		   sock_path, strerror (errno),
		   SAPWOOD_SERVER);
      close (fd);
      return -1;
    }

  return fd;
}

static gboolean
pixbuf_proto_request (const char *req, ssize_t reqlen,
		      char       *rep, ssize_t replen,
		      GError    **err)
{
  static int fd = -1;
  ssize_t    n;

  if (fd == -1)
    {
      fd = pixbuf_proto_get_socket (err);
      if (fd == -1)
	return FALSE;
    }

  n = write (fd, req, reqlen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "write: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != reqlen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "wrote %d of %d bytes", n, reqlen);
      return FALSE;
    }

  if (!rep)
    return TRUE;

  n = read (fd, rep, replen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "read: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != replen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "read %d, expected %d bytes", n, replen);
      return FALSE;
    }

  return TRUE;
}

SapwoodPixmap *
sapwood_pixmap_get_for_file (const char *filename,
			    int border_left,
			    int border_right,
			    int border_top,
			    int border_bottom,
			    GError **err)
{
  SapwoodPixmap     *self;
  char               buf[ sizeof(PixbufOpenRequest) + PATH_MAX + 1 ] = {0};
  PixbufOpenRequest *req = (PixbufOpenRequest *) buf;
  PixbufOpenResponse rep;
  int                flen;
  int                i, j;

  /* marshal request */
  flen = g_strlcpy (req->filename, filename, PATH_MAX);
  if (flen > PATH_MAX)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "%s: filename too long", filename);
      return NULL;
    }

  req->base.op       = PIXBUF_OP_OPEN;
  req->base.length   = sizeof(*req) + flen + 1;
  req->border_left   = border_left;
  req->border_right  = border_right;
  req->border_top    = border_top;
  req->border_bottom = border_bottom;

  if (!pixbuf_proto_request ((char*)req,  req->base.length,
			     (char*)&rep, sizeof(rep), err))
    return NULL;

  /* unmarshal response */
  self = g_new0 (SapwoodPixmap, 1);
  self->id     = rep.id;
  self->width  = rep.width;
  self->height = rep.height;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	GdkPixmap *pixmap  = NULL;
	GdkBitmap *pixmask = NULL;
	int xerror;

	if (rep.pixmap[i][j])
	  {
	    gdk_error_trap_push ();
	    pixmap = gdk_pixmap_foreign_new (rep.pixmap[i][j]);
	    gdk_flush ();
	    if ((xerror = gdk_error_trap_pop ()) || !pixmap)
	      {
		g_warning ("%s: pixmap[%d][%d]: gdk_pixmap_foreign_new(%x) failed, X error = %d",
			   g_basename (filename), i, j, rep.pixmap[i][j], xerror);
		if (pixmap)
		  g_object_unref (pixmap);
		pixmap = NULL;
	      }
	  }

	if (rep.pixmask[i][j])
	  {
	    gdk_error_trap_push ();
	    pixmask = gdk_pixmap_foreign_new (rep.pixmask[i][j]);
	    gdk_flush ();
	    if ((xerror = gdk_error_trap_pop ()) || !pixmask)
	      {
		g_warning ("%s: pixmask[%d][%d]: gdk_pixmap_foreign_new(%x) failed, X error = %d", 
			   g_basename (filename), i, j, rep.pixmask[i][j], xerror);
		if (pixmask)
		  g_object_unref (pixmask);
		pixmask = NULL;
	      }
	  }

	if (pixmask && !pixmap)
	  {
	    g_warning ("%s: pixmask[%d][%d]: no pixmap", g_basename (filename), i, j);
	  }

	self->pixmap[i][j]  = pixmap;
	self->pixmask[i][j] = pixmask;
      }

  return self;
}

static void
pixbuf_proto_unref_pixmap (guint32 id)
{
  PixbufCloseRequest  req;
  GError             *err = NULL;

  req.base.op     = PIXBUF_OP_CLOSE;
  req.base.length = sizeof(PixbufCloseRequest);
  req.id          = id;
  if (!pixbuf_proto_request ((char*)&req, req.base.length, NULL, 0, &err))
    {
      g_warning ("close(0x%x): %s", id, err->message);
      g_error_free (err);
      return;
    }
}

void
sapwood_pixmap_free (SapwoodPixmap *self)
{
  if (self)
    {
      GdkDisplay *display = NULL;
      int         i, j;

      for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	  if (self->pixmap[i][j])
	    {
	      if (!display)
		display = gdk_drawable_get_display (self->pixmap[i][j]);

	      g_object_unref (self->pixmap[i][j]);
	      if (self->pixmask[i][j])
		g_object_unref (self->pixmask[i][j]);

	      self->pixmap[i][j] = NULL;
	      self->pixmask[i][j] = NULL;
	    }

      /* need to make sure all our operations are processed before the pixmaps
       * are free'd by the server or we risk causing BadPixmap error */
      if (display)
	gdk_display_sync (display);

      pixbuf_proto_unref_pixmap (self->id);
      g_free (self);
    }
}

gboolean
sapwood_pixmap_get_geometry (SapwoodPixmap *self,
			    gint         *width,
			    gint         *height)
{
  if (!self)
    return FALSE;

  if (width) 
    *width = self->width;
  if (height)
    *height = self->height;

  return TRUE;
}

void
sapwood_pixmap_get_pixmap (SapwoodPixmap *self, gint x, gint y,
			   GdkPixmap **pixmap, GdkBitmap **pixmask)
{
  *pixmap  = self->pixmap[y][x];
  *pixmask = self->pixmask[y][x];
}

static void
sapwood_pixmap_render_rects_internal (SapwoodPixmap *self,
                                      GdkDrawable   *draw,
                                      gint           draw_x,
                                      gint           draw_y,
                                      GdkBitmap     *mask,
                                      gint           mask_x,
                                      gint           mask_y,
                                      gboolean       mask_required,
                                      GdkRectangle  *clip_rect,
                                      gint           n_rect,
                                      SapwoodRect   *rect)
{
  static GdkGC *mask_gc = NULL;
  static GdkGC *draw_gc = NULL;
  GdkGCValues   values;
  gint          xofs;
  gint          yofs;
  gint          n;
  gboolean      have_mask = FALSE;

  xofs = draw_x - mask_x;
  yofs = draw_y - mask_y;

  if (mask)
    {
      if (!mask_gc)
	{
	  values.fill = GDK_TILED;
	  mask_gc = gdk_gc_new_with_values (mask, &values, GDK_GC_FILL);
	}

      for (n = 0; n < n_rect; n++)
	{
	  /* const */ GdkRectangle *dest = &rect[n].dest;
	  GdkRectangle              area;

	  if (!mask_required && clip_rect)
	    {
	      if (!gdk_rectangle_intersect (dest, clip_rect, &area))
		continue;
	    }	  
	  else
	    area = *dest;

	  if (rect[n].pixmap && rect[n].pixmask)
	    {
	      values.tile = rect[n].pixmask;
	      values.ts_x_origin = dest->x - xofs;
	      values.ts_y_origin = dest->y - yofs;
	      gdk_gc_set_values (mask_gc, &values, GDK_GC_TILE|GDK_GC_TS_X_ORIGIN|GDK_GC_TS_Y_ORIGIN);

	      gdk_draw_rectangle (mask, mask_gc, TRUE, area.x - xofs, area.y - yofs, area.width, area.height);

	      have_mask = TRUE;
	    }
	}
    }

  if (!draw_gc)
    {
      values.fill = GDK_TILED;
      draw_gc = gdk_gc_new_with_values (draw, &values, GDK_GC_FILL);
    }

  values.clip_mask = have_mask ? mask : NULL;
  values.clip_x_origin = xofs;
  values.clip_y_origin = yofs;
  gdk_gc_set_values (draw_gc, &values, GDK_GC_CLIP_MASK|GDK_GC_CLIP_X_ORIGIN|GDK_GC_CLIP_Y_ORIGIN);

  for (n = 0; n < n_rect; n++)
    {
      /* const */ GdkRectangle *dest = &rect[n].dest;
      GdkRectangle              area;

      if (clip_rect)
	{
	  if (!gdk_rectangle_intersect (dest, clip_rect, &area))
	    continue;
	}	  
      else
	area = *dest;

      if (rect[n].pixmap)
	{
	  values.tile = rect[n].pixmap;
	  values.ts_x_origin = dest->x;
	  values.ts_y_origin = dest->y;
	  gdk_gc_set_values (draw_gc, &values, GDK_GC_TILE|GDK_GC_TS_X_ORIGIN|GDK_GC_TS_Y_ORIGIN);

	  gdk_draw_rectangle (draw, draw_gc, TRUE, area.x, area.y, area.width, area.height);
	}
    }
}

void
sapwood_pixmap_render_rects (SapwoodPixmap *self,
                             GdkDrawable   *draw,
                             gint           draw_x,
                             gint           draw_y,
                             gint           width,
                             gint           height,
                             GdkBitmap     *mask,
                             gint           mask_x,
                             gint           mask_y,
                             gboolean       mask_required,
                             GdkRectangle  *clip_rect,
                             gint           n_rect,
                             SapwoodRect   *rect)
{
  gint       tmp_width;
  gint       tmp_height;
  GdkPixmap *tmp;
  gboolean   need_tmp_mask = FALSE;
  GdkPixmap *tmp_mask = NULL;
  cairo_t   *mask_cr = NULL;
  cairo_t   *tmp_cr;
  cairo_t   *cr;
  gint       n;
  
  /* Don't even try to scale down shape masks (should never be useful, and
   * implementing would add some complexity.) Areas larger than the pixmap
   * can be tiled fine.
   */
  if (mask_required || (width >= self->width && height >= self->height))
    {
      sapwood_pixmap_render_rects_internal (self, draw, draw_x, draw_y, mask, mask_x, mask_y, mask_required, clip_rect, n_rect, rect);
      return;
    }

  /* offset the drawing on temporary pixmap */
  for (n = 0; n < n_rect; n++)
    {
      SapwoodRect *r = &rect[n];
      r->dest.x -= draw_x;
      r->dest.y -= draw_y;
      if (r->pixmap && r->pixmask)
	need_tmp_mask = TRUE;
    }

  /* prepare temporary pixmap and mask to draw in full size */
  tmp_width = MAX(width, self->width);
  tmp_height = MAX(height, self->height);

  tmp = gdk_pixmap_new (draw, tmp_width, tmp_height, -1);

  if (need_tmp_mask)
    {
      tmp_mask = gdk_pixmap_new (draw, tmp_width, tmp_height, 1);

      mask_cr = gdk_cairo_create (tmp_mask);
      cairo_set_source_rgb (mask_cr, 1., 1., 1.);
      cairo_paint (mask_cr);
    }

  sapwood_pixmap_render_rects_internal (self, tmp, 0, 0, tmp_mask, 0, 0, mask_required, NULL, n_rect, rect);

  tmp_cr = gdk_cairo_create (tmp);

  /* finally, render downscaled at draw_x,draw_y */
  cr = gdk_cairo_create (draw);
  if (clip_rect)
    {
      gdk_cairo_rectangle (cr, clip_rect);
      cairo_clip (cr);
    }
  cairo_rectangle (cr, draw_x, draw_y, width, height);
  cairo_clip (cr);

  cairo_translate (cr, draw_x, draw_y);
  cairo_scale (cr, (double)width / (double)tmp_width,
		   (double)height / (double)tmp_height);

  cairo_set_source_surface (cr, cairo_get_target (tmp_cr), 0, 0);
  if (mask_cr)
    cairo_mask_surface (cr, cairo_get_target (mask_cr), 0, 0);
  else
    cairo_paint (cr);

  /* clean up */
  cairo_destroy (cr);
  if (mask_cr)
    cairo_destroy (mask_cr);
  if (tmp_mask)
    g_object_unref (tmp_mask);
  cairo_destroy (tmp_cr);
  g_object_unref (tmp);
}
