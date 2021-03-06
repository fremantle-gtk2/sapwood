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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "sapwood-pixmap.h"

/* internals */

typedef struct _ThemeData ThemeData;
typedef struct _ThemeImage ThemeImage;
typedef struct _ThemeMatchData ThemeMatchData;
typedef struct _ThemePixbuf ThemePixbuf;

enum
{
  TOKEN_SHADOWCOLOR = G_TOKEN_LAST + 1,
  TOKEN_IMAGE,
  TOKEN_FUNCTION,
  TOKEN_FILE,
  TOKEN_STRETCH,
  TOKEN_SHAPED,
  TOKEN_BORDER,
  TOKEN_DETAIL,
  TOKEN_STATE,
  TOKEN_SHADOW,
  TOKEN_GAP_SIDE,
  TOKEN_GAP_FILE,
  TOKEN_GAP_BORDER,
  TOKEN_GAP_START_FILE,
  TOKEN_GAP_START_BORDER,
  TOKEN_GAP_END_FILE,
  TOKEN_GAP_END_BORDER,
  TOKEN_OVERLAY_FILE,
  TOKEN_OVERLAY_BORDER,
  TOKEN_OVERLAY_STRETCH,
  TOKEN_ARROW_DIRECTION,
  TOKEN_D_HLINE,
  TOKEN_D_VLINE,
  TOKEN_D_SHADOW,
  TOKEN_UNUSED_1,
  TOKEN_D_ARROW,
  TOKEN_D_DIAMOND,
  TOKEN_UNUSED_2,
  TOKEN_UNUSED_3,
  TOKEN_D_BOX,
  TOKEN_D_FLAT_BOX,
  TOKEN_D_CHECK,
  TOKEN_D_OPTION,
  TOKEN_UNUSED_4,
  TOKEN_UNUSED_5,
  TOKEN_D_TAB,
  TOKEN_D_SHADOW_GAP,
  TOKEN_D_BOX_GAP,
  TOKEN_D_EXTENSION,
  TOKEN_D_FOCUS,
  TOKEN_D_SLIDER,
  TOKEN_UNUSED_6,
  TOKEN_D_HANDLE,
  TOKEN_D_STEPPER,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_TOP,
  TOKEN_UP,
  TOKEN_BOTTOM,
  TOKEN_DOWN,
  TOKEN_LEFT,
  TOKEN_RIGHT,
  TOKEN_NORMAL,
  TOKEN_ACTIVE,
  TOKEN_PRELIGHT,
  TOKEN_SELECTED,
  TOKEN_INSENSITIVE,
  TOKEN_NONE,
  TOKEN_IN,
  TOKEN_OUT,
  TOKEN_ETCHED_IN,
  TOKEN_ETCHED_OUT,
  TOKEN_ORIENTATION,
  TOKEN_HORIZONTAL,
  TOKEN_VERTICAL,
  TOKEN_POSITION
};

typedef enum
{
  COMPONENT_NORTH_WEST = 1 << 0,
  COMPONENT_NORTH      = 1 << 1,
  COMPONENT_NORTH_EAST = 1 << 2, 
  COMPONENT_WEST       = 1 << 3,
  COMPONENT_CENTER     = 1 << 4,
  COMPONENT_EAST       = 1 << 5, 
  COMPONENT_SOUTH_EAST = 1 << 6,
  COMPONENT_SOUTH      = 1 << 7,
  COMPONENT_SOUTH_WEST = 1 << 8,
  COMPONENT_ALL        = 1 << 9
} ThemePixbufComponent;

typedef enum {
  THEME_MATCH_GAP_SIDE        = 1 << 0,
  THEME_MATCH_ORIENTATION     = 1 << 1,
  THEME_MATCH_STATE           = 1 << 2,
  THEME_MATCH_SHADOW          = 1 << 3,
  THEME_MATCH_ARROW_DIRECTION = 1 << 4,
  THEME_MATCH_POSITION        = 1 << 5
} ThemeMatchFlags;

typedef enum {
  THEME_POS_LEFT   = 1 << 0, /* GTK_POS_LEFT   */
  THEME_POS_RIGHT  = 1 << 1, /* GTK_POS_RIGHT  */
  THEME_POS_TOP    = 1 << 2, /* GTK_POS_TOP    */
  THEME_POS_BOTTOM = 1 << 3  /* GTK_POS_BOTTOM */
} ThemePositionFlags;

struct _ThemePixbuf
{
  const char *dirname;
  gchar      *basename;

  SapwoodPixmap *pixmap;

  guint16     border_left;
  guint16     border_right;
  guint16     border_bottom;
  guint16     border_top;
  guint       refcnt : 14;
  guint       shared : 1;
  guint       stretch : 1;
};

struct _ThemeMatchData
{
  gchar          *detail;
  guint16         function;	/* Mandatory */

  ThemeMatchFlags flags           : 6;
  ThemePositionFlags position     : 4;
  GtkStateType    state           : 3;
  GtkShadowType   shadow          : 3;
  GtkPositionType gap_side        : 2;
  guint           arrow_direction : 2;	/* GtkArrowType, but without NONE */
  GtkOrientation  orientation     : 1;
};

struct _ThemeImage
{
  ThemePixbuf    *background;
  ThemePixbuf    *overlay;
  ThemePixbuf    *gap_start;
  ThemePixbuf    *gap;
  ThemePixbuf    *gap_end;

  ThemeMatchData  match_data;

  guint           refcount : 31;
  guint           background_shaped : 1;
};


ThemePixbuf *theme_pixbuf_new          (void) G_GNUC_INTERNAL;
void         theme_pixbuf_unref        (ThemePixbuf  *theme_pb) G_GNUC_INTERNAL;
ThemePixbuf *theme_pixbuf_canonicalize (ThemePixbuf  *theme_pb,
                                        gboolean     *warn) G_GNUC_INTERNAL;
void         theme_pixbuf_set_filename (ThemePixbuf  *theme_pb,
					const char   *filename) G_GNUC_INTERNAL;
gboolean     theme_pixbuf_get_geometry (ThemePixbuf  *theme_pb,
					gint         *width,
					gint         *height) G_GNUC_INTERNAL;
void         theme_pixbuf_set_border   (ThemePixbuf  *theme_pb,
					gint          left,
					gint          right,
					gint          top,
					gint          bottom) G_GNUC_INTERNAL;
void         theme_pixbuf_set_stretch  (ThemePixbuf  *theme_pb,
					gboolean      stretch) G_GNUC_INTERNAL;
gboolean     theme_pixbuf_render       (ThemePixbuf  *theme_pb,
					GtkWidget    *widget,
					GdkWindow    *window,
					GdkBitmap    *mask,
					GdkRectangle *clip_rect,
					guint         component_mask,
					gboolean      center,
					gint          dest_x,
					gint          dest_y,
					gint          dest_width,
					gint          dest_height) G_GNUC_INTERNAL;


extern GtkStyleClass pixmap_default_class G_GNUC_INTERNAL;
