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
#ifndef _weather_popup_h
#define _weather_popup_h 1
/*******************************************************************************/
#include "weather-common.h"
/*******************************************************************************/
extern	gchar		path_large_icon[];
extern	WeatherSource	weather_sources[];
/*******************************************************************************/
void popup_window_destroy(void);
GtkWidget* create_sun_time_widget(GSList *day);
GtkWidget* create_moon_phase_widget(GSList *current);
GtkWidget* create_time_updates_widget(GSList *current);

void weather_window_popup(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void settings_button_handler(GtkWidget *button, GdkEventButton *event,
                                                            gpointer user_data);
void refresh_button_handler(GtkWidget *button, GdkEventButton *event,
                                                            gpointer user_data);
void popup_close_button_handler(GtkWidget *button, GdkEventButton *event,
                                                            gpointer user_data);
GtkWidget* create_day_tab(GSList *current, GSList *day, gchar **day_name);
GtkWidget* create_current_tab(GSList *current);
GtkWidget* create_copyright_widget(const gchar *text, const gchar *image);
/*******************************************************************************/
extern void weather_window_settings(GtkWidget *widget, GdkEvent *event,
				    gpointer user_data);
/*extern void pre_update_weather(void);*/
extern void update_weather(gboolean show_update_window);
extern void set_font_size(GtkWidget *widget, char font_size);
extern void set_font_color (GtkWidget *widget, guint16 red, guint16 green, guint16 blue);
extern int c2f(int temp);
extern void swap_temperature(int *hi, int *low);
extern gpointer hash_table_find(gpointer key, gboolean search_short_name);
extern float convert_wind_units(int to, float value);
extern GtkWidget* create_button_with_image(const char *path, const char *image_name,
				    int image_size, gboolean with_border);
/*******************************************************************************/
#endif
