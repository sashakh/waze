/*
 * Yattm 
 *
 * Copyright (C) 1999, Torrey Searle <tsearle@uci.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#include "roadmap_progress.h"


typedef struct {
    int tag;
    GtkWidget * progress_meter;
    GtkWidget * progress_window;
    // unsigned long size;
} progress_window_data;

static GList *bars = NULL;
static int last = 0;

static void destroy(GtkWidget * widget, gpointer data)
{
    progress_window_data * pwd = data;
    GList * l;

    for(l = bars; l; l = l->next)
    {
	if(pwd == l->data) {
	    bars = g_list_remove_link(bars, l);
	    g_free(pwd);

	    /* small hack - reset last if there are no more bars */
	    if(bars == NULL)
		last = 0;

	    break;
	}
    }
}

// int roadmap_progress_new( char * filename, unsigned long size )
// int roadmap_progress_new( unsigned long size )
int roadmap_progress_new( void )
{
    progress_window_data *pwd = g_new0(progress_window_data, 1);
    
    // GtkWidget * vbox = gtk_vbox_new( FALSE, 5);
    // GtkWidget * label;
    // gchar buff[2048];

    // pwd->size = size;
    pwd->tag = ++last;

    // g_snprintf( buff, sizeof(buff), _("Transfering %s"), filename);
    // label = gtk_label_new(buff);

    // gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 5);
    // gtk_widget_show(label);

    pwd->progress_meter = gtk_progress_bar_new();
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pwd->progress_meter), 
	    GTK_PROGRESS_LEFT_TO_RIGHT );


    pwd->progress_window = gtk_dialog_new();
    // gtk_window_set_position(GTK_WINDOW(pwd->progress_window),
    // 				GTK_WIN_POS_MOUSE);
    // gtk_container_add(GTK_CONTAINER(pwd->progress_window), vbox);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pwd->progress_window)->vbox),
    	pwd->progress_meter, FALSE, TRUE, 0);
    gtk_widget_show(pwd->progress_meter);
    gtk_signal_connect( GTK_OBJECT(pwd->progress_window), "destroy",
				GTK_SIGNAL_FUNC(destroy), pwd );

    // gtk_widget_show(vbox);
    gtk_widget_show(pwd->progress_window);

    bars = g_list_append(bars, pwd);

    return pwd->tag;
}
	
void roadmap_progress_update(int tag, int total, int progress)
{
    GList * l;
    for(l = bars; l; l=l->next)
    {
	progress_window_data * pwd = l->data;
	if(pwd->tag == tag) {
	    gtk_progress_bar_update
		(GTK_PROGRESS_BAR(pwd->progress_meter),
		(double)progress/(double)total);
	    break;
	}
    }
}

void roadmap_progress_close(int tag)
{
    GList * l;
    for(l = bars; l; l=l->next)
    {
	progress_window_data * pwd = l->data;
	if(pwd->tag == tag) {
	    gtk_widget_destroy(pwd->progress_window);
	    break;
	}
    }
}
