/* This file is part of sapwood
 * This file was a part of hildon examples
 *
 * Copyright (C) 2008,2009 Nokia Corporation, all rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <gtk/gtk.h>

static GtkTextBuffer *buffer = NULL;

static void
text_view_mapped                         (GtkWidget* widget)
{
  GError* error = NULL;
  GMappedFile* file = g_mapped_file_new (TOP_SRCDIR G_DIR_SEPARATOR_S "COPYING", FALSE, &error);
  gint i;

  if (error)
    {
      g_warning ("error opening file: %s",
                 error->message);
      g_error_free (error);
      return;
    }

  for (i = 0; i < 14; i++)
    {
      GtkTextIter  iter;
      gtk_text_buffer_get_end_iter (buffer, &iter);

      gtk_text_buffer_insert (buffer, &iter,
                              g_mapped_file_get_contents (file),
                              g_mapped_file_get_length (file));
    }

  g_mapped_file_free (file);
}

int
main                                            (int    argc,
                                                 char **argv)
{
    GtkWidget *textview;
    GtkWidget *win;

    gtk_rc_add_default_file (TOP_SRCDIR G_DIR_SEPARATOR_S "tests" G_DIR_SEPARATOR_S "large-window.gtkrc");
    gtk_init (&argc, &argv);

    /* Window and vbox to pack everything */
    win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* The text view */
    textview = gtk_text_view_new ();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

    /* Pack all widgets */
    gtk_container_set_border_width (GTK_CONTAINER (win), 20);
    GtkWidget *pannable_area;
    pannable_area = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (pannable_area), GTK_WIDGET (textview));
    gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (pannable_area));

    /* Connect signals */
    g_signal_connect_after (textview, "realize",
                            G_CALLBACK (text_view_mapped), NULL);

    g_timeout_add (3000, (GSourceFunc)gtk_main_quit, NULL);

    /* Run example */
    gtk_widget_show_all (win);
    gtk_main ();

    return 0;
}

