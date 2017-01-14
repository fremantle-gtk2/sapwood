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

#include <math.h>
#include <string.h>

#include "theme-pixbuf.h"
#include "sapwood-rc-style.h"
#include "sapwood-style.h"

#ifdef ENABLE_DEBUG
#define LOG(...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG(...)
#endif

G_DEFINE_DYNAMIC_TYPE (SapwoodStyle, sapwood_style, GTK_TYPE_STYLE);

#ifdef ENABLE_DEBUG
static gchar *
enum_value_to_string (GType enum_type,
                      gint  enum_value)
{
  gpointer enum_class;
  GEnumValue *val;
  gchar* result;

  g_assert (G_TYPE_IS_ENUM (enum_type));

  enum_class = g_type_class_ref (enum_type);
  val = g_enum_get_value (G_ENUM_CLASS (enum_class), enum_value);
  result = g_strdup (val ? val->value_nick : "undefined");
  g_type_class_unref (enum_class);

  return result;
}
#endif /* ENABLE_DEBUG */

static ThemeImage *
match_theme_image (GtkStyle       *style,
		   ThemeMatchData *match_data)
{
  GList *tmp_list;

  tmp_list = SAPWOOD_RC_STYLE (style->rc_style)->img_list;

  while (tmp_list)
    {
      guint flags;
      ThemeImage *image = tmp_list->data;
      tmp_list = tmp_list->next;

      if (match_data->function != image->match_data.function)
	continue;

      flags = match_data->flags & image->match_data.flags;

      if (flags != image->match_data.flags) /* Required components not present */
	continue;

      if ((flags & THEME_MATCH_STATE) &&
	  match_data->state != image->match_data.state)
	continue;

      if ((flags & THEME_MATCH_POSITION) &&
	  match_data->position != image->match_data.position)
	continue;

      if ((flags & THEME_MATCH_SHADOW) &&
	  match_data->shadow != image->match_data.shadow)
	continue;

      if ((flags & THEME_MATCH_ARROW_DIRECTION) &&
	  match_data->arrow_direction != image->match_data.arrow_direction)
	continue;

      if ((flags & THEME_MATCH_ORIENTATION) &&
	  match_data->orientation != image->match_data.orientation)
	continue;

      if ((flags & THEME_MATCH_GAP_SIDE) &&
	  match_data->gap_side != image->match_data.gap_side)
	continue;

      /* simple pattern matching for (treeview) details
       * in gtkrc 'detail = "*_start"' will match all calls with detail ending
       * with '_start' such as 'cell_even_start', 'cell_odd_start', etc.
       */
      if (image->match_data.detail)
        {
          if (!match_data->detail)
            continue;
          else if (image->match_data.detail[0] == '*')
            {
              if (!g_str_has_suffix (match_data->detail, image->match_data.detail + 1))
                continue;
            }
          else if (strcmp (match_data->detail, image->match_data.detail) != 0)
            continue;
        }

      return image;
    }

  return NULL;
}

static GdkBitmap *
get_window_for_shape (ThemeImage *image,
                      GdkWindow  *window,
                      GtkWidget  *widget,
                      gint        x,
                      gint        y,
                      gint        width,
                      gint        height)
{
  /* It's not a good idea to set a shape mask when painting on anything but
   * widget->window, not only does the shape mask get wrongly offset but also
   * causes re-exposing the widget, and we'll re-apply the shape mask, ad
   * infinitum.
   *
   * Noticed when GtkMenu was changed to do two paints, the other one being on
   * ->bin_window (http://bugzilla.gnome.org/show_bug.cgi?id=169532)
   */
  if (image->background_shaped &&
      widget && window == widget->window)
    {
      gint window_width;
      gint window_height;
      /* It also is not a good idea to apply shape mask when filling a window
       * partially.
       *
       * For example GtkMenu paints/painted the arrow backgrounds with exactly
       * the same parameters as full window background - except for the
       * geometry. (http://bugzilla.gnome.org/show_bug.cgi?id=404571)
       */
      if (x != 0 || y != 0)
        return NULL;

      gdk_drawable_get_size (window, &window_width, &window_height);
      if (width != window_width || height != window_height)
        return NULL;

      if (GTK_IS_MENU (widget))
	return gtk_widget_get_parent_window (widget);
      else if (GTK_IS_WINDOW (widget))
	return window;
    }

  return NULL;
}

static void
check_child_position (GtkWidget      *child,
		      GList          *children,
		      ThemeMatchData *match_data)
{
  GtkWidget *left;
  GtkWidget *right;
  GtkWidget *top;
  GtkWidget *bottom;
  GList *l;

  left = right = top = bottom = child;

  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *widget = l->data;

      if (!GTK_WIDGET_DRAWABLE (widget))
	continue;

      /* XXX Should we consider the lower right corner instead, for
       * right/bottom? */

      if (left->allocation.x > widget->allocation.x)
	left = widget;
      if (right->allocation.x < widget->allocation.x)
	right = widget;
      if (top->allocation.y > widget->allocation.y)
	top = widget;
      if (bottom->allocation.y < widget->allocation.y)
	bottom = widget;
    }

  match_data->flags |= THEME_MATCH_POSITION;
  match_data->position = 0;
  if (left == child)
    match_data->position |= THEME_POS_LEFT;
  if (right == child)
    match_data->position |= THEME_POS_RIGHT;
  if (top == child)
    match_data->position |= THEME_POS_TOP;
  if (bottom == child)
    match_data->position |= THEME_POS_BOTTOM;
}

static void
check_buttonbox_child_position (GtkWidget      *child,
                                ThemeMatchData *match_data)
{
  GList *children = NULL;
  GList *l;
  GtkWidget *bbox;
  gboolean secondary;

  g_assert (GTK_IS_BUTTON_BOX (child->parent));
  bbox = child->parent;

  secondary = gtk_button_box_get_child_secondary (GTK_BUTTON_BOX (bbox), child);

  for (l = GTK_BOX (bbox)->children; l != NULL; l = l->next)
    {
      GtkBoxChild *child_info = l->data;
      GtkWidget *widget = child_info->widget;

      if (child_info->is_secondary == secondary && GTK_WIDGET_DRAWABLE (widget))
	children = g_list_prepend (children, widget);
    }

  check_child_position (child, children, match_data);
  g_list_free (children);
}

static gboolean
draw_simple_image (GtkStyle       *style,
                   GdkWindow      *window,
                   GdkRectangle   *area,
                   GtkWidget      *widget,
                   ThemeMatchData *match_data,
                   gboolean        draw_center,
                   gint            x,
                   gint            y,
                   gint            width,
                   gint            height)
{
  gboolean maemo_position_theming;
  ThemeImage *image;

  if ((width == -1) && (height == -1))
    gdk_drawable_get_size (window, &width, &height);
  else if (width == -1)
    gdk_drawable_get_size (window, &width, NULL);
  else if (height == -1)
    gdk_drawable_get_size (window, NULL, &height);

  if (!(match_data->flags & THEME_MATCH_ORIENTATION))
    {
      match_data->flags |= THEME_MATCH_ORIENTATION;

      if (height > width)
	match_data->orientation = GTK_ORIENTATION_VERTICAL;
      else
	match_data->orientation = GTK_ORIENTATION_HORIZONTAL;
    }

  /* Check for maemo-position-theming to update the position data */
  if (widget && widget->parent &&
      gtk_widget_class_find_style_property (GTK_WIDGET_GET_CLASS (widget->parent),
                                            "maemo-position-theming"))
    {
      gtk_widget_style_get (widget->parent,
			    "maemo-position-theming", &maemo_position_theming,
			    NULL);
      if (maemo_position_theming)
	{
	  if (GTK_IS_BUTTON_BOX (widget->parent))
	    check_buttonbox_child_position (widget, match_data);
	  else
	    {
              /* Generic code for other kinds of containers */
	      GList *children;

	      children = gtk_container_get_children (GTK_CONTAINER (widget->parent));
	      check_child_position (widget, children, match_data);
	      g_list_free (children);
	    }
	}
    }

  image = match_theme_image (style, match_data);
  if (image)
    {
      if (image->background)
	{
	  GdkWindow *maskwin;
	  GdkBitmap *mask = NULL;
	  gboolean valid;

	  maskwin = get_window_for_shape (image, window, widget, x, y, width, height);
	  if (maskwin)
	    mask = gdk_pixmap_new (maskwin, width, height, 1);

	  valid = theme_pixbuf_render (image->background, widget,
				       window, mask, area,
				       draw_center ? COMPONENT_ALL : COMPONENT_ALL | COMPONENT_CENTER,
				       FALSE,
				       x, y, width, height);

	  if (mask)
	    {
	      if (valid)
		gdk_window_shape_combine_mask (maskwin, mask, 0, 0);
	      g_object_unref (mask);
	    }
	}

      if (image->overlay && draw_center)
	theme_pixbuf_render (image->overlay, widget,
			     window, NULL, area, COMPONENT_ALL,
			     TRUE, 
			     x, y, width, height);

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
draw_gap_image (GtkStyle       *style,
                GdkWindow      *window,
                GdkRectangle   *area,
                GtkWidget      *widget,
                ThemeMatchData *match_data,
                gboolean        draw_center,
                gint            x,
                gint            y,
                gint            width,
                gint            height,
                GtkPositionType gap_side,
                gint            gap_x,
                gint            gap_width)
{
  ThemeImage *image;

  if ((width == -1) && (height == -1))
    gdk_drawable_get_size (window, &width, &height);
  else if (width == -1)
    gdk_drawable_get_size (window, &width, NULL);
  else if (height == -1)
    gdk_drawable_get_size (window, NULL, &height);

  if (!(match_data->flags & THEME_MATCH_ORIENTATION))
    {
      match_data->flags |= THEME_MATCH_ORIENTATION;

      if (height > width)
	match_data->orientation = GTK_ORIENTATION_VERTICAL;
      else
	match_data->orientation = GTK_ORIENTATION_HORIZONTAL;
    }

  match_data->flags |= THEME_MATCH_GAP_SIDE;
  match_data->gap_side = gap_side;

  image = match_theme_image (style, match_data);
  if (image)
    {
      gint xthickness, ythickness;
      GdkRectangle r1 = {0, }, r2 = {0, }, r3 = {0, };
      guint components = COMPONENT_ALL;

      if (!draw_center)
	components |= COMPONENT_CENTER;

      if (!theme_pixbuf_get_geometry (image->gap_start, &xthickness, &ythickness))
	{
	  xthickness = style->xthickness;
	  ythickness = style->ythickness;
	}

      switch (gap_side)
	{
	case GTK_POS_TOP:
	  if (!draw_center)
	    components |= COMPONENT_NORTH_WEST | COMPONENT_NORTH | COMPONENT_NORTH_EAST;

	  r1.x      = x;
	  r1.y      = y;
	  r1.width  = gap_x;
	  r1.height = ythickness;
	  r2.x      = x + gap_x;
	  r2.y      = y;
	  r2.width  = gap_width;
	  r2.height = ythickness;
	  r3.x      = x + gap_x + gap_width;
	  r3.y      = y;
	  r3.width  = width - (gap_x + gap_width);
	  r3.height = ythickness;
	  break;

	case GTK_POS_BOTTOM:
	  if (!draw_center)
	    components |= COMPONENT_SOUTH_WEST | COMPONENT_SOUTH | COMPONENT_SOUTH_EAST;

	  r1.x      = x;
	  r1.y      = y + height - ythickness;
	  r1.width  = gap_x;
	  r1.height = ythickness;
	  r2.x      = x + gap_x;
	  r2.y      = y + height - ythickness;
	  r2.width  = gap_width;
	  r2.height = ythickness;
	  r3.x      = x + gap_x + gap_width;
	  r3.y      = y + height - ythickness;
	  r3.width  = width - (gap_x + gap_width);
	  r3.height = ythickness;
	  break;

	case GTK_POS_LEFT:
	  if (!draw_center)
	    components |= COMPONENT_NORTH_WEST | COMPONENT_WEST | COMPONENT_SOUTH_WEST;

	  r1.x      = x;
	  r1.y      = y;
	  r1.width  = xthickness;
	  r1.height = gap_x;
	  r2.x      = x;
	  r2.y      = y + gap_x;
	  r2.width  = xthickness;
	  r2.height = gap_width;
	  r3.x      = x;
	  r3.y      = y + gap_x + gap_width;
	  r3.width  = xthickness;
	  r3.height = height - (gap_x + gap_width);
	  break;

	case GTK_POS_RIGHT:
	  if (!draw_center)
	    components |= COMPONENT_NORTH_EAST | COMPONENT_EAST | COMPONENT_SOUTH_EAST;

	  r1.x      = x + width - xthickness;
	  r1.y      = y;
	  r1.width  = xthickness;
	  r1.height = gap_x;
	  r2.x      = x + width - xthickness;
	  r2.y      = y + gap_x;
	  r2.width  = xthickness;
	  r2.height = gap_width;
	  r3.x      = x + width - xthickness;
	  r3.y      = y + gap_x + gap_width;
	  r3.width  = xthickness;
	  r3.height = height - (gap_x + gap_width);
	  break;
	}

      if (image->background)
	theme_pixbuf_render (image->background, widget,
			     window, NULL, area, components, FALSE,
			     x, y, width, height);
      if (image->gap_start)
	theme_pixbuf_render (image->gap_start, widget,
			     window, NULL, area, COMPONENT_ALL, FALSE,
			     r1.x, r1.y, r1.width, r1.height);
      if (image->gap)
	theme_pixbuf_render (image->gap, widget,
			     window, NULL, area, COMPONENT_ALL, FALSE,
			     r2.x, r2.y, r2.width, r2.height);
      if (image->gap_end)
	theme_pixbuf_render (image->gap_end, widget,
			     window, NULL, area, COMPONENT_ALL, FALSE,
			     r3.x, r3.y, r3.width, r3.height);

      return TRUE;
    }
  else
    return FALSE;
}

static void
draw_hline (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x1,
	    gint          x2,
	    gint          y)
{
  ThemeImage *image;
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=hline, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_HLINE;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_ORIENTATION | THEME_MATCH_STATE;
  match_data.state = state;
  match_data.orientation = GTK_ORIENTATION_HORIZONTAL;

  image = match_theme_image (style, &match_data);
  if (image)
    {
      if (image->background)
	theme_pixbuf_render (image->background, widget,
			     window, NULL, area, COMPONENT_ALL, FALSE,
			     x1, y, (x2 - x1) + 1, 2);
    }
  else
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_hline (
      style, window, state, area, widget, detail, x1, x2, y);
}

static void
draw_vline (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          y1,
	    gint          y2,
	    gint          x)
{
  ThemeImage    *image;
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=vline, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_VLINE;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_ORIENTATION | THEME_MATCH_STATE;
  match_data.state = state;
  match_data.orientation = GTK_ORIENTATION_VERTICAL;

  image = match_theme_image (style, &match_data);
  if (image)
    {
      if (image->background)
	theme_pixbuf_render (image->background, widget,
			     window, NULL, area, COMPONENT_ALL, FALSE,
			     x, y1, 2, (y2 - y1) + 1);
    }
  else
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_vline (
      style, window, state, area, widget, detail, y1, y2, x);
}

static void
draw_shadow (GtkStyle     *style,
             GdkWindow    *window,
             GtkStateType  state,
             GtkShadowType shadow,
             GdkRectangle *area,
             GtkWidget    *widget,
             const gchar  *detail,
             gint          x,
             gint          y,
             gint          width,
             gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=shadow, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_SHADOW;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  if (!draw_simple_image (style, window, area, widget, &match_data, FALSE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_shadow (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

/* This function makes up for some brokeness in gtkrange.c
 * where we never get the full arrow of the stepper button
 * and the type of button in a single drawing function.
 *
 * It doesn't work correctly when the scrollbar is squished
 * to the point we don't have room for full-sized steppers.
 */
static void
reverse_engineer_stepper_box (GtkWidget    *range,
			      GtkArrowType  arrow_type,
			      gint         *x,
			      gint         *y,
			      gint         *width,
			      gint         *height)
{
  gint slider_width = 14, stepper_size = 14;
  gint box_width;
  gint box_height;

  if (range)
    {
      gtk_widget_style_get (range,
			    "slider_width", &slider_width,
			    "stepper_size", &stepper_size,
			    NULL);
    }

  if (arrow_type == GTK_ARROW_UP || arrow_type == GTK_ARROW_DOWN)
    {
      box_width = slider_width;
      box_height = stepper_size;
    }
  else
    {
      box_width = stepper_size;
      box_height = slider_width;
    }

  *x = *x - (box_width - *width) / 2;
  *y = *y - (box_height - *height) / 2;
  *width = box_width;
  *height = box_height;
}

static void
draw_arrow (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state,
	    GtkShadowType shadow,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    GtkArrowType  arrow_direction,
	    gint          fill,
	    gint          x,
	    gint          y,
	    gint          width,
	    gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=arrow, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  if (detail &&
      (strcmp (detail, "hscrollbar") == 0 || strcmp (detail, "vscrollbar") == 0))
    {
      /* This is a hack to work around the fact that scrollbar steppers are drawn
       * as a box + arrow, so we never have
       *
       *   The full bounding box of the scrollbar 
       *   The arrow direction
       *
       * At the same time. We simulate an extra paint function, "STEPPER", by doing
       * nothing for the box, and then here, reverse engineering the box that
       * was passed to draw box and using that
       */
      gint box_x = x;
      gint box_y = y;
      gint box_width = width;
      gint box_height = height;

      reverse_engineer_stepper_box (widget, arrow_direction,
				    &box_x, &box_y, &box_width, &box_height);

      match_data.function = TOKEN_D_STEPPER;
      match_data.detail = (gchar *)detail;
      match_data.flags = (THEME_MATCH_SHADOW | 
			  THEME_MATCH_STATE | 
			  THEME_MATCH_ARROW_DIRECTION);
      match_data.shadow = shadow;
      match_data.state = state;
      match_data.arrow_direction = arrow_direction;

      if (draw_simple_image (style, window, area, widget, &match_data, TRUE,
			     box_x, box_y, box_width, box_height))
	{
	  /* The theme included stepper images, we're done */
	  return;
	}

      /* Otherwise, draw the full box, and fall through to draw the arrow
       */
      match_data.function = TOKEN_D_BOX;
      match_data.detail = (gchar *)detail;
      match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
      match_data.shadow = shadow;
      match_data.state = state;

      if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			      box_x, box_y, box_width, box_height))
	GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_box (
          style, window, state, shadow, area, widget, detail,
          box_x, box_y, box_width, box_height);
    }

#if GTK_CHECK_VERSION(2,9,2)
  if (arrow_direction == GTK_ARROW_NONE)
    return;
#endif

  match_data.function = TOKEN_D_ARROW;
  match_data.detail = (gchar *)detail;
  match_data.flags = (THEME_MATCH_SHADOW | 
		      THEME_MATCH_STATE | 
		      THEME_MATCH_ARROW_DIRECTION);
  match_data.shadow = shadow;
  match_data.state = state;
  match_data.arrow_direction = arrow_direction;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_arrow (
      style, window, state, shadow, area, widget, detail,
      arrow_direction, fill, x, y, width, height);
}

static void
draw_diamond (GtkStyle     *style,
	      GdkWindow    *window,
	      GtkStateType  state,
	      GtkShadowType shadow,
	      GdkRectangle *area,
	      GtkWidget    *widget,
	      const gchar  *detail,
	      gint          x,
	      gint          y,
	      gint          width,
	      gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=diamond, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_DIAMOND;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_diamond (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

static void
draw_string (GtkStyle     *style,
             GdkWindow    *window,
             GtkStateType  state,
             GdkRectangle *area,
             GtkWidget    *widget,
             const gchar  *detail,
             gint          x,
             gint          y,
             const gchar  *string)
{
  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=string, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  if (state == GTK_STATE_INSENSITIVE)
    {
      if (area)
	{
	  gdk_gc_set_clip_rectangle (style->white_gc, area);
	  gdk_gc_set_clip_rectangle (style->fg_gc[state], area);
	}

      gdk_draw_string (window, gtk_style_get_font (style),
                       style->fg_gc[state], x, y, string);

      if (area)
	{
	  gdk_gc_set_clip_rectangle (style->white_gc, NULL);
	  gdk_gc_set_clip_rectangle (style->fg_gc[state], NULL);
	}
    }
  else
    {
      gdk_gc_set_clip_rectangle (style->fg_gc[state], area);
      gdk_draw_string (window, gtk_style_get_font (style),
                       style->fg_gc[state], x, y, string);
      gdk_gc_set_clip_rectangle (style->fg_gc[state], NULL);
    }
}

static void
maybe_check_submenu_state (GtkMenuItem    *menu_item,
                           ThemeMatchData *match_data)
{
  /* Distinguish between active and passive focus, depending on whether the
   * focus is in submenu.
   *
   * Active focus:
   *   function = BOX
   *   state    = PRELIGHT
   *
   * Passive focus:
   *   function = BOX
   *   state    = SELECTED
   */
  if (menu_item->submenu)
    {
      GtkWidget *sub_item;

      sub_item = GTK_MENU_SHELL (menu_item->submenu)->active_menu_item;
      if (sub_item && GTK_WIDGET_STATE (sub_item) != GTK_STATE_NORMAL)
	match_data->state = GTK_STATE_SELECTED;
    }

  /* This is a hack to cover the cases where a menu item is being 
   * used outside the usual a GtkMenu. More specificaly, this covers
   * menu items inside the hildon-desktop custom menu widget. */
  if (!GTK_IS_MENU (GTK_WIDGET (menu_item)->parent) &&
      !gtk_widget_is_focus (GTK_WIDGET (menu_item)) &&
      GTK_WIDGET_STATE (menu_item) != GTK_STATE_NORMAL)
    match_data->state = GTK_STATE_SELECTED;
}

static void
draw_box (GtkStyle     *style,
	  GdkWindow    *window,
 	  GtkStateType  state,
 	  GtkShadowType shadow,
 	  GdkRectangle *area,
 	  GtkWidget    *widget,
	  const gchar  *detail,
	  gint          x,
	  gint          y,
	  gint          width,
	  gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=box, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  if (detail &&
      (strcmp (detail, "hscrollbar") == 0 || strcmp (detail, "vscrollbar") == 0))
    {
      /* We handle this in draw_arrow */
      return;
    }

  match_data.function = TOKEN_D_BOX;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  if (GTK_IS_MENU_ITEM (widget))
    maybe_check_submenu_state (GTK_MENU_ITEM (widget), &match_data);

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height)) {
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_box (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
  }
}

static void
maybe_check_cursor_position (GtkTreeView *treeview,
                             gint         x,
                             gint         y,
                             gint         width,
                             gint         height,
                             ThemeMatchData *match_data)
{
  GtkTreePath *cursor_path;
  GdkRectangle cursor_rect;
  GdkRectangle paint_rect;

  gtk_tree_view_get_cursor (treeview, &cursor_path, NULL);
  if (!cursor_path)
    return;

  /* we only really care about the vertical position here */

  gtk_tree_view_get_background_area (treeview, cursor_path, NULL, &cursor_rect);
  gtk_tree_path_free (cursor_path);

  paint_rect.y = y;
  paint_rect.height = height;

  paint_rect.x = cursor_rect.x = x;
  paint_rect.width = cursor_rect.width = width;

  if (!gdk_rectangle_intersect (&paint_rect, &cursor_rect, &paint_rect))
    return;

  /* We're painting the cursor row background, so distinguish between focused
   * and non-focused treeview so that we can paint the passive focus ring when
   * non-focused. (gtk_paint_focus is used to paint the focus ring when focused.)
   * Knowing that treeview always uses GTK_SHADOW_NONE, it should be (more or
   * less) safe to (ab)use it for passive focus.
   *
   * Passive focus:
   *   function = FLAT_BOX
   *   state    = SELECTED / NORMAL
   *   shadow   = OUT
   *
   * Selection / normal row:
   *   function = FLAT_BOX
   *   state    = SELECTED / NORMAL
   *   shadow   = NONE
   */
  if (!GTK_WIDGET_HAS_FOCUS (treeview))
    match_data->shadow = GTK_SHADOW_OUT;
}

static void
draw_flat_box (GtkStyle     *style,
	       GdkWindow    *window,
	       GtkStateType  state,
	       GtkShadowType shadow,
	       GdkRectangle *area,
	       GtkWidget    *widget,
	       const gchar  *detail,
	       gint          x,
	       gint          y,
	       gint          width,
	       gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=flat_box, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  /* Hack because Gtk doesn't give us the correct widget state */
  if (widget && GTK_IS_ENTRY(widget))
  {
    if (!GTK_WIDGET_IS_SENSITIVE(widget))
      state = GTK_STATE_INSENSITIVE;

    if (GTK_WIDGET_HAS_FOCUS(widget))
      state = GTK_STATE_ACTIVE;
  }

  match_data.function = TOKEN_D_FLAT_BOX;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  /* Special handling for treeview cursor row */
  if (GTK_IS_TREE_VIEW (widget))
    maybe_check_cursor_position (GTK_TREE_VIEW (widget), x, y, width, height, &match_data);

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_flat_box (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

static void
draw_check (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state,
	    GtkShadowType shadow,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          width,
	    gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=check, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_CHECK;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  /* Special casing for GtkCheckButton: We want to set the widget state to
   * ACTIVE to get the correct graphics used in the RC files. Ideally we'd
   * use the FOCUS rules, but this is not possible due to technical limitations
   * in how focus is drawn in sapwood */
  if (GTK_IS_CHECK_BUTTON (widget) &&
      GTK_WIDGET_HAS_FOCUS (widget))
    match_data.state = GTK_STATE_ACTIVE;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_check (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

static void
draw_option (GtkStyle      *style,
	     GdkWindow     *window,
	     GtkStateType  state,
	     GtkShadowType shadow,
	     GdkRectangle *area,
	     GtkWidget    *widget,
	     const gchar  *detail,
	     gint          x,
	     gint          y,
	     gint          width,
	     gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=option, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_OPTION;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  /* Special casing for GtkRadioButton: We want to set the widget state to
   * ACTIVE to get the correct graphics used in the RC files. Ideally we'd
   * use the FOCUS rules, but this is not possible due to technical limitations
   * in how focus is drawn in sapwood */
  if (GTK_IS_RADIO_BUTTON (widget) &&
      GTK_WIDGET_HAS_FOCUS (widget))
    match_data.state = GTK_STATE_ACTIVE;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_option (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

static void
draw_tab (GtkStyle     *style,
	  GdkWindow    *window,
	  GtkStateType  state,
	  GtkShadowType shadow,
	  GdkRectangle *area,
	  GtkWidget    *widget,
	  const gchar  *detail,
	  gint          x,
	  gint          y,
	  gint          width,
	  gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=tab, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_TAB;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.shadow = shadow;
  match_data.state = state;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_tab (
      style, window, state, shadow, area, widget, detail, x, y, width, height);
}

static void
draw_shadow_gap (GtkStyle       *style,
		 GdkWindow      *window,
		 GtkStateType    state,
		 GtkShadowType   shadow,
		 GdkRectangle   *area,
		 GtkWidget      *widget,
		 const gchar    *detail,
		 gint            x,
		 gint            y,
		 gint            width,
		 gint            height,
		 GtkPositionType gap_side,
		 gint            gap_x,
		 gint            gap_width)
{
  ThemeMatchData match_data;

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=shadow_gap, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_SHADOW_GAP;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.flags = (THEME_MATCH_SHADOW | 
		      THEME_MATCH_STATE | 
		      THEME_MATCH_ORIENTATION);
  match_data.shadow = shadow;
  match_data.state = state;

  if (!draw_gap_image (style, window, area, widget, &match_data, FALSE,
		       x, y, width, height, gap_side, gap_x, gap_width))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_shadow_gap (
      style, window, state, shadow, area, widget, detail,
      x, y, width, height, gap_side, gap_x, gap_width);
}

static void
draw_box_gap (GtkStyle       *style,
	      GdkWindow      *window,
	      GtkStateType    state,
	      GtkShadowType   shadow,
	      GdkRectangle   *area,
	      GtkWidget      *widget,
	      const gchar    *detail,
	      gint            x,
	      gint            y,
	      gint            width,
	      gint            height,
	      GtkPositionType gap_side,
	      gint            gap_x,
	      gint            gap_width)
{
  ThemeMatchData match_data;

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=box_gap, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_BOX_GAP;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE;
  match_data.flags = (THEME_MATCH_SHADOW | 
		      THEME_MATCH_STATE | 
		      THEME_MATCH_ORIENTATION);
  match_data.shadow = shadow;
  match_data.state = state;

  if (!draw_gap_image (style, window, area, widget, &match_data, TRUE,
		       x, y, width, height, gap_side, gap_x, gap_width))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_box_gap (
      style, window, state, shadow, area, widget, detail,
      x, y, width, height, gap_side, gap_x, gap_width);
}

static void
draw_expander (GtkStyle        *style,
               GdkWindow       *window,
               GtkStateType     state,
               GdkRectangle    *area,
               GtkWidget       *widget,
               const gchar     *detail,
               gint             center_x,
               gint             center_y,
               GtkExpanderStyle expander_style)
{
  ThemeMatchData match_data;
  gint expander_size = 10;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=expander, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  /* Reusing the arrow theming here as it's flexible enough (almost, we do lose
   * the intermediate states.) It also allows us to use existing gtkrc.
   * XXX Might want to introduce proper keywords for expanders some day.
   */

  if (widget)
    gtk_widget_style_get (widget, "expander-size", &expander_size, NULL);

  match_data.function = TOKEN_D_ARROW;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_STATE | THEME_MATCH_ARROW_DIRECTION;
  match_data.state = state;

  switch (expander_style)
    {
    case GTK_EXPANDER_COLLAPSED:
    case GTK_EXPANDER_SEMI_COLLAPSED:
      match_data.arrow_direction = GTK_ARROW_RIGHT;
      break;
    case GTK_EXPANDER_EXPANDED:
    case GTK_EXPANDER_SEMI_EXPANDED:
      match_data.arrow_direction = GTK_ARROW_DOWN;
      break;
    default:
      g_return_if_reached ();
    }

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
                          center_x - expander_size/2, center_y - expander_size/2, expander_size, expander_size))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_expander (
      style, window, state, area, widget, detail,
      center_x, center_y, expander_style);
}

static void
draw_extension (GtkStyle       *style,
		GdkWindow      *window,
		GtkStateType    state,
		GtkShadowType   shadow,
		GdkRectangle   *area,
		GtkWidget      *widget,
		const gchar    *detail,
		gint            x,
		gint            y,
		gint            width,
		gint            height,
		GtkPositionType gap_side)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=extension, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_EXTENSION;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_SHADOW | THEME_MATCH_STATE | THEME_MATCH_GAP_SIDE;
  match_data.shadow = shadow;
  match_data.state = state;
  match_data.gap_side = gap_side;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_extension (
      style, window, state, shadow, area, widget, detail,
      x, y, width, height, gap_side);
}

static void
draw_focus (GtkStyle     *style,
	    GdkWindow    *window,
	    GtkStateType  state,
	    GdkRectangle *area,
	    GtkWidget    *widget,
	    const gchar  *detail,
	    gint          x,
	    gint          y,
	    gint          width,
	    gint          height)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=focus, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_FOCUS;
  match_data.detail = (gchar *)detail;
  match_data.flags = THEME_MATCH_STATE;
  match_data.state = state;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_focus (
      style, window, state, area, widget, detail, x, y, width, height);
}

static void
draw_slider (GtkStyle      *style,
	     GdkWindow     *window,
	     GtkStateType   state,
	     GtkShadowType  shadow,
	     GdkRectangle  *area,
	     GtkWidget     *widget,
	     const gchar   *detail,
	     gint           x,
	     gint           y,
	     gint           width,
	     gint           height,
	     GtkOrientation orientation)
{
  ThemeMatchData           match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=slider, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_SLIDER;
  match_data.detail = (gchar *)detail;
  match_data.flags = (THEME_MATCH_SHADOW | 
		      THEME_MATCH_STATE | 
		      THEME_MATCH_ORIENTATION);
  match_data.shadow = shadow;
  match_data.state = state;
  match_data.orientation = orientation;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_slider (
      style, window, state, shadow, area, widget, detail,
      x, y, width, height, orientation);
}


static void
draw_handle (GtkStyle      *style,
	     GdkWindow     *window,
	     GtkStateType   state,
	     GtkShadowType  shadow,
	     GdkRectangle  *area,
	     GtkWidget     *widget,
	     const gchar   *detail,
	     gint           x,
	     gint           y,
	     gint           width,
	     gint           height,
	     GtkOrientation orientation)
{
  ThemeMatchData match_data;

  g_return_if_fail (style != NULL);
  g_return_if_fail (window != NULL);

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=handle, state=%s, shadow=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state),
        enum_value_to_string (gtk_shadow_type_get_type (), shadow),
        detail,
        gtk_widget_get_name (widget));

  match_data.function = TOKEN_D_HANDLE;
  match_data.detail = (gchar *)detail;
  match_data.flags = (THEME_MATCH_SHADOW | 
		      THEME_MATCH_STATE | 
		      THEME_MATCH_ORIENTATION);
  match_data.shadow = shadow;
  match_data.state = state;
  match_data.orientation = orientation;

  if (!draw_simple_image (style, window, area, widget, &match_data, TRUE,
			  x, y, width, height))
    GTK_STYLE_CLASS (sapwood_style_parent_class)->draw_handle (
      style, window, state, shadow, area, widget, detail,
      x, y, width, height, orientation);
}

static GdkPixbuf *
set_transparency (const GdkPixbuf *pixbuf, gdouble alpha_percent)
{
        GdkPixbuf *target;
        guchar *data, *current;
        guint x, y, rowstride, height, width;

        g_return_val_if_fail (pixbuf != NULL, NULL);
        g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

        /* Returns a copy of pixbuf with it's non-completely-transparent pixels to
           have an alpha level "alpha_percent" of their original value. */

        target = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

        if (alpha_percent == 1.0)
                return target;
        width = gdk_pixbuf_get_width (target);
        height = gdk_pixbuf_get_height (target);
        rowstride = gdk_pixbuf_get_rowstride (target);
        data = gdk_pixbuf_get_pixels (target);

        for (y = 0; y < height; y++)
        {
                for (x = 0; x < width; x++)
                {
                        /* The "4" is the number of chars per pixel, in this case, RGBA,
                           the 3 means "skip to the alpha" */
                        current = data + (y * rowstride) + (x * 4) + 3;
                        *(current) = (guchar) (*(current) * alpha_percent);
                }
        }

        return target;
}

static GdkPixbuf*
scale_or_ref (GdkPixbuf *src,
              int width,
              int height)
{
        if (width == gdk_pixbuf_get_width (src) &&
            height == gdk_pixbuf_get_height (src))
        {
                return g_object_ref (src);
        }
        else
        {
                return gdk_pixbuf_scale_simple (src,
                                                 width, height,
                                                 GDK_INTERP_BILINEAR);
        }
}

static GdkPixbuf *
render_icon (GtkStyle               *style,
	     const GtkIconSource    *source,
	     GtkTextDirection        direction,
	     GtkStateType            state,
	     GtkIconSize             size,
	     GtkWidget              *widget,
	     const gchar            *detail)
{
	int width = 1;
	int height = 1;
	GdkPixbuf *scaled;
	GdkPixbuf *stated;
	GdkPixbuf *base_pixbuf;
	GdkScreen *screen;
	GtkSettings *settings;

	/* Oddly, style can be NULL in this function, because
	 * GtkIconSet can be used without a style and if so
	 * it uses this function.
	 */

	base_pixbuf = gtk_icon_source_get_pixbuf (source);

	g_return_val_if_fail (base_pixbuf != NULL, NULL);

	if (widget && gtk_widget_has_screen (widget))
	{
		screen = gtk_widget_get_screen (widget);
		settings = gtk_settings_get_for_screen (screen);
	}
	else if (style->colormap)
	{
		screen = gdk_colormap_get_screen (style->colormap);
		settings = gtk_settings_get_for_screen (screen);
	}
	else
	{
		settings = gtk_settings_get_default ();
		GTK_NOTE (MULTIHEAD,
			  g_warning ("Using the default screen for gtk_default_render_icon()"));
	}

	if (size != (GtkIconSize) -1 && !gtk_icon_size_lookup_for_settings (settings, size, &width, &height))
	{
		g_warning (G_STRLOC ": invalid icon size '%d'", size);
		return NULL;
	}

	/* If the size was wildcarded, and we're allowed to scale, then scale; otherwise,
	 * leave it alone.
	 */
	if (size != (GtkIconSize)-1 && gtk_icon_source_get_size_wildcarded (source))
		scaled = scale_or_ref (base_pixbuf, width, height);
	else
		scaled = g_object_ref (base_pixbuf);

	/* If the state was wildcarded, then generate a state. */
	if (gtk_icon_source_get_state_wildcarded (source))
	{
		if (state == GTK_STATE_INSENSITIVE)
		{
			stated = set_transparency (scaled, 0.3);
			gdk_pixbuf_saturate_and_pixelate (stated, stated, 0.1, FALSE);

			g_object_unref (scaled);
		}
		else if (state == GTK_STATE_PRELIGHT)
		{
			stated = gdk_pixbuf_copy (scaled);

			gdk_pixbuf_saturate_and_pixelate (scaled, stated, 1.2, FALSE);

			g_object_unref (scaled);
		}
		else
		{
			stated = scaled;
		}
	}
	else
		stated = scaled;

	return stated;
}

static void
draw_layout (GtkStyle     *style,
	     GdkWindow    *window,
	     GtkStateType  state_type,
	     gboolean      use_text,
	     GdkRectangle *area,
	     GtkWidget    *widget,
	     const char   *detail,
	     gint          x,
	     gint          y,
	     PangoLayout  *layout)
{
  /* Simply draw the text, without any of the fancy effects for insensitive
   * state in the default theme engine */
  GdkGC *gc;

  /* FIXME: memory leak */
  LOG ("widget=%s, primitive=layout, state=%s, detail='%s', name='%s'",
        G_OBJECT_TYPE_NAME (widget),
        enum_value_to_string (gtk_state_type_get_type (), state_type),
        detail,
        gtk_widget_get_name (widget));

  gc = use_text ? style->text_gc[state_type] : style->fg_gc[state_type];

  if (area)
    gdk_gc_set_clip_rectangle (gc, area);

  /* Draw the optional shadow */
  if (SAPWOOD_RC_STYLE (style->rc_style)->has_shadow)
    gdk_draw_layout_with_colors (window, gc, x + 1, y + 1, layout, &SAPWOOD_RC_STYLE (style->rc_style)->shadowcolor, NULL);

  gdk_draw_layout (window, gc, x, y, layout);

  if (area)
    gdk_gc_set_clip_rectangle (gc, NULL);
}

void
sapwood_style_register_types (GTypeModule *module)
{
  sapwood_style_register_type (module);
}

static void
sapwood_style_init (SapwoodStyle *style)
{
}

static void
sapwood_style_class_init (SapwoodStyleClass *klass)
{
  GtkStyleClass *style_class = GTK_STYLE_CLASS (klass);

  style_class->draw_hline = draw_hline;
  style_class->draw_vline = draw_vline;
  style_class->draw_shadow = draw_shadow;
  style_class->draw_arrow = draw_arrow;
  style_class->draw_diamond = draw_diamond;
  style_class->draw_string = draw_string;
  style_class->draw_box = draw_box;
  style_class->draw_flat_box = draw_flat_box;
  style_class->draw_check = draw_check;
  style_class->draw_option = draw_option;
  style_class->draw_tab = draw_tab;
  style_class->draw_shadow_gap = draw_shadow_gap;
  style_class->draw_box_gap = draw_box_gap;
  style_class->draw_extension = draw_extension;
  style_class->draw_focus = draw_focus;
  style_class->draw_slider = draw_slider;
  style_class->draw_handle = draw_handle;
  style_class->render_icon = render_icon;
  style_class->draw_expander = draw_expander;
  style_class->draw_layout = draw_layout;
}

static void
sapwood_style_class_finalize (SapwoodStyleClass *klass)
{
}
