/*
 * This file is part of Other Maemo Weather(omweather)
 *
 * Copyright (C) 2006 Vlad Vasiliev
 * Copyright (C) 2006 Pavel Fialko
 * 	for the code
 *        
 * Copyright (C) 2008 Andrew Zhilin
 *		      az@pocketpcrussia.com 
 *	for default icon set (Glance)
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
*/
/*******************************************************************************/
#include "weather-utils.h"
#include "weather-common.h"
/*******************************************************************************/
float convert_wind_units(int to, float value){
    float	result = value;
    switch(to){
	    default:
	    case METERS_S: result *= 10.0f / 36.0f; break;
/*	    case KILOMETERS_S: result /= 3600.0f; break;
 *
	    case MILES_S: result /= (1.609344f * 3600.0f); break;
	    case METERS_H: result *= 1000.0f; break;
*/	    case KILOMETERS_H: result *= 1.0f; break;
	    case MILES_H: result /= 1.609344f; break;
	}
    return result;
}
/*******************************************************************************/
void set_font_size(GtkWidget *widget, char font_size){
    PangoFontDescription *pfd = NULL;
#ifndef RELEASE
    fprintf(stderr,"BEGIN %s(): \n", __PRETTY_FUNCTION__);
#endif
    if(!widget)
	return;
    pfd = pango_font_description_copy( 
            pango_context_get_font_description(gtk_widget_get_pango_context(widget)));
    pango_font_description_set_absolute_size(pfd, font_size * PANGO_SCALE);	    
    gtk_widget_modify_font(GTK_WIDGET(widget), NULL);   /* this function is leaking */
    gtk_widget_modify_font(GTK_WIDGET(widget), pfd);   /* this function is leaking */
    pango_font_description_free(pfd);
}
/*******************************************************************************/
void set_font_color(GtkWidget *widget, guint16 red, guint16 green, guint16 blue){
    PangoAttribute	*attr;
    PangoAttrList	*attrs = NULL;

    if(!widget)
	return;

    attrs = pango_attr_list_new();
    attr = pango_attr_foreground_new(red,green,blue);
    attr->start_index = 0;
    attr->end_index = G_MAXUINT;
    pango_attr_list_insert(attrs, attr);
    /* Set the attributes */
    g_object_set(widget, "attributes", attrs, NULL);
    pango_attr_list_unref(attrs);
}
/*******************************************************************************/
/* Convert Celsius temperature to Farenhait temperature */
int c2f(int temp){
    return (temp * 1.8f ) + 32;
}
/*******************************************************************************/
void swap_temperature(int *hi, int *low){
    int tmp;
    
    tmp = *hi; *hi = *low; *low = tmp;
}
/*******************************************************************************/
void set_background_color(GtkWidget *widget, GdkColor *bgc){
/* undo previos changes */
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, NULL);
/* set one color for all states of widget */
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, bgc);
}
/*******************************************************************************/
