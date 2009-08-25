/* This file is part of herzi's playground
 *
 * AUTHORS
 *     Sven Herzberg  <sven@lanedo.com>
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

static GtkWidget* button;
static GtkWidget* toggle_button;

static void
update_width (GtkRange * range,
              GtkWidget* widget)
{
  GtkWidget* widgets[] = {button, toggle_button};
  int i;

  for (i = 0; i < G_N_ELEMENTS (widgets); i++)
    {
      int width  = -1;
      int height = -1;

      widget = widgets[i];

      gtk_widget_get_size_request (widget, &width, &height);

      width = gtk_range_get_value (range);

      gtk_widget_set_size_request (widget, width, height);
    }
}

static void
update_height (GtkRange * range,
               GtkWidget* widget)
{
  GtkWidget* widgets[] = {button, toggle_button};
  int i;

  for (i = 0; i < G_N_ELEMENTS (widgets); i++)
    {
      int width  = -1;
      int height = -1;

      widget = widgets[i];

      gtk_widget_get_size_request (widget, &width, &height);

      height = gtk_range_get_value (range);

      gtk_widget_set_size_request (widget, width, height);
    }
}

int
main (int   argc,
      char**argv)
{
  GtkWidget* window;
  GtkWidget* table;
  GtkWidget* width_label;
  GtkWidget* width_scale;
  GtkWidget* height_label;
  GtkWidget* height_scale;

  gtk_rc_add_default_file ("cropping.gtkrc");
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  table  = gtk_table_new (3, 2, FALSE);
  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  toggle_button = gtk_toggle_button_new_with_label ("Sliff");

  width_label = gtk_label_new ("Width:");
  width_scale = gtk_hscale_new_with_range (1.0, 300.0, 1.0);

  height_label = gtk_label_new ("Height:");
  height_scale = gtk_hscale_new_with_range (1.0, 300.0, 1.0);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gtk_widget_destroy), window);
  g_signal_connect (width_scale, "value-changed",
                    G_CALLBACK (update_width), button);
  g_signal_connect (height_scale, "value-changed",
                    G_CALLBACK (update_height), button);

  gtk_table_attach (GTK_TABLE (table), button,
                    0, 1, 0, 1,
                    GTK_EXPAND | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK,
                    0, 0);
  gtk_table_attach (GTK_TABLE (table), toggle_button,
                    1, 2, 0, 1,
                    GTK_EXPAND | GTK_SHRINK, GTK_EXPAND | GTK_SHRINK,
                    0, 0);
  gtk_table_attach (GTK_TABLE (table), width_label,
                    0, 1, 1, 2,
                    GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_table_attach (GTK_TABLE (table), width_scale,
                    1, 2, 1, 2,
                    GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_table_attach (GTK_TABLE (table), height_label,
                    0, 1, 2, 3,
                    GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_table_attach (GTK_TABLE (table), height_scale,
                    1, 2, 2, 3,
                    GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_container_add (GTK_CONTAINER (window), table);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}

