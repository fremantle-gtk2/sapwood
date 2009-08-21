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

#include "sapwood-pixmap-priv.h"
#include "sapwood-proto.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static gboolean
pixbuf_proto_request (const char *req,
                      ssize_t     reqlen,
                      char       *rep,
                      ssize_t     replen,
                      GError    **err)
{
  static int fd = -1;
  ssize_t    n;

  if (fd == -1)
    {
      fd = sapwood_client_get_socket (err);
      if (fd == -1)
	return FALSE;
    }

  n = write (fd, req, reqlen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "write: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != reqlen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "wrote %d of %d bytes", n, reqlen);
      return FALSE;
    }

  if (!rep)
    return TRUE;

  n = read (fd, rep, replen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "read: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != replen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "read %d, expected %d bytes", n, replen);
      return FALSE;
    }

  return TRUE;
}

SapwoodPixmap *
sapwood_pixmap_get_for_file (const char *filename,
                             int         border_left,
                             int         border_right,
                             int         border_top,
                             int         border_bottom,
                             int         depth,
                             GError    **err)
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
      g_set_error (err, SAPWOOD_CLIENT_ERROR, SAPWOOD_CLIENT_ERROR_UNKNOWN,
		   "%s: filename too long", filename);
      return NULL;
    }

  req->base.op       = PIXBUF_OP_OPEN;
  req->base.length   = sizeof(*req) + flen + 1;
  req->depth         = depth;
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
            if (gdk_colormap_get_visual (gdk_screen_get_rgba_colormap (gdk_screen_get_default ()))->depth == depth)
              {
                gdk_drawable_set_colormap (pixmap,
                                           gdk_screen_get_rgba_colormap (gdk_screen_get_default ()));
              }
            else if (gdk_colormap_get_visual (gdk_screen_get_rgb_colormap (gdk_screen_get_default ()))->depth == depth)
              {
                gdk_drawable_set_colormap (pixmap,
                                           gdk_screen_get_rgb_colormap (gdk_screen_get_default ()));
              }
            else if (gdk_colormap_get_visual (gdk_screen_get_system_colormap (gdk_screen_get_default ()))->depth == depth)
              {
                gdk_drawable_set_colormap (pixmap,
                                           gdk_screen_get_system_colormap (gdk_screen_get_default ()));
              }
            else
              {
                g_assert_not_reached ();
              }

            if (sapwood_debug_xtraps)
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

            if (sapwood_debug_xtraps)
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
                             gint          *width,
                             gint          *height)
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
sapwood_pixmap_get_pixmap (SapwoodPixmap *self,
                           gint           x,
                           gint           y,
                           GdkPixmap    **pixmap,
                           GdkBitmap    **pixmask)
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
  gint          xofs;
  gint          yofs;
  gint          n;

  xofs = draw_x - mask_x;
  yofs = draw_y - mask_y;

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
          cairo_t* cr = gdk_cairo_create (draw);

          gdk_cairo_set_source_pixmap (cr, rect[n].pixmap, dest->x, dest->y);
          cairo_pattern_set_extend (cairo_get_source (cr),
                                    CAIRO_EXTEND_REPEAT);
          cairo_rectangle (cr, area.x, area.y, area.width, area.height);
          cairo_clip (cr);

          if (!mask || !rect[n].pixmask)
            {
              cairo_paint (cr);
            }
          else
            {
              cairo_matrix_t  matrix;

              cairo_t* tmp_cr = gdk_cairo_create (rect[n].pixmask);
              cairo_pattern_t* pattern = cairo_pattern_create_for_surface (cairo_get_target (tmp_cr));

              cairo_matrix_init_translate (&matrix, - dest->x, - dest->y);
              cairo_pattern_set_extend (pattern,
                                        CAIRO_EXTEND_REPEAT);
              cairo_pattern_set_matrix (pattern,
                                        &matrix);

              cairo_mask (cr, pattern);

              cairo_pattern_destroy (pattern);
              cairo_destroy (tmp_cr);
            }
          cairo_destroy (cr);
        }
    }
}

static void
sapwood_crop_pixmap (GdkPixmap *pixmap,
                     int        x,
                     int        y,
                     int        requested_width,
                     int        requested_height,
                     int        original_width,
                     int        original_height)
{
  cairo_t *cr;
  cairo_surface_t *surface;
  int w1, h1, w2, h2;

  cr = gdk_cairo_create (pixmap);
  surface = cairo_get_target (cr);

  w1 = requested_width / 2;
  h1 = requested_height / 2;
  w2 = (requested_width + 1) / 2;
  h2 = (requested_height + 1) / 2;

  /* top-left */
  cairo_save (cr);
  cairo_set_source_surface (cr, surface, x, y);
  cairo_rectangle (cr, x, y, w2, h2);
  cairo_clip (cr);
  cairo_paint (cr);
  cairo_restore (cr);

  /* top-right */
  cairo_save (cr);
  cairo_set_source_surface (cr, surface, x + requested_width - original_width, y);
  cairo_rectangle (cr, x + w1, y, w2, h2);
  cairo_clip (cr);
  cairo_paint (cr);
  cairo_restore (cr);

  /* bottom-left */
  cairo_save (cr);
  cairo_set_source_surface (cr, surface, x,  y + requested_height - original_height);
  cairo_rectangle (cr, x, y + h1, w2, h2);
  cairo_clip (cr);
  cairo_paint (cr);
  cairo_restore (cr);

  /* bottom-right */
  cairo_save (cr);
  cairo_set_source_surface (cr, surface, x + requested_width - original_width,
					 y + requested_height - original_height);
  cairo_rectangle (cr, x + w1, y + h1, w2, h2);
  cairo_clip (cr);
  cairo_paint (cr);
  cairo_restore (cr);

  cairo_destroy (cr);
}

void
sapwood_pixmap_render_rects (SapwoodPixmap *self,
                             GtkWidget     *widget,
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

  if (gdk_drawable_get_depth (tmp) == 32)
    {
      tmp_cr = gdk_cairo_create (tmp);
      cairo_set_operator (tmp_cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (tmp_cr, 0.0, 0.0, 0.0, 0.0);
      cairo_paint (tmp_cr);
      cairo_destroy (tmp_cr);
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

  if (width > 0 && width < tmp_width || height > 0 && height < tmp_height)
      sapwood_crop_pixmap (tmp, 0, 0, width, height,
			   self->width, self->height);
  else if (width != tmp_width || height != tmp_height)
    {
      cairo_scale (cr, (double)width / (double)tmp_width,
		       (double)height / (double)tmp_height);

      if (sapwood_debug_scaling)
        {
	  g_warning ("scaling pixmap for %s: requested %dx%d; real %dx%d",
		     G_OBJECT_TYPE_NAME (widget),
		     width, height,
		     self->width, self->height);
	  cairo_save (tmp_cr);
	  cairo_set_source_rgba (tmp_cr, 1.0, 1.0, 0.0, 0.25);
	  cairo_paint (tmp_cr);
	  cairo_restore (tmp_cr);
        }
    }

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
