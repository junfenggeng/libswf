#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "swf.h"

#define DEFAULT_WIDTH  400
#define DEFAULT_HEIGHT 200

struct swf_movie movie;

/* forward definition of actual painting function for our drawing area widget
 */
static void paint (GtkWidget      *widget,
		   GdkEventExpose *eev,
                   gpointer        data);

gint
main (gint    argc,
      gchar **argv)
{
	GtkWidget *window;
	GtkWidget *canvas;

	int status;
	char* filename;

	gtk_init (&argc, &argv);

	/* create a new top level window
	 */
	window   = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	/* make the gtk terminate the process the close button is pressed
	 */
	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (gtk_main_quit), NULL);

	/* create a new drawing area widget
	 */
	canvas = gtk_drawing_area_new ();

	/* set a requested (minimum size) for the canvas
	 */
	gtk_widget_set_size_request (canvas, DEFAULT_WIDTH, DEFAULT_HEIGHT);

	/* connect our drawing method to the "expose" signal
	 */
	g_signal_connect (G_OBJECT (canvas), "expose-event",
			  G_CALLBACK (paint),
			  NULL  /*< here we can pass a pointer to a custom data structure */
		);

	/* pack canvas widget into window
	 */
	gtk_container_add (GTK_CONTAINER (window), canvas);

	/* show window and all it's children (just the canvas widget)
	 */
	gtk_widget_show_all (window);

	filename = "test/sample.swf";

	status = swf_open(&movie, filename);
	if (status != 0) {
		printf("failed to load swf movie from [%s]\n", filename);
		goto out;
	}

	swf_print_info(&movie);
 
	/* enter main loop
	 */ 
	gtk_main ();

out:
	return 0;
}


/* the actual function invoked to paint the canvas
 * widget, this is where most cairo painting functions
 * will go
 */
static void
paint (GtkWidget      *widget,
       GdkEventExpose *eev,
       gpointer        data)
{
	gint width, height;
	gint i;
	cairo_t *cr;
	struct swf_graphics gc;

	width  = widget->allocation.width;
	height = widget->allocation.height;
  
	cr = gdk_cairo_create (widget->window);
	gc.cr = cr;

	swf_advance(&movie);
	swf_render(&movie, &gc);
  
	/* clear background */
	cairo_set_source_rgb (cr, 1,1,1);
	cairo_paint (cr);

	cairo_set_source_rgb (cr, 0,0,1);

	cairo_move_to (cr, 0, 0);
	cairo_rel_line_to (cr, 5,  10);
	cairo_rel_line_to (cr, 5, -10);
	cairo_stroke (cr);

	cairo_destroy (cr);
}

