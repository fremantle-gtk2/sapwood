/*
 * This file is part of sapwood
 *
 * Copyright (C) 2007 Nokia Corporation. 
 *
 * Contact: Tommi Komulainen <tommi.komulainen@nokia.com>
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
 */

#include <gtk/gtk.h>

static gboolean
window_expose_event (GtkWidget     * widget,
                     GdkEventExpose* expose,
                     gpointer        user_data G_GNUC_UNUSED)
{
  cairo_t* cr = gdk_cairo_create (widget->window);

  cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
    cairo_paint (cr);
  cairo_restore (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_rectangle (cr,
                   widget->allocation.x - (widget->allocation.width - 20) / 2,
                   widget->allocation.y,
                   20,
                   widget->allocation.height);
  cairo_set_source_rgba (cr, 0.5, 0.0, 0.0, 0.5);
  cairo_fill (cr);
  cairo_paint (cr);

  cairo_destroy (cr);

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *align;
  GtkWidget *button;

  gtk_rc_add_default_file ("gradient.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  button = gtk_button_new_with_label ("OK");

  if (argc > 1 && gdk_screen_is_composited (gtk_widget_get_screen (window)))
    {
      gtk_widget_set_colormap (window,
                               gdk_screen_get_rgba_colormap (gtk_widget_get_screen (window)));

      gtk_widget_set_app_paintable (window, TRUE);

      g_signal_connect (window, "expose-event",
                        G_CALLBACK (window_expose_event), NULL);
    }
  else
    {
      g_print ("not ");
    }

  g_print ("using rgba colormap\n");

  gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);

  gtk_container_add (GTK_CONTAINER (window), align);
  gtk_container_add (GTK_CONTAINER (align), button);

  gtk_widget_set_size_request (button, 100, 100);

  gtk_widget_show_all (window);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_main ();

  return 0;
}
