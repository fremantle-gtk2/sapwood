/* This file is part of GTK+ Sapwood Engine
 *
 * AUTHORS
 *     Sven Herzberg  <sven@imendio.com>
 *
 * Copyright (C) 2009  Sven Herzberg
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <gtk/gtk.h>

static gboolean
window_expose_event (GtkWidget     * widget,
                     GdkEventExpose* event,
                     gpointer        user_data)
{
  cairo_t* cr = gdk_cairo_create (event->window);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return FALSE;
}

int
main (int   argc,
      char**argv)
{
  GtkWidget* button;
  GtkWidget* entry;
  GtkWidget* table;
  GtkWidget* window;

  gtk_rc_add_default_file ("rgba-demo.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  table  = gtk_table_new (1, 2, FALSE);
  entry  = gtk_entry_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);

  gtk_container_add (GTK_CONTAINER (window), table);
  gtk_table_attach  (GTK_TABLE (table), entry,
                     0, 1, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     0, 0);
  gtk_table_attach  (GTK_TABLE (table), button,
                     0, 1, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     0, 0);

  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (window, "expose-event",
                    G_CALLBACK (window_expose_event), NULL);

  gtk_table_set_row_spacings (GTK_TABLE (table), 12);

  g_signal_connect (entry, "expose-event",
                    G_CALLBACK (window_expose_event), NULL);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gtk_widget_destroy), window);

  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}

