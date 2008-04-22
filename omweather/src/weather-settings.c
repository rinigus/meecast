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
#include "weather-settings.h"
#include "weather-locations.h"
#include "weather-help.h"
#include "weather-utils.h"
#include "build"
#if defined (BSD) && !_POSIX_SOURCE
    #include <sys/dir.h>
    typedef struct dirent Dirent;
#else
    #include <dirent.h>
    #include <linux/fs.h>
    typedef struct dirent Dirent;
#endif
/*******************************************************************************/
/* Hack for Maemo SDK 2.0 */
#ifndef DT_DIR
#define DT_DIR 4
#endif
/*******************************************************************************/
#define OMW_RESPONSE_ADD_CUSTOM_STATION 10000
#define ZERO(type, name) type name; memset(&name, 0, sizeof name)
#define SIG_TIMER_EXPIRATION SIGRTMIN
#define CLOCK_TYPE CLOCK_MONOTONIC
/*******************************************************************************/
static GtkWidget    *countries,
		    *states,
		    *stations,
		    *icon_size,
		    *layout_type,
		    *update_time,
		    *temperature_unit,
		    *days_number,
		    *custom_station_name,
		    *custom_station_code,
		    *units,
		    *iconset,
		    *wunits,
		    *valid_time_list,
		    *time_2switch_list,
		    *station_list_view,
		    *window_add_station;
static gboolean flag_update_station = FALSE; /* Flag update station list */
static gchar *_weather_station_id_temp; /* Temporary value for weather_station_id */

/*******************************************************************************/
void add_station_to_user_list(gchar *weather_station_name,
				gchar *weather_station_id, gboolean is_gps){
    GtkTreeIter		iter;
    
    /* Add station to stations list */
    gtk_list_store_append(app->user_stations_list, &iter);
    gtk_list_store_set(app->user_stations_list, &iter,
#ifdef HILDON
                                0, weather_station_name,
                                1, weather_station_id,
                                2, is_gps,
#else
                                0, weather_station_name,
                                1, weather_station_id,
#endif
                                -1);
			
    /* Set it station how current (for GPS stations) */				
    if(is_gps && app->gps_must_be_current){
	if(app->config->current_station_id != NULL)
	    g_free(app->config->current_station_id);
	app->config->current_station_id = g_strdup(weather_station_id);
	if(app->config->current_station_name)
	    g_free(app->config->current_station_name);
	app->config->current_station_name = g_strdup(weather_station_name);
    }
}
/*******************************************************************************/
#ifdef HILDON
void delete_all_gps_stations(void){
    gboolean		valid;
    GtkTreeIter		iter;
    gchar		*station_name = NULL,
	    		*station_code = NULL;
    gboolean		is_gps = FALSE;

    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
    while(valid){
    		gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                        	    &iter,
                    		    0, &station_name,
                        	    1, &station_code,
				    2, &is_gps,
                        	    -1);
    		if(is_gps){
		    if(app->config->current_station_id &&
			!strcmp(app->config->current_station_id,station_code) &&
			app->config->current_station_name && 
			!strcmp(app->config->current_station_name,station_name)){
		        /* deleting current station */
		        app->gps_must_be_current = TRUE;
		       	g_free(app->config->current_station_id);
            		g_free(app->config->current_station_name);					
		        app->config->current_station_id = NULL;
			app->config->current_station_name = NULL;
        		app->config->previos_days_to_show = app->config->days_to_show;
            	    }
		    else
			app->gps_must_be_current = FALSE;		    
		    valid = gtk_list_store_remove(app->user_stations_list, &iter);
		}else
		    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                                        &iter);					
							
    	    }
    /* Set new current_station */
    if(!app->config->current_station_id){
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
	if(valid){
	    gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                        	    &iter,
                    		    0, &station_name,
                        	    1, &station_code,
				    2, &is_gps,
                        	    -1);
	    app->config->current_station_id = g_strdup(station_code);
	    app->config->current_station_name = g_strdup(station_name);
	}		    	
    }
}
#endif
/*******************************************************************************/
void changed_country(void){
    GtkTreeModel	*model;
    GtkTreeIter		iter;
    gchar		*country_name = NULL;
    long		regions_start = -1,
			regions_end = -1,
			regions_number = 0;
    
    /* clear regions list */
    if(app->regions_list)
	gtk_list_store_clear(app->regions_list);
    /* clear locations list */
    if(app->stations_list)
	gtk_list_store_clear(app->stations_list);    

    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(countries), &iter)){
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(countries));
	gtk_tree_model_get(model, &iter, 0, &country_name,
					 1, &regions_start,
					 2, &regions_end,
					    -1);
	if(app->regions_list)
	    gtk_list_store_clear(app->regions_list);
	app->regions_list = create_items_list(REGIONSFILE, regions_start,
						regions_end, &regions_number);

	gtk_combo_box_set_row_span_column((GtkComboBox*)states, 0);
	gtk_combo_box_set_model((GtkComboBox*)states,
				(GtkTreeModel*)app->regions_list);

	/* if region is one then set it active and disable combobox */
	if(regions_number < 2){
	    gtk_combo_box_set_active((GtkComboBox*)states, 0);
	    gtk_widget_set_sensitive(states, FALSE);
	}
	else{
	    gtk_combo_box_set_active((GtkComboBox*)states, -1);
	    gtk_widget_set_sensitive(states, TRUE);
	}

	g_free(app->config->current_country);
	app->config->current_country = country_name;
    }
}
/*******************************************************************************/
void changed_state(void){
    GtkTreeModel	*model = NULL;
    GtkTreeIter		iter;
    gchar		*state_name = NULL;
    long		stations_start = -1,
			stations_end = -1;

/* clear locations list */
    if(app->stations_list)
	gtk_list_store_clear(app->stations_list);
	
    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(states), &iter)){
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(states));
	gtk_tree_model_get(model, &iter, 0, &state_name,
					 1, &stations_start,
					 2, &stations_end,
					    -1);
	/* clear locations list */
	if(app->stations_list)
	    gtk_list_store_clear(app->stations_list);

	app->stations_list = create_items_list(LOCATIONSFILE, stations_start,
						stations_end, NULL);
	gtk_combo_box_set_row_span_column((GtkComboBox*)stations, 0);
	gtk_combo_box_set_model((GtkComboBox*)stations,
				(GtkTreeModel*)app->stations_list);

	g_free(state_name);
    }
}
/*******************************************************************************/
void changed_stations(void){
    GtkTreeModel	*model = NULL;
    GtkTreeIter		iter;
    gchar		*station_name = NULL,
			*station_id0 = NULL;
    double		station_latitude = 0.0F,
			station_longitude = 0.0F;

    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(stations), &iter)){
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(stations));
	gtk_tree_model_get(model, &iter, 0, &station_name,
					 1, &station_id0,
					 2, &station_latitude,
					 3, &station_longitude,
					-1);
	_weather_station_id_temp = g_strdup(station_id0);
	g_free(station_name);
	g_free(station_id0);
    }
}
/*******************************************************************************/
/* Edit the station */
void weather_window_edit_station(GtkWidget *widget, GdkEvent *event,
                    					    gpointer user_data){
    
    GtkWidget	*window_edit_station,
		*label,
/*		*code_label,*/
		*table,
		*station_name_edit;
/*		*station_code_edit;*/
    
    GtkTreeIter	iter;
    gchar	*selected_station_name = NULL,
	        *station_name = NULL,
	        *station_code = NULL;
    gboolean	valid;
    GtkTreeModel	*model;
    GtkTreeSelection	*selection;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(station_list_view));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(station_list_view));
    if( !gtk_tree_selection_get_selected(selection, NULL, &iter) )
	return;
    gtk_tree_model_get(model, &iter, 0, &selected_station_name, -1); 
    
    /* Create dialog window */
    window_edit_station = gtk_dialog_new_with_buttons(_("Rename Station"),
        						NULL,
							GTK_DIALOG_MODAL,
							NULL);
    /* Add buttons */
    gtk_dialog_add_button(GTK_DIALOG(window_edit_station),
        		    _("OK"), GTK_RESPONSE_ACCEPT);
    gtk_dialog_add_button(GTK_DIALOG(window_edit_station),
        		    _("Cancel"), GTK_RESPONSE_REJECT);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window_edit_station)->vbox),
        		    table = gtk_table_new(2, 2, FALSE), TRUE, TRUE, 0);	    

   /* Add Label and Edit field for station name */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Station:")),
        			0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        			1, 2, 0, 1);
    station_name_edit = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(station_name_edit), 50);
    gtk_entry_set_text(GTK_ENTRY(station_name_edit), selected_station_name);
    gtk_container_add(GTK_CONTAINER(label), station_name_edit);
    /* set size for dialog */
    gtk_widget_set_size_request(GTK_WIDGET(window_edit_station), 350, -1);
    gtk_widget_show_all(window_edit_station);

    /* start dialog */
    switch(gtk_dialog_run(GTK_DIALOG(window_edit_station))){
	case GTK_RESPONSE_ACCEPT:/* Press Button Ok */
	    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
	    while(valid){
    		gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                        	    &iter,
                    		    0, &station_name,
                        	    1, &station_code,
                        	    -1);
    		if(!strcmp(selected_station_name, station_name)){
		    /* update current station name */
		    g_free(station_name);
		    gtk_list_store_remove(app->user_stations_list, &iter);
		    
		    /* Add station to stations list */
                    add_station_to_user_list( g_strdup(gtk_entry_get_text(GTK_ENTRY(station_name_edit))),
					    station_code, FALSE);
		    if(app->config->current_station_name)
			g_free(app->config->current_station_name);
		    app->config->current_station_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(station_name_edit)));
		    new_config_save(app->config);
		    flag_update_station = TRUE;
		    weather_frame_update(TRUE);
        	    break;
		}else{
		    g_free(station_name);
		    g_free(station_code);
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                                        &iter);
    	    }
	break;
	default:
	break;
    }
    if(selected_station_name)
        g_free(selected_station_name);
    gtk_widget_destroy(window_edit_station);
}
/*******************************************************************************/
/* Delete station from list */
static gboolean weather_delete_station(GtkWidget *widget, GdkEvent *event,
                    					    gpointer user_data){
    GtkWidget		*dialog;
    GtkTreeIter		iter;
    gchar		*station_selected = NULL,
			*station_name = NULL,
			*station_code = NULL;
    GtkTreeModel	*model;
    GtkTreeSelection	*selection;
    gboolean		valid;
    gint		result = GTK_RESPONSE_NONE;
    GtkTreePath		*path;
#ifdef HILDON
    gboolean 		is_gps = FALSE;
#endif
#ifndef RELEASE    
    fprintf(stderr,"%s()\n", __PRETTY_FUNCTION__);
#endif
/* create confirm dialog */
    dialog = gtk_message_dialog_new(NULL,
                            	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                            	    GTK_MESSAGE_QUESTION,
                            	    GTK_BUTTONS_NONE,
                            	    _("Are you sure to want delete this station ?"));
    gtk_dialog_add_button(GTK_DIALOG(dialog),
        		    _("Yes"), GTK_RESPONSE_YES);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
        		    _("No"), GTK_RESPONSE_NO);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if(result != GTK_RESPONSE_YES)
	return FALSE;
/* search station for delete */    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(station_list_view));
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(station_list_view));
    if( !gtk_tree_selection_get_selected(selection, NULL, &iter) )
	return FALSE;
 
    gtk_tree_model_get(model, &iter, 0, &station_selected, -1); 
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
    while(valid){
        gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                            &iter,
#ifdef HILDON
                            0, &station_name,
                            1, &station_code,
			    2, &is_gps,
#else
                            0, &station_name,
                            1, &station_code,
#endif
                            -1);
	if(!strcmp(station_name, station_selected)){
	    path = gtk_tree_model_get_path(GTK_TREE_MODEL(app->user_stations_list),
					    &iter);
#ifdef HILDON
	    if(is_gps){
		/* Reset gps station */
		app->gps_station.id0[0] = 0;
		app->gps_station.name[0] = 0;
		app->gps_station.latitude = 0;
		app->gps_station.longtitude = 0;
	    }
#endif	    	    
	    /* delete selected station */
	    gtk_list_store_remove(app->user_stations_list, &iter);
	    g_free(station_name);
    	    g_free(station_code);
	    /* try to get previos station data */
	    if(gtk_tree_path_prev(path)){
		valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(app->user_stations_list),
						&iter,
						path);
		if(valid){
		    /* set current station */
		    gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                        		&iter,
                        		0, &station_name,
                        		1, &station_code,
                        		-1);
		    /* update current station code */
        	    if(app->config->current_station_id)
            		g_free(app->config->current_station_id);
        	    app->config->current_station_id = station_code;
        	    /* update current station name */
        	    if(app->config->current_station_name)
            		g_free(app->config->current_station_name);
        	    app->config->current_station_name = station_name;
        	    app->config->previos_days_to_show = app->config->days_to_show;
		    break;
		}
		else
		    gtk_tree_path_free(path);
	    }
	    else{/* if no next station than set current station to NO STATION */
		/* update current station code */
        	if(app->config->current_station_id)
            	    g_free(app->config->current_station_id);
        	app->config->current_station_id = NULL;
        	/* update current station name */
        	if(app->config->current_station_name)
            	    g_free(app->config->current_station_name);
        	app->config->current_station_name = NULL;
        	app->config->previos_days_to_show = app->config->days_to_show;
		gtk_tree_path_free(path);
		break;
	    }
	}
	else{
	    g_free(station_name);
    	    g_free(station_code);
	}
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                                        &iter);
    }
    g_free(station_selected);
    weather_frame_update(TRUE);    
    /* Update config file */
    new_config_save(app->config);
    highlight_current_station(GTK_TREE_VIEW(station_list_view));
#ifndef RELEASE
    fprintf(stderr,"End %s()\n", __PRETTY_FUNCTION__);    
#endif
    return TRUE;
}
/*******************************************************************************/
GtkWidget* create_tree_view(GtkListStore* list){
    GtkWidget		*tree_view = NULL;
    GtkTreeSelection	*list_selection = NULL;
    GtkCellRenderer	*renderer = NULL;
    GtkTreeViewColumn	*column = NULL;

/* create the tree view model LIST */
    tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
/* make the list component single selectable */
    list_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    gtk_tree_selection_set_mode(list_selection, GTK_SELECTION_SINGLE);
/* add name column to the view */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);

    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                      "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
/* return widget to caller */
    return tree_view;
}
/*******************************************************************************/
void weather_window_add_custom_station(void){
    GtkWidget	*window_add_custom_station,
		*label,
		*table;
    gboolean	station_code_invalid = TRUE;
/*    GtkTreeIter iter;*/
    gchar       *station_name = NULL,
                *station_code = NULL;

/* Create dialog window */
    window_add_custom_station = gtk_dialog_new_with_buttons(_("Add Custom Station"),
        							NULL, GTK_DIALOG_MODAL,
								NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window_add_custom_station)->vbox),
        		    table = gtk_table_new(4, 2, FALSE), TRUE, TRUE, 0);	    
    gtk_dialog_add_button(GTK_DIALOG(window_add_custom_station),
        		    _("OK"), GTK_RESPONSE_ACCEPT);
    gtk_dialog_add_button(GTK_DIALOG(window_add_custom_station),
        		    _("Cancel"), GTK_RESPONSE_REJECT);
/* Add Custom Station Name  */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Station name:")),
        			0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f) ,
        			1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(label),custom_station_name = gtk_entry_new());
    gtk_entry_set_max_length((GtkEntry*)custom_station_name, 16);
    gtk_entry_set_width_chars((GtkEntry*)custom_station_name, 16);
/* Add Custom Station Code  */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Station code\n (ZIP Code):")),
        			0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 1.f, 0.f, 1.f) ,
        			1, 2, 1, 2);
    gtk_container_add(GTK_CONTAINER(label),custom_station_code = gtk_entry_new());	    
    gtk_entry_set_max_length((GtkEntry*)custom_station_code, 9);
    gtk_entry_set_width_chars((GtkEntry*)custom_station_code, 9);
/* enable help for this window */
    ossohelp_dialog_help_enable(GTK_DIALOG(window_add_custom_station), 
			    OMWEATHER_ADD_CUSTOM_STATION_HELP_ID, app->osso);
    gtk_widget_show_all(window_add_custom_station);
    gtk_widget_set_size_request(GTK_WIDGET(custom_station_name), 230, -1);
    gtk_widget_set_size_request(GTK_WIDGET(custom_station_code), 230, -1);
    gtk_widget_set_size_request(GTK_WIDGET(window_add_custom_station), 450, -1);
    while(station_code_invalid){
	/* start dialog */
	switch(gtk_dialog_run(GTK_DIALOG(window_add_custom_station))){
	    case GTK_RESPONSE_ACCEPT:/* Press Button Ok */
		    station_code_invalid = check_station_code(gtk_entry_get_text((GtkEntry*)custom_station_code));
		    if(!station_code_invalid){
			if(app->config->current_station_id != NULL)
			    g_free(app->config->current_station_id);
			app->config->current_station_id = g_strdup(gtk_entry_get_text((GtkEntry*)custom_station_code));
			station_code = g_strdup(app->config->current_station_id);
			if(app->config->current_station_name)
			    g_free(app->config->current_station_name);
			app->config->current_station_name = g_strdup(gtk_entry_get_text((GtkEntry*)custom_station_name));
			station_name = g_strdup(app->config->current_station_name);
                        
                        /* Add station to stations list */
                        add_station_to_user_list(station_name,station_code, FALSE);

		        /* Update config file */
			new_config_save(app->config);
			flag_update_station = TRUE;
		    }
	    break;
	    default:
		station_code_invalid = FALSE;
	    break;
	}
    }
    gtk_widget_destroy(window_add_custom_station);
}
/*******************************************************************************/
void weather_window_add_station(GtkWidget *widget, GdkEvent *event,
                    					    gpointer user_data){
    GtkWidget	*label,
		*table;
/*    GtkTreeIter iter;*/
    gchar	*station_name = NULL,
		*station_code = NULL;
/* Create dialog window */
    window_add_station = gtk_dialog_new_with_buttons(_("Add Station"),
        						NULL,
							GTK_DIALOG_MODAL,
							NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window_add_station)->vbox),
        		table = gtk_table_new(4, 2, FALSE), TRUE, TRUE, 0);
    gtk_dialog_add_button(GTK_DIALOG(window_add_station),
                    		_("Add Custom Station"), OMW_RESPONSE_ADD_CUSTOM_STATION);
    gtk_dialog_add_button(GTK_DIALOG(window_add_station),
        			_("OK"), GTK_RESPONSE_ACCEPT);
    gtk_dialog_add_button(GTK_DIALOG(window_add_station),
        			_("Cancel"), GTK_RESPONSE_REJECT);				
/* Add Country */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Country:")),
        			0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        			1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(label),countries = gtk_combo_box_new_text());
/* Add State */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("State(Province):")),
        			0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        			1, 2, 2, 3);
    gtk_container_add(GTK_CONTAINER(label),states = gtk_combo_box_new_text());
/* Add Station */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Station(Place):")),
        			0, 1, 3, 4);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f) ,
        			1, 2, 3, 4);
    gtk_container_add(GTK_CONTAINER(label),stations = gtk_combo_box_new_text());
    gtk_widget_show_all(window_add_station);
    gtk_combo_box_set_row_span_column((GtkComboBox*)countries, 0);
    gtk_combo_box_set_model((GtkComboBox*)countries, (GtkTreeModel*)app->countrys_list);
    gtk_widget_set_size_request(GTK_WIDGET(countries), 350, -1);
    gtk_widget_set_size_request(GTK_WIDGET(states), 350, -1);
    gtk_widget_set_size_request(GTK_WIDGET(stations), 350, -1);
/* Set default value to country combo_box */
    gtk_combo_box_set_active(GTK_COMBO_BOX(countries),
				get_active_item_index((GtkTreeModel*)app->countrys_list,
				    -1, app->config->current_country, TRUE));
    changed_country();
    changed_state();
    g_signal_connect((gpointer)countries, "changed",
            		G_CALLBACK (changed_country), NULL);
    g_signal_connect((gpointer)states, "changed",
                	G_CALLBACK (changed_state), NULL);
    g_signal_connect((gpointer) stations, "changed",
            		G_CALLBACK (changed_stations), NULL);
    
    /* enable help for this window */
    ossohelp_dialog_help_enable(GTK_DIALOG(window_add_station), 
				OMWEATHER_ADD_STATION_HELP_ID, app->osso);
    /* run dialog */
    switch(gtk_dialog_run(GTK_DIALOG(window_add_station))){
	default:
	case GTK_RESPONSE_REJECT:/* Press Cancel  */
	break;
	case OMW_RESPONSE_ADD_CUSTOM_STATION:/* Press Custom station add  */
	    weather_window_add_custom_station();
	break;
	case GTK_RESPONSE_ACCEPT:/* Press Button Ok */
	    if (gtk_combo_box_get_active(GTK_COMBO_BOX(stations)) == -1) /* Item not selected */
		break;
	    flag_update_station = TRUE;
	    if(app->config->current_station_id != NULL)
	        g_free(app->config->current_station_id);
	    app->config->current_station_id = g_strdup(_weather_station_id_temp);
	    station_code = g_strdup(_weather_station_id_temp);
	    if(app->config->current_station_name)
		g_free(app->config->current_station_name);
	    app->config->current_station_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(stations));
	    station_name = g_strdup(app->config->current_station_name);
	    /* Add station to stations list */
            add_station_to_user_list(station_name,station_code, FALSE);
	    /* Update config file */
	    new_config_save(app->config);
	    weather_frame_update(TRUE);
	    break;
    }
    gtk_widget_destroy(window_add_station); 
}
/*******************************************************************************/
/* Main preference window */
void weather_window_settings(GtkWidget *widget,	GdkEvent *event,
							    gpointer user_data){

    GtkWidget	*window_config = NULL,
		*notebook = NULL,
		*label = NULL,
#ifdef HILDON
                *label_gps = NULL,
                *hbox_gps = NULL,
                *chk_gps = NULL,
#endif
		*time_update_label = NULL,
		*table = NULL,
		*font_color = NULL,
		*background_color = NULL,
		*chk_transparency = NULL,
		*chk_downloading_after_connection = NULL,
		*separate_button = NULL,
		*swap_temperature_button = NULL,
		*hide_station_name = NULL,
		*hide_arrows = NULL,
		*scrolled_window = NULL,
		*button_add = NULL,
		*button_del = NULL,
		*button_ren = NULL,
		*up_icon = NULL,
		*down_icon = NULL,
		*up_station_button = NULL,
		*down_station_button = NULL;
    GtkIconInfo *gtkicon_arrow;
    gboolean	valid = FALSE;
    GtkTreeIter	iter;
    char	flag_update_icon = '\0'; /* Flag update main weather icon of desktop */
#ifndef RELEASE
    char	tmp_buff[1024];
#endif
    gboolean	flag_tuning_warning; /* Flag for show the warnings about tuning images of applet */
    GdkColor	_weather_font_color_temp, /* Temporary for font color */
		background_color_temp;
    static char *temp_string; /* Temporary for the results differnet strdup functions */
    static int result_gtk_dialog_run; /* Temporary for the gtk_dialog_run result */

    not_event = TRUE;
    flag_update_station = FALSE;
    flag_update_icon = FALSE;
    flag_tuning_warning = FALSE; 

    if(!app->dbus_is_initialize)
	weather_initialize_dbus();

    window_config = gtk_dialog_new_with_buttons(_("Other Maemo Weather Settings"),
        				NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					NULL);
    /* add OK button */
    gtk_dialog_add_button(GTK_DIALOG(window_config),
        		    _("OK"), GTK_RESPONSE_ACCEPT);
    /* add CANCEL button */
    gtk_dialog_add_button(GTK_DIALOG(window_config),
        		    _("Cancel"), GTK_RESPONSE_REJECT);
    /* add About button */
    gtk_dialog_add_button(GTK_DIALOG(window_config),
        		    _("About"), OMWEATHER_RESPONSE_ABOUT);
/* Create Notebook widget */
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window_config)->vbox),
        		notebook = gtk_notebook_new(), TRUE, TRUE, 0);
/* Locations tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        	    table = gtk_table_new(7, 4, FALSE),
        	    label = gtk_label_new(_("Locations")));
	    
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        	    label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        	    0, 1, 0, 6);

  
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
					GTK_SHADOW_OUT);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 450, 270);

    station_list_view = create_tree_view(app->user_stations_list);
    gtk_container_add(GTK_CONTAINER(scrolled_window),
                	GTK_WIDGET(station_list_view));
    gtk_container_add(GTK_CONTAINER(label), scrolled_window);
#ifdef HILDON
/* preparing GPS checkbox */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        	    label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        	    0, 1, 6, 7);
    hbox_gps = 	 gtk_hbox_new(FALSE, 0);
    label_gps = gtk_label_new(_("Enable station from GPS:"));
    chk_gps = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_gps),
        			    app->config->gps_station);
    gtk_box_pack_start(GTK_BOX(hbox_gps),
				label_gps, FALSE, FALSE, 2);
    gtk_box_pack_end(GTK_BOX(hbox_gps),
				chk_gps, FALSE, FALSE, 2);
    gtk_container_add(GTK_CONTAINER(label), hbox_gps);
#endif
/* Up Station and Down Station Buttons */
/* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_arrow_up", 16, 0);
    up_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare up_station_button */    
    up_station_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(up_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(up_station_button), up_icon);
    gtk_widget_set_events(up_station_button, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(up_station_button, "clicked",
			G_CALLBACK(up_key_handler), (gpointer)station_list_view);
    gtk_table_attach_defaults(GTK_TABLE(table), up_station_button,
        			1, 2, 2, 3);
/* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_arrow_down", 16, 0);
    down_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare down_station_button */    
    down_station_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(down_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(down_station_button), down_icon);
    gtk_widget_set_events(down_station_button, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(down_station_button, "clicked",
		    	G_CALLBACK(down_key_handler), (gpointer)station_list_view);
    gtk_table_attach_defaults(GTK_TABLE(table), down_station_button,
        			1, 2, 4, 5);
    gtk_table_attach_defaults(GTK_TABLE(table),
        	    label = gtk_alignment_new(0.f, 0.f, 0.f, 0.f),
        	    2, 3, 0, 6);
/* buttons Add, Delete, Rename */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(" "),
        			3, 4, 0, 1);
    button_add = gtk_button_new_with_label(_(" Add "));
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			button_add,
        			3, 4, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(" "),
        			3, 4, 2, 3);
    button_ren = gtk_button_new_with_label(_("Rename"));
    gtk_table_attach_defaults(GTK_TABLE(table),
        			button_ren,
        			3, 4, 3, 4);				
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(" "),
        			3, 4, 4, 5);				
    button_del = gtk_button_new_with_label(_("Delete"));
    gtk_table_attach_defaults(GTK_TABLE(table),
        			button_del,
        			3, 4, 5, 6);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(" "),
        			3, 4, 6, 7);
    g_signal_connect(station_list_view, "cursor-changed",
                	G_CALLBACK(station_list_view_select_handler), NULL);
    g_signal_connect(button_ren, "clicked",
                	G_CALLBACK(weather_window_edit_station), NULL);				
    g_signal_connect(button_del, "clicked",
                	G_CALLBACK(weather_delete_station), NULL);
    g_signal_connect(button_add, "clicked",
                	G_CALLBACK(weather_window_add_station), NULL);

/* Interface tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			table = gtk_table_new(7, 4, FALSE),
        			label = gtk_label_new(_("Interface")));
/* Days to show */
    app->config->days_to_show--; /* count down, because combobox items start with 0 */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
				label = gtk_label_new(_("Visible items:")),
				0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
				1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(label), days_number = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "1");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "3");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "4");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "5");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "6");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "7");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "8");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "9");
    gtk_combo_box_append_text(GTK_COMBO_BOX(days_number), "10");
    switch(app->config->days_to_show){
	case 0:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 0);break;
	case 1:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 1);break;
	case 2:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 2);break;
	case 3:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 3);break;
	default:
	case 4:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 4);break;
	case 5:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 5);break;
	case 6:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 6);break;
	case 7:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 7);break;
	case 8:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 8);break;
	case 9:  gtk_combo_box_set_active(GTK_COMBO_BOX(days_number), 9);break;
    }    
    app->config->days_to_show++; /* count up to return to real value */
/* Layout */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Layout:")),
        			0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f) ,
        			1, 2, 1, 2);
    gtk_container_add(GTK_CONTAINER(label),layout_type = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("One row"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("One column"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Two rows"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Two columns"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Combination"));    
    switch(app->config->icons_layout){
	default:
	case ONE_ROW: 	  gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 0);break;
	case ONE_COLUMN:  gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 1);break;
	case TWO_ROWS:    gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 2);break;
	case TWO_COLUMNS: gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 3);break;
	case COMBINATION: gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 4);break;	
    }
/* Icon set */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Icon set:")),
        			0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 2, 3);
    gtk_container_add(GTK_CONTAINER(label), iconset = gtk_combo_box_new_text());
/* add icons set to list */
    if(create_icon_set_list(iconset) < 2)
	gtk_widget_set_sensitive(iconset, FALSE);
    else
	gtk_widget_set_sensitive(iconset, TRUE);
/* Icon size */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Icon size:")),
        			0, 1, 3, 4);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 3, 4);
    gtk_container_add(GTK_CONTAINER(label),icon_size = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(icon_size), _("Tiny"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(icon_size), _("Small"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(icon_size), _("Medium"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(icon_size), _("Large"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(icon_size), _("Giant"));    
    switch(app->config->icons_size){
	case TINY: gtk_combo_box_set_active(GTK_COMBO_BOX(icon_size), TINY - 1);break;
	case SMALL: gtk_combo_box_set_active(GTK_COMBO_BOX(icon_size), SMALL - 1);break;
	case MEDIUM: gtk_combo_box_set_active(GTK_COMBO_BOX(icon_size), MEDIUM - 1);break;
	default:
	case LARGE: gtk_combo_box_set_active(GTK_COMBO_BOX(icon_size), LARGE - 1);break;
	case GIANT: gtk_combo_box_set_active(GTK_COMBO_BOX(icon_size), GIANT - 1);break;
    }
/* Font color */   
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Font color:")),
        			0, 1, 4, 5);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 4, 5);
    font_color = gtk_color_button_new();
    gtk_container_add(GTK_CONTAINER(label), font_color);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(font_color), &(app->config->font_color));
/* Background color */   
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Background color:")),
        			0, 1, 5, 6);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 5, 6);
    background_color = gtk_color_button_new();
    gtk_container_add(GTK_CONTAINER(label), background_color);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(background_color), &(app->config->background_color));
    if((background_color) && app->config->transparency)
        gtk_widget_set_sensitive(background_color, FALSE);	
    else
        gtk_widget_set_sensitive(background_color, TRUE);
/* Transparency */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Transparency:")),
        			0, 1, 6, 7);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f) ,
        			1, 2, 6, 7);
    gtk_container_add(GTK_CONTAINER(label), chk_transparency = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_transparency),
        			    app->config->transparency);
    g_signal_connect(GTK_TOGGLE_BUTTON(chk_transparency), "toggled",
            		    G_CALLBACK(transparency_button_toggled_handler), background_color);
/* Split */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Show only current weather on first icon:")),
        			0, 1, 7, 8);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f) ,
        			1, 2, 7, 8);
    gtk_container_add(GTK_CONTAINER(label), separate_button = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(separate_button),
        			    app->config->separate);
/* Hide station name button */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Hide station name:")),
        			0, 1, 9, 10);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 9, 10);
    gtk_container_add(GTK_CONTAINER(label), hide_station_name = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_station_name),
        			    app->config->hide_station_name);
/* Hide arrows button */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Hide arrows:")),
        			0, 1, 10, 11);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 10, 11);
    gtk_container_add(GTK_CONTAINER(label), hide_arrows = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_arrows),
        			    app->config->hide_arrows);
/* Units tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			table = gtk_table_new(1, 3, FALSE),
        			label = gtk_label_new(_("Units")));
/* Temperature units */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Temperature units:")),
        			0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(label),temperature_unit = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(temperature_unit), _("Celsius (Metric)"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(temperature_unit), _("Fahrenheit (Imperial)"));
    switch(app->config->temperature_units){
	default:
	case CELSIUS: gtk_combo_box_set_active(GTK_COMBO_BOX(temperature_unit), 0); break;
	case FAHRENHEIT: gtk_combo_box_set_active(GTK_COMBO_BOX(temperature_unit), 1); break;
    }
/* Swap temperature */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_label_new(_("Swap hi/low temperature:")),
        			0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f) ,
        			1, 2, 1, 2);
    gtk_container_add(GTK_CONTAINER(label), swap_temperature_button = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(swap_temperature_button),
        			    app->config->swap_hi_low_temperature);
/* Distance units */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
				label = gtk_label_new(_("Distance units:")),
				0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
				1, 2, 2, 3);
    gtk_container_add(GTK_CONTAINER(label), units = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(units), _("Meters"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(units), _("Kilometers"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(units), _("Miles"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(units), _("Miles (Sea)"));
    switch(app->config->distance_units){
	case METERS: 	gtk_combo_box_set_active(GTK_COMBO_BOX(units), METERS);break;
	default:
	case KILOMETERS: gtk_combo_box_set_active(GTK_COMBO_BOX(units), KILOMETERS);break;
	case MILES:	gtk_combo_box_set_active(GTK_COMBO_BOX(units), MILES);break;
	case SEA_MILES:	gtk_combo_box_set_active(GTK_COMBO_BOX(units), SEA_MILES);break;
    }    
/* Wind units */
    gtk_table_attach_defaults(GTK_TABLE(table),	    
				label = gtk_label_new(_("Wind speed units:")),
				0, 1, 3, 4);
    gtk_table_attach_defaults(GTK_TABLE(table),	    
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
				1, 2, 3, 4);
    gtk_container_add(GTK_CONTAINER(label), wunits = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("m/s"));
/*    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("km/s"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("mi/s"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("m/h"));
*/    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("km/h"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(wunits), _("mi/h"));
    switch(app->config->wind_units){
	default:
	case METERS_S:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), METERS_S);break;
/*	case KILOMETERS_S:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), KILOMETERS_S);break;
	case MILES_S:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), MILES_S);break;
	case METERS_H:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), METERS_H);break;
*/	case KILOMETERS_H:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), KILOMETERS_H);break;
	case MILES_H:  gtk_combo_box_set_active(GTK_COMBO_BOX(wunits), MILES_H);break;
    }    
/* Update tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			table = gtk_table_new(5, 2, FALSE),
        			label = gtk_label_new(_("Update")));
			
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Automatically update data\t\nwhen connecting to the Internet:")),
        			0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 0, 1);
    gtk_container_add(GTK_CONTAINER(label), chk_downloading_after_connection = gtk_check_button_new());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_downloading_after_connection),
        			    app->config->downloading_after_connecting);
    /* Switch time to the next station */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Switch to the next station after:")),
        			0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 1, 2);
    gtk_container_add(GTK_CONTAINER(label), time_2switch_list = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("Never"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("10 seconds"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("20 seconds"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("30 seconds"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("40 seconds"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("50 seconds"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(time_2switch_list), _("60 seconds"));

    switch((guint)(app->config->switch_time / 10)){
	default:
	case 0:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 0);break;
	case 1:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 1);break;
	case 2:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 2);break;
	case 3:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 3);break;
	case 4:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 4);break;
	case 5:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 5);break;
	case 6:  gtk_combo_box_set_active(GTK_COMBO_BOX(time_2switch_list), 6);break;
    }

    /* Vaild time */    
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Valid time for current weather:")),
        			0, 1, 2, 3);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 2, 3);
    gtk_container_add(GTK_CONTAINER(label), valid_time_list = gtk_combo_box_new_text());
    gtk_combo_box_append_text(GTK_COMBO_BOX(valid_time_list), _("1 hour"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(valid_time_list), _("2 hours"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(valid_time_list), _("4 hours"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(valid_time_list), _("8 hours"));
    switch((guint)(app->config->data_valid_interval / 3600)){
	case 1:  gtk_combo_box_set_active(GTK_COMBO_BOX(valid_time_list), 0);break;
	default:
	case 2:  gtk_combo_box_set_active(GTK_COMBO_BOX(valid_time_list), 1);break;
	case 4:  gtk_combo_box_set_active(GTK_COMBO_BOX(valid_time_list), 2);break;
	case 8:  gtk_combo_box_set_active(GTK_COMBO_BOX(valid_time_list), 3);break;
    }

    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Updating of weather data:")),
        			0, 1, 3, 4);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 3, 4);
    gtk_container_add(GTK_CONTAINER(label), update_time = gtk_combo_box_new_text());
    
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_label_new(_("Next update:")),
        			0, 1, 5, 6);
    gtk_table_attach_defaults(GTK_TABLE(table),
        			label = gtk_alignment_new(0, 0.5, 0.f, 0.f),
        			1, 2, 5, 6);

    time_update_label = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(label), time_update_label);
    g_signal_connect(update_time, "changed",
                	G_CALLBACK(update_iterval_changed_handler), time_update_label);
/* Fill update time box */
    gtk_combo_box_set_row_span_column(GTK_COMBO_BOX(update_time), 0);
    gtk_combo_box_set_model(GTK_COMBO_BOX(update_time),
				(GtkTreeModel*)app->time_update_list);
    gtk_combo_box_set_active(GTK_COMBO_BOX(update_time),
    get_active_item_index((GtkTreeModel*)app->time_update_list,
				    app->config->update_interval, NULL, FALSE));
#ifndef RELEASE
/* Events list tab */
    memset(tmp_buff, 0, sizeof(tmp_buff));
    print_list(tmp_buff, sizeof(tmp_buff) - 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_LEFT),
        			label = gtk_label_new(_("Events")));
#endif
/* enable help for this window */
    ossohelp_dialog_help_enable(GTK_DIALOG(window_config), OMWEATHER_SETTINGS_HELP_ID, app->osso);

    gtk_widget_show_all(window_config);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), app->config->current_settings_page);
    highlight_current_station(GTK_TREE_VIEW(station_list_view));

/* kill popup window :-) */
    if(app->popup_window)
        popup_window_destroy();

    while (0 != (result_gtk_dialog_run = gtk_dialog_run(GTK_DIALOG(window_config)))){
/* start dialog window */
      switch(result_gtk_dialog_run){
	case GTK_RESPONSE_ACCEPT:/* Pressed Button Ok */
/* icon set */	
	    temp_string = gtk_combo_box_get_active_text(GTK_COMBO_BOX(iconset));
	    if(strcmp(app->config->icon_set, temp_string)){
		if(app->config->icon_set)
		    g_free(app->config->icon_set);
	        app->config->icon_set = g_strdup(temp_string);
		memset(path_large_icon, 0, sizeof(path_large_icon));
		sprintf(path_large_icon, "%s%s/", ICONS_PATH, app->config->icon_set);
		flag_update_icon = TRUE;
	    }
	    g_free(temp_string);
/* icon size */	    
	    if(app->config->icons_size != gtk_combo_box_get_active(GTK_COMBO_BOX(icon_size)) + 1){
		app->config->icons_size = gtk_combo_box_get_active(GTK_COMBO_BOX(icon_size)) + 1;
		flag_update_icon = TRUE;
		flag_tuning_warning = TRUE;
	    }
/* Temperature units */
	    if(app->config->temperature_units != gtk_combo_box_get_active(GTK_COMBO_BOX(temperature_unit))){
		app->config->temperature_units = gtk_combo_box_get_active(GTK_COMBO_BOX(temperature_unit));
		flag_update_icon = TRUE;
	    }
/* Font color */
	    if(font_color){
		gtk_color_button_get_color(GTK_COLOR_BUTTON(font_color), &_weather_font_color_temp);
		if(( _weather_font_color_temp.red - app->config->font_color.red ) ||
		    ( _weather_font_color_temp.green - app->config->font_color.green ) ||
		    ( _weather_font_color_temp.blue - app->config->font_color.blue )){
		    memcpy(&(app->config->font_color), &_weather_font_color_temp, sizeof(app->config->font_color));
    		    flag_update_icon = TRUE;
		}
	    }	
/* Background color */
	    if(background_color){
		gtk_color_button_get_color(GTK_COLOR_BUTTON(background_color), &background_color_temp);
		if(( background_color_temp.red - app->config->background_color.red ) ||
		    ( background_color_temp.green - app->config->background_color.green ) ||
		    ( background_color_temp.blue - app->config->background_color.blue )){
		    memcpy(&(app->config->background_color), &background_color_temp, sizeof(app->config->background_color));
    		    flag_update_icon = TRUE;
		}    
	    }
/* Days to show */
	    if( gtk_combo_box_get_active((GtkComboBox*)days_number)!= app->config->days_to_show - 1){
		app->config->previos_days_to_show = app->config->days_to_show;/* store previos number of icons */
		app->config->days_to_show = gtk_combo_box_get_active((GtkComboBox*)days_number);
		app->config->days_to_show++;
    		flag_update_icon = TRUE;
		flag_tuning_warning = TRUE;
	    }
/* Layout Type */
	    if( gtk_combo_box_get_active((GtkComboBox*)layout_type) != app->config->icons_layout ){
		app->config->icons_layout = gtk_combo_box_get_active((GtkComboBox*)layout_type);
    		flag_update_icon = TRUE;
		flag_tuning_warning = TRUE;
	    }
/* Transparency mode */
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_transparency)) != app->config->transparency){
		app->config->transparency = 
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_transparency));
    		flag_update_icon = TRUE;
	    }
/* Split data mode */
    	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(separate_button)) != app->config->separate){
		app->config->separate = 
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(separate_button));
    		flag_update_icon = TRUE;
	    }
/* Swap temperature data mode */
    	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(swap_temperature_button)) != app->config->swap_hi_low_temperature){
		app->config->swap_hi_low_temperature = 
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(swap_temperature_button));
    		flag_update_icon = TRUE;
	    }
/* Hide station name */
    	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_station_name)) != app->config->hide_station_name){
		app->config->hide_station_name = 
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_station_name));
    		flag_update_icon = TRUE;
	    }
/* Hide arrows */
    	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_arrows)) != app->config->hide_arrows){
		app->config->hide_arrows = 
		    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_arrows));
    		flag_update_icon = TRUE;
	    }
/* Distance units */
	    if( gtk_combo_box_get_active((GtkComboBox*)units) != app->config->distance_units ){
		app->config->distance_units = gtk_combo_box_get_active((GtkComboBox*)units);
    		flag_update_icon = TRUE;
	    }
/* Wind units */
	    if( gtk_combo_box_get_active((GtkComboBox*)wunits) != app->config->wind_units ){
		app->config->wind_units = gtk_combo_box_get_active((GtkComboBox*)wunits);
    		flag_update_icon = TRUE;
	    }
/* Switch time */
	    if( gtk_combo_box_get_active((GtkComboBox*)time_2switch_list) != app->config->switch_time / 10){
		app->config->switch_time = 10 * gtk_combo_box_get_active((GtkComboBox*)time_2switch_list);    
		g_source_remove(app->switch_timer);
		if(app->config->switch_time > 0)
		    app->switch_timer = g_timeout_add(app->config->switch_time * 1000,
                    					(GtkFunction)switch_timer_handler,
                        				app->main_window);
    		flag_update_icon = TRUE;
	    }
/* Data valid time */
	    if( (1 << gtk_combo_box_get_active((GtkComboBox*)valid_time_list)) != app->config->data_valid_interval / 3600 ){
		app->config->data_valid_interval = 3600 * (1 << gtk_combo_box_get_active((GtkComboBox*)valid_time_list));
    		flag_update_icon = TRUE;
	    }
/* Downloading after connection */	    
	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_downloading_after_connection)))
		app->config->downloading_after_connecting = TRUE;
	    else
		app->config->downloading_after_connecting = FALSE;
#ifdef HILDON
/* Use GPS station */	    
	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_gps))){
		app->config->gps_station = TRUE;
                add_gps_event(1);
            }
	    else{
		app->config->gps_station = FALSE;
		/* Reset gps station */
		app->gps_station.id0[0] = 0;
		app->gps_station.name[0] = 0;
		app->gps_station.latitude = 0;
		app->gps_station.longtitude = 0;		
	    }	
#endif
/* Current tab number */
	    if(app->config->current_settings_page != gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))){
		app->config->current_settings_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
		flag_update_icon = TRUE;
	    }
/* Update interval
 * saved in update_iterval_changed_handler
*/
    	    new_config_save(app->config);
	    if(flag_update_icon){
    		weather_frame_update(FALSE);
		app->config->previos_days_to_show = app->config->days_to_show;/* store previos number of icons */
	    }	
	    if(flag_update_station){
		if(app->iap_connected){
		    /* check number of elements in user stations list */
		    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                            		    &iter);
		    if(valid && gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
							    &iter)){
			app->show_update_window = TRUE;
			update_weather();
		    }
		} 
		else    
    		    weather_frame_update(TRUE);
	    }
	    /* clear regions list */
	    if(app->regions_list)
		gtk_list_store_clear(app->regions_list);
	    /* clear locations list */
	    if(app->stations_list)
		gtk_list_store_clear(app->stations_list);    
	break;
	case OMWEATHER_RESPONSE_ABOUT:/* Pressed About Button */
	    create_about_dialog();
	break;
	default:/* Pressed CANCEL */
	    if( flag_update_station && app->iap_connected ){
		/* check number of elements in user stations list */
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                            		    &iter);
		if(valid && gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
							    &iter)){
		    app->show_update_window = TRUE;
		    update_weather();
		}
	    }
	break;
      }
	if(result_gtk_dialog_run !=  OMWEATHER_RESPONSE_ABOUT) 
    	    break; /* We are leave a cycle WHILE */
    }
    not_event = FALSE;
#ifndef HILDON
    if(flag_tuning_warning)
	hildon_banner_show_information(app->main_window,
					NULL,
					_("Use Edit layout \nfor tuning images of applet"));
#endif					
    gtk_widget_destroy(window_config);
}
/*******************************************************************************/
/* get icon set names */
int create_icon_set_list(GtkWidget *store){
    Dirent	*dp;
    DIR		*dir_fd;
    gint	i = 0;
    char 	*temp_string = NULL;
    int		sets_number = 0;
    
    dir_fd	= opendir(ICONS_PATH);
    if(dir_fd){
	while( (dp = readdir(dir_fd)) ){
	    if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
		continue;
	    if(dp->d_type == DT_DIR){
		gtk_combo_box_append_text(GTK_COMBO_BOX(store), dp->d_name);
		sets_number++;
		if(!strcmp(app->config->icon_set, dp->d_name))
		    gtk_combo_box_set_active(GTK_COMBO_BOX(store), i);
		i++;
	    }
	}
	closedir(dir_fd);
	/* check if selected icon set not found */
	temp_string = gtk_combo_box_get_active_text(GTK_COMBO_BOX(store));
	if(!temp_string)
	    gtk_combo_box_set_active(GTK_COMBO_BOX(store), 0);
	else 
	    g_free(temp_string);
    }
    else{
    	gtk_combo_box_append_text(GTK_COMBO_BOX(store), app->config->icon_set);
	gtk_combo_box_set_active(GTK_COMBO_BOX(store), 0);
    }
    return sets_number;
}
/*******************************************************************************/
void create_about_dialog(void){
    GtkWidget	*help_dialog,
		*notebook,
		*title;
    char	tmp_buff[2048];
    gint	result;

    help_dialog = gtk_dialog_new_with_buttons(_("Other Maemo Weather Info"),
        				NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        				_("OK"), GTK_RESPONSE_ACCEPT,
					NULL);
/* Create Notebook widget */
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(help_dialog)->vbox),
        	    notebook = gtk_notebook_new(), TRUE, TRUE, 0);
/* About tab */
    snprintf(tmp_buff, sizeof(tmp_buff),
#ifdef DISPLAY_BUILD
	    "%s%s%s%s%s%s",
#else
	    "%s%s%s%s",
#endif
	    _("\nHildon desktop applet\n"
	    "for Nokia 770/N800/N810\n"
	    "to show weather forecasts.\n"
	    "Version "), VERSION, 
#ifdef DISPLAY_BUILD
    " Build: ", BUILD,
#endif	    
	    _("\nCopyright(c) 2006-2008\n"
	    "Vlad Vasiliev, Pavel Fialko"),
    	    _("\nCopyright(c) 2008\n"
	    "for default icon set (Glance)\n"
	    "Andrew Zhilin")
	    );

	    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_CENTER),
				title = gtk_label_new(_("About")));
/* Authors tab */
    snprintf(tmp_buff, sizeof(tmp_buff), "%s",
		_("\nAuthor and maintenance:\n"
		"\tVlad Vasiliev, vlad@gas.by\n"
		"Maintenance:\n\tPavel Fialko, pavelnf@gmail.com\n"
		"Documentation:\n\tMarko Vertainen\n"
		"Design of default iconset:\n\tAndrew Zhilin"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_LEFT),
				title = gtk_label_new(_("Authors")));
/* Thanks tab */
    snprintf(tmp_buff, sizeof(tmp_buff), "%s",
	    _("\nEd Bartosh - for more feature requests,\n"
	    "\t\t\t\tsupport and criticism\n"
	    "Eugen Kaluta aka tren - for feature requests\n"
	    "\t\t\t\tand support\n"
	    "Maxim Kalinkevish aka spark for testing\n"
	    "Yuri Komyakov - for Nokia 770 device \n"
	    "Greg Thompson for support stations.txt file\n"
	    "Frank Persian - for idea of new layout\n"
	    "Brian Knight - for idea of iconset, criticism \n"
	    "\t\t\t\tand donation ;-)\n"));
    strcat(tmp_buff,	    
	    _("Andrew aka Tabster - for testing and ideas\n"
	    "Brad Jones aka kazrak - for testing\n"
	    "Alexis Iglauer - for testing\n"
	    "Eugene Roytenberg - for testing\n"
	    "Jarek Szczepanski aka Imrahil - for testing\n"
	    "Vladimir Shakhov aka Mendoza - for testing \n"
	    "Marc Dilon - for spell/stylecheck text of English\n"));
    strcat(tmp_buff,
	    _("Arkady Glazov aka Globster - for testing\n"
	      "Alexander Savchenko aka dizel - for testing\n"));
    strcat(tmp_buff,
	    _("Eric Link - for feature request and donation\n"));
              
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_LEFT),
        			title = gtk_label_new(_("Thanks")));
/* Translators tab */
    snprintf(tmp_buff, sizeof(tmp_buff), "%s",
	    _("French - Nicolas Granziano\n"
	      "Russian - Pavel Fialko, Vlad Vasiliev,\n\t    Ed Bartosh\n"
	      "Finnish - Marko Vertainen\n"
	      "German - Claudius Henrichs\n"
	      "Italian - Pavel Fialko\n"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_LEFT),
        			title = gtk_label_new(_("Translators")));
/* enable help for this window */
    ossohelp_dialog_help_enable(GTK_DIALOG(help_dialog), OMWEATHER_ABOUT_HELP_ID, app->osso);
    gtk_widget_show_all(help_dialog);
/* start dialog window */
    result = gtk_dialog_run(GTK_DIALOG(help_dialog));
    gtk_widget_destroy(help_dialog);
}
/*******************************************************************************/
GtkWidget* create_scrolled_window_with_text(const char* text,
				GtkJustification justification){

    GtkWidget	*text_view,
		*scrolled_window;
    GtkTextBuffer	*text_buffer;

    text_view = gtk_text_view_new();
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(text_buffer), text, -1);
    /* set params of text view */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_justification(GTK_TEXT_VIEW(text_view),
				    justification);
    gtk_text_view_set_overwrite(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_NONE);
    /* scrolled window */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
					GTK_SHADOW_OUT);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 550, 200);
    /* pack childs to the scrolled window */
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
    return scrolled_window;
}
/*******************************************************************************/
void station_list_view_select_handler(GtkTreeView *tree_view,
                                    			    gpointer user_data){
    GtkTreeIter		iter;
    gchar		*station_selected = NULL,
			*station_name = NULL,
			*station_code = NULL;
    gboolean		valid;
    GtkTreeSelection	*selected_line;
    GtkTreeModel	*model;


    selected_line = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    if( !gtk_tree_selection_get_selected(selected_line, NULL, &iter) )
        return;
    gtk_tree_model_get(model, &iter, 0, &station_selected, -1);

    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
    while(valid){
        gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                            &iter,
                            0, &station_name,
                            1, &station_code,
                            -1);
        if(!strcmp(station_selected, station_name)){
        /* update current station code */
            if(app->config->current_station_id)
                g_free(app->config->current_station_id);
            app->config->current_station_id = station_code;
            /* update current station name */
            if(app->config->current_station_name)
                g_free(app->config->current_station_name);
            app->config->current_station_name = station_name;
	    /* add selected station name to the rename entry */
	    gtk_entry_set_text(GTK_ENTRY(user_data), station_name);
            break;
        }
	else{
	    g_free(station_name);
	    g_free(station_code);
	}
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                    	    &iter);
    }
    g_free(station_selected);
    weather_frame_update(TRUE);
    new_config_save(app->config);
}
/*******************************************************************************/
void update_iterval_changed_handler(GtkComboBox *widget, gpointer user_data){
    time_t		update_time = 0;
    GtkLabel		*label;
    GtkTreeModel	*model;
    GtkTreeIter		iter;
    gchar		*temp_string,
			tmp_buff[100];

    label = GTK_LABEL(user_data);

    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	gtk_tree_model_get(model, &iter, 0, &temp_string,
					 1, &update_time,
    					    -1);
	g_free(temp_string);
	if(app->config->update_interval != update_time){
	    app->config->update_interval = update_time;
	    remove_periodic_event();
	    add_periodic_event(time(NULL));
	}
	/* fill next update field */
	update_time = next_update();
	if(!update_time)
	    temp_string = _("Never");
	else{
	    tmp_buff[0] = 0;
	    strftime(tmp_buff, sizeof(tmp_buff) - 1, "%X %x",
	    		    localtime(&update_time));
	    temp_string = tmp_buff;
	}
	gtk_label_set_text(label, temp_string);
    }
}
/*******************************************************************************/
int get_active_item_index(GtkTreeModel *list, int time, const gchar *text,
						gboolean use_index_as_result){

    int		result = 0,
		index = 0;
    gboolean	valid = FALSE;
    GtkTreeIter	iter;
    gchar	*str_data = NULL;
    gint	int_data;

    valid = gtk_tree_model_get_iter_first((GtkTreeModel*)list, &iter);
    while(valid){
	gtk_tree_model_get(list, &iter, 
                    	    0, &str_data,
                    	    1, &int_data,
			    -1);
	if(text){ /* if parameter is string */
	    if(!strcmp((char*)text, str_data)){
		if(use_index_as_result)
		    result = index;
		else
		    result = int_data;
		break;
	    }
	}
	else{/* if parameter is int */
	    if(time == int_data){
		result = index;
		break;
	    }
	}
	g_free(str_data);
	str_data = NULL;
	index++;
	valid = gtk_tree_model_iter_next(list, &iter);
    }
    if(str_data)
	g_free(str_data);
    return result;
}
/*******************************************************************************/
void transparency_button_toggled_handler(GtkToggleButton *togglebutton,
                                        		    gpointer user_data){
    GtkWidget	*background_color;
    
    background_color = GTK_WIDGET(user_data);
    
    if(gtk_toggle_button_get_active(togglebutton))
	gtk_widget_set_sensitive(background_color, FALSE);
    else
	gtk_widget_set_sensitive(background_color, TRUE);
}
/*******************************************************************************/
gboolean check_station_code(const gchar *station_code){
    if(strlen((char*)station_code) < 5)
	return TRUE;
    return FALSE;
}
/*******************************************************************************/
void up_key_handler(GtkButton *button, gpointer list){
    GtkTreeView		*stations = (GtkTreeView*)list;
    GtkTreeIter		iter,
			prev_iter;
    GtkTreeSelection	*selected_line;
    GtkTreeModel	*model;
    GtkTreePath		*path;

    selected_line = gtk_tree_view_get_selection(GTK_TREE_VIEW(stations));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(stations));
    if( !gtk_tree_selection_get_selected(selected_line, NULL, &iter) )
        return;
    path = gtk_tree_model_get_path(model, &iter);
    if(!gtk_tree_path_prev(path)){
	gtk_tree_path_free(path);
	return;
    }
    else{
	if(gtk_tree_model_get_iter(model, &prev_iter, path))
	    gtk_list_store_move_before(GTK_LIST_STORE(model), &iter, &prev_iter);
    }
    gtk_tree_path_free(path);
}
/*******************************************************************************/
void down_key_handler(GtkButton *button, gpointer list){
    GtkTreeView		*stations = (GtkTreeView*)list;
    GtkTreeIter		iter,
			next_iter;
    GtkTreeSelection	*selected_line;
    GtkTreeModel	*model;
    GtkTreePath		*path;

    selected_line = gtk_tree_view_get_selection(GTK_TREE_VIEW(stations));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(stations));
    if( !gtk_tree_selection_get_selected(selected_line, NULL, &iter) )
        return;
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_path_next(path);
    if(gtk_tree_model_get_iter(model, &next_iter, path))
	gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, &next_iter);
    gtk_tree_path_free(path);
}
/*******************************************************************************/
void highlight_current_station(GtkTreeView *tree_view){
    GtkTreeIter		iter;
    gchar		*station_name = NULL,
			*station_code = NULL;
    gboolean		valid;
    GtkTreePath		*path;
    GtkTreeModel	*model;
    
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
    while(valid){
        gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                            &iter,
                            0, &station_name,
                            1, &station_code,
                            -1);
        if(app->config->current_station_name && station_name &&                    
            !strcmp(app->config->current_station_name, station_name)){
	    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	    path = gtk_tree_model_get_path(model, &iter);
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree_view), path, NULL, FALSE);
	    gtk_tree_path_free(path);
            break;
        }
	else{
	    g_free(station_name);
	    g_free(station_code);
	}
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                    	    &iter);
    }
}
/*******************************************************************************/
void weather_window_settings_021(GtkWidget *widget, GdkEvent *event,
							    gpointer user_data){

    GtkWidget	*window_config = NULL,
		*notebook = NULL,
		*label = NULL,
		*vbox = NULL,
		*buttons_box = NULL,
		*about_button = NULL,
		*apply_button = NULL,
		*close_button = NULL,
		*back_button = NULL,
		*left_vbox = NULL,
		*right_vbox = NULL,
		*stations_list_with_buttons_hbox = NULL,
		*up_down_delete_buttons_vbox = NULL,
		*rename_entry = NULL,
		*station_name = NULL,
		*station_code = NULL,
		*add_station_button = NULL,
		*add_station_button1 = NULL,
		*add_station_button2 = NULL,
		*name_code_add_button_hbox = NULL,
		*label_and_station_name_hbox = NULL,
		*label_and_station_code_hbox = NULL,
		*labels_name_code_vbox = NULL,
		*countries_regions_stations_add_hbox = NULL,
		*countries_regions_stations_vbox = NULL,
		*stations_and_add_button_hbox = NULL,
		*countries_hbox = NULL,
		*states_hbox = NULL,
		*interface_vbox = NULL,
		*visible_items_label_number_hbox = NULL,
		*visible_items_number = NULL,
		*layout_label_list_hbox = NULL,
		*layout_type = NULL,
		*icon_set_label_list_hbox = NULL,
		*icon_set = NULL,
		*icon_size_label_number_hbox = NULL,
		*icon_size = NULL,
		*hide_station_name = NULL,
		*hide_arrows = NULL,
		*hide_station_name_label_button_hbox = NULL,
		*hide_arrows_label_button_hbox = NULL,
		*transparency = NULL,
		*transparency_label_button_hbox = NULL,
#ifdef HILDON
                *label_gps = NULL,
                *hbox_gps = NULL,
                *chk_gps = NULL,
#endif
		*time_update_label = NULL,
		*table = NULL,
		*font_color = NULL,
		*font_color_label_button_hbox = NULL,
		*background_color = NULL,
		*chk_transparency = NULL,
		*chk_downloading_after_connection = NULL,
		*separate = NULL,
		*separate_label_button_hbox = NULL,
		*swap_temperature_button = NULL,
		*scrolled_window = NULL,
		*button_add = NULL,
		*button_ren = NULL,
		*up_icon = NULL,
		*down_icon = NULL,
		*delete_icon = NULL,
		*add_icon = NULL,
		*add_icon1 = NULL,
		*add_icon2 = NULL,
		*up_station_button = NULL,
		*down_station_button = NULL,
		*delete_station_button = NULL;
    GtkIconInfo *gtkicon_arrow;
    gboolean	valid = FALSE;
    GtkTreeIter	iter;
    char	flag_update_icon = '\0'; /* Flag update main weather icon of desktop */
#ifndef RELEASE
    char	tmp_buff[1024];
#endif
    gboolean	flag_tuning_warning; /* Flag for show the warnings about tuning images of applet */
    GdkColor	_weather_font_color_temp, /* Temporary for font color */
		background_color_temp;
    static char *temp_string; /* Temporary for the results differnet strdup functions */
    static int result_gtk_dialog_run; /* Temporary for the gtk_dialog_run result */

    not_event = TRUE;
    flag_update_station = FALSE;
    flag_update_icon = FALSE;
    flag_tuning_warning = FALSE; 

    if(!app->dbus_is_initialize)
	weather_initialize_dbus();

/* Main window */
    window_config = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GLADE_HOOKUP_OBJECT_NO_REF(window_config, window_config, "window_config");
    gtk_window_set_title(GTK_WINDOW(window_config),
			    _("Other Maemo Weather Settings"));
    gtk_window_set_modal(GTK_WINDOW(window_config), TRUE);
    gtk_window_fullscreen(GTK_WINDOW(window_config));
    /* create frame vbox */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window_config), vbox);
/* create tabs widget */
    notebook = gtk_notebook_new();
    GLADE_HOOKUP_OBJECT(window_config, notebook, "notebook");
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
/* Locations tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        	    table = gtk_table_new(2, 2, FALSE),
        	    label = gtk_label_new(_("Stations")));
    /* Stations hbox */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			gtk_label_new(_("Arrange")),
        			0, 1, 0, 1);
    /* left side vbox */
    gtk_table_attach_defaults(GTK_TABLE(table),
				left_vbox = gtk_vbox_new(FALSE, 0),
        			0, 1, 1, 2);
    stations_list_with_buttons_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_vbox), stations_list_with_buttons_hbox,
			TRUE, TRUE, 0);
    /* Stations list */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),
					GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 280, 300);

    station_list_view = create_tree_view(app->user_stations_list);
    gtk_container_add(GTK_CONTAINER(scrolled_window),
                	GTK_WIDGET(station_list_view));
    gtk_box_pack_start(GTK_BOX(stations_list_with_buttons_hbox), scrolled_window,
                        TRUE, TRUE, 0);
/* Up Station and Down Station and Delete Station Buttons */
    up_down_delete_buttons_vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(stations_list_with_buttons_hbox),
			up_down_delete_buttons_vbox,
                        FALSE, FALSE, 0);
/* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_arrow_up", 16, 0);
    up_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare up_station_button */    
    up_station_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(up_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(up_station_button), up_icon);
    gtk_widget_set_events(up_station_button, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(up_station_button), FALSE);
    g_signal_connect(up_station_button, "clicked",
			G_CALLBACK(up_key_handler), (gpointer)station_list_view);
/* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_arrow_down", 16, 0);
    down_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare down_station_button */    
    down_station_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(down_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(down_station_button), down_icon);
    gtk_widget_set_events(down_station_button, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(down_station_button), FALSE);
    g_signal_connect(down_station_button, "clicked",
		    	G_CALLBACK(down_key_handler), (gpointer)station_list_view);
/* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_list_gene_invalid", 16, 0);
    delete_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare delete_station_button */    
    delete_station_button = gtk_button_new();
    gtk_button_set_focus_on_click(GTK_BUTTON(delete_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(delete_station_button), delete_icon);
    gtk_widget_set_events(delete_station_button, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(delete_station_button), FALSE);
    g_signal_connect(delete_station_button, "clicked",
                	G_CALLBACK(weather_delete_station), NULL);
/* Pack Up, Down and Delete buttons */
    gtk_box_pack_start(GTK_BOX(up_down_delete_buttons_vbox), up_station_button,
                        TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(up_down_delete_buttons_vbox), delete_station_button,
                        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(up_down_delete_buttons_vbox), down_station_button,
                        TRUE, TRUE, 0);
/* Rename station entry */
    rename_entry = gtk_entry_new();
    GLADE_HOOKUP_OBJECT(window_config, rename_entry, "rename_entry");
    gtk_box_pack_start(GTK_BOX(left_vbox), rename_entry,
			TRUE, TRUE, 0);
    /* right side vbox */
    gtk_table_attach_defaults(GTK_TABLE(table),
        			gtk_label_new(_("Add")),
        			1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table),
				right_vbox = gtk_vbox_new(FALSE, 0),
        			1, 2, 1, 2);
    /* hbox for fields and add button */
    gtk_box_pack_start(GTK_BOX(right_vbox),
			label_and_station_name_hbox = gtk_hbox_new(FALSE, 0),
                        FALSE, TRUE, 5);
    /* label By name */
    gtk_box_pack_start(GTK_BOX(label_and_station_name_hbox),
			gtk_label_new(_("Name:       ")),
                        TRUE, TRUE, 0);
    /* entry for station name */
    gtk_box_pack_start(GTK_BOX(label_and_station_name_hbox),
			station_name = gtk_entry_new(),
                        TRUE, TRUE, 0);
    GLADE_HOOKUP_OBJECT(window_config, station_name, "station_name_entry");
    /* add station button */
    /* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_gene_plus", 16, 0);
    add_icon = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare add_station_button */    
    add_station_button = gtk_button_new();
    gtk_widget_set_name(add_station_button, "add_name");
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button), FALSE);
    gtk_container_add(GTK_CONTAINER(add_station_button), add_icon);
    gtk_widget_set_events(add_station_button, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button), FALSE);
    g_signal_connect(add_station_button, "clicked",
			G_CALLBACK(add_button_handler), (gpointer)window_config);
    gtk_box_pack_start(GTK_BOX(label_and_station_name_hbox),
			add_station_button,
			FALSE, FALSE, 5);
    /* hbox for station code and label */
    gtk_box_pack_start(GTK_BOX(right_vbox),
			label_and_station_code_hbox = gtk_hbox_new(FALSE, 0),
                        FALSE, TRUE, 5);
    /* label By (Zip) code */
    gtk_box_pack_start(GTK_BOX(label_and_station_code_hbox),
			gtk_label_new(_("Code (Zip):")),
                        TRUE, TRUE, 0);
    /* entry for station name */
    gtk_box_pack_start(GTK_BOX(label_and_station_code_hbox),
			station_code = gtk_entry_new(),
                        TRUE, TRUE, 0);
    GLADE_HOOKUP_OBJECT(window_config, station_code, "station_code_entry");
    /* add button */
    /* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_gene_plus", 16, 0);
    add_icon1 = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare add_station_button */    
    add_station_button1 = gtk_button_new();
    gtk_widget_set_name(add_station_button1, "add_code");
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button1), FALSE);
    gtk_container_add(GTK_CONTAINER(add_station_button1), add_icon1);
    gtk_widget_set_events(add_station_button1, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button1), FALSE);
    g_signal_connect(add_station_button1, "clicked",
			G_CALLBACK(add_button_handler), (gpointer)window_config);
    gtk_box_pack_start(GTK_BOX(label_and_station_code_hbox),
			add_station_button1,
			FALSE, FALSE, 5);    
    /* Label */
    gtk_box_pack_start(GTK_BOX(right_vbox),
			gtk_label_new(_("From the list:")),
                        FALSE, TRUE, 20);
    /* countries list  */
    countries = gtk_combo_box_new_text();
    gtk_combo_box_set_row_span_column((GtkComboBox*)countries, 0);
    gtk_combo_box_set_model((GtkComboBox*)countries,
			    (GtkTreeModel*)app->countrys_list);
    gtk_box_pack_start(GTK_BOX(right_vbox),
			countries_hbox = gtk_hbox_new(FALSE, 0),
			FALSE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(countries_hbox),
			countries, TRUE, TRUE, 10);
    /* states list */
    states = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(right_vbox),
			states_hbox = gtk_hbox_new(FALSE, 0),
			FALSE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(states_hbox),
			states, TRUE, TRUE, 10);
    /* stations list */
    stations = gtk_combo_box_new_text();
    GLADE_HOOKUP_OBJECT(window_config, stations, "stations");
    /* add button */
    /* prepare icon */
    gtkicon_arrow = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
        	                        	"qgn_indi_gene_plus", 16, 0);
    add_icon2 = gtk_image_new_from_file(gtk_icon_info_get_filename(gtkicon_arrow));
    gtk_icon_info_free(gtkicon_arrow);
/* prepare add_station_button */    
    add_station_button2 = gtk_button_new();
    gtk_widget_set_name(add_station_button2, "add_from_list");
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button2), FALSE);
    gtk_container_add(GTK_CONTAINER(add_station_button2), add_icon2);
    gtk_widget_set_events(add_station_button2, GDK_BUTTON_PRESS_MASK);
    gtk_button_set_focus_on_click(GTK_BUTTON(add_station_button2), FALSE);
    g_signal_connect(add_station_button2, "clicked",
			G_CALLBACK(add_button_handler), (gpointer)window_config);
    gtk_box_pack_start(GTK_BOX(right_vbox),
			stations_and_add_button_hbox = gtk_hbox_new(FALSE, 0),
			FALSE, TRUE, 10);
    /* pack items to the box */
    gtk_box_pack_start(GTK_BOX(stations_and_add_button_hbox),
			stations, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(stations_and_add_button_hbox),
			add_station_button2,
			FALSE, FALSE, 10);

/* Set default value to country combo_box */
    gtk_combo_box_set_active(GTK_COMBO_BOX(countries),
				get_active_item_index((GtkTreeModel*)app->countrys_list,
				-1, app->config->current_country, TRUE));
    changed_country();
    changed_state();
    g_signal_connect((gpointer)countries, "changed",
            		G_CALLBACK(changed_country), NULL);
    g_signal_connect((gpointer)states, "changed",
                	G_CALLBACK(changed_state), NULL);
    g_signal_connect((gpointer) stations, "changed",
            		G_CALLBACK(changed_stations), NULL);

    g_signal_connect(station_list_view, "cursor-changed",
                        G_CALLBACK(station_list_view_select_handler),
			rename_entry);
/* Interface tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			interface_vbox = gtk_vbox_new(FALSE, 0),
        			label = gtk_label_new(_("Interface")));
    /* Visible items */
    visible_items_label_number_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			visible_items_label_number_hbox, TRUE, TRUE, 0);
    /* Visible items label */
    gtk_box_pack_start(GTK_BOX(visible_items_label_number_hbox),
			gtk_label_new(_("Visible items")), TRUE, TRUE, 0);
    /* Visible items number */
    visible_items_number = hildon_controlbar_new();
    GLADE_HOOKUP_OBJECT(window_config, visible_items_number, "visible_items_number");
    hildon_controlbar_set_min(visible_items_number, 1);
    hildon_controlbar_set_max(visible_items_number, Max_count_weather_day);
    hildon_controlbar_set_value(visible_items_number, app->config->days_to_show);
    gtk_box_pack_start(GTK_BOX(visible_items_label_number_hbox),
			visible_items_number, TRUE, TRUE, 5);
    /* Layout */
    layout_label_list_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			layout_label_list_hbox, TRUE, TRUE, 0);
    /* Layout label */
    gtk_box_pack_start(GTK_BOX(layout_label_list_hbox),
			gtk_label_new(_("Layout")), TRUE, TRUE, 0);
    /* Layout type */
    layout_type = gtk_combo_box_new_text();
    GLADE_HOOKUP_OBJECT(window_config, layout_type, "layout_type");
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("One row"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("One column"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Two rows"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Two columns"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(layout_type), _("Combination"));    
    switch(app->config->icons_layout){
	default:
	case ONE_ROW: 	  gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 0);break;
	case ONE_COLUMN:  gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 1);break;
	case TWO_ROWS:    gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 2);break;
	case TWO_COLUMNS: gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 3);break;
	case COMBINATION: gtk_combo_box_set_active(GTK_COMBO_BOX(layout_type), 4);break;	
    }
    gtk_box_pack_start(GTK_BOX(layout_label_list_hbox),
			layout_type, TRUE, TRUE, 5);
    /* Icon set */
    icon_set_label_list_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			icon_set_label_list_hbox, TRUE, TRUE, 0);
    /* Layout label */
    gtk_box_pack_start(GTK_BOX(icon_set_label_list_hbox),
			gtk_label_new(_("Icon set")), TRUE, TRUE, 0);
    /* Icon set list */
    icon_set = gtk_combo_box_new_text();
    GLADE_HOOKUP_OBJECT(window_config, icon_set, "icon_set");
/* add icons set to list */
    if(create_icon_set_list(icon_set) < 2)
	gtk_widget_set_sensitive(icon_set, FALSE);
    else
	gtk_widget_set_sensitive(icon_set, TRUE);
    gtk_box_pack_start(GTK_BOX(icon_set_label_list_hbox),
			icon_set, TRUE, TRUE, 5);
    /* Icon size */
    icon_size_label_number_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			icon_size_label_number_hbox, TRUE, TRUE, 0);
    /* Icon size label */
    gtk_box_pack_start(GTK_BOX(icon_size_label_number_hbox),
			gtk_label_new(_("Icon size")), TRUE, TRUE, 0);
    /* Icon size number */
    icon_size = hildon_controlbar_new();
    GLADE_HOOKUP_OBJECT(window_config, icon_size, "icon_size");
    hildon_controlbar_set_min(icon_size, 1);
    hildon_controlbar_set_max(icon_size, 5);
    switch(app->config->icons_size){
	case TINY: hildon_controlbar_set_value(icon_size, TINY); break;
	case SMALL: hildon_controlbar_set_value(icon_size, SMALL); break;
	case MEDIUM: hildon_controlbar_set_value(icon_size, MEDIUM); break;
	default:
	case LARGE: hildon_controlbar_set_value(icon_size, LARGE); break;
	case GIANT: hildon_controlbar_set_value(icon_size, GIANT); break;
    }
    gtk_box_pack_start(GTK_BOX(icon_size_label_number_hbox),
			icon_size, TRUE, TRUE, 5);
    /* Separate weather */
    separate_label_button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			separate_label_button_hbox, TRUE, TRUE, 0);
    /* Separate label */
    gtk_box_pack_start(GTK_BOX(separate_label_button_hbox),
			gtk_label_new(_("Show only current weather on first icon")),
			TRUE, TRUE, 0);
    /* Separate button */
    separate = gtk_check_button_new();
    GLADE_HOOKUP_OBJECT(window_config, separate, "separate");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(separate),
        			    app->config->separate);
    gtk_box_pack_start(GTK_BOX(separate_label_button_hbox),
			separate,
			TRUE, TRUE, 5);
    /* Hide station name */
    hide_station_name_label_button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			hide_station_name_label_button_hbox, TRUE, TRUE, 0);
    /* Hide station name label */
    gtk_box_pack_start(GTK_BOX(hide_station_name_label_button_hbox),
			gtk_label_new(_("Hide station name")),
			TRUE, TRUE, 0);
    /* Hide station name button */
    hide_station_name = gtk_check_button_new();
    GLADE_HOOKUP_OBJECT(window_config, hide_station_name, "hide_station_name");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_station_name),
        			    app->config->hide_station_name);
    gtk_box_pack_start(GTK_BOX(hide_station_name_label_button_hbox),
			hide_station_name,
			TRUE, TRUE, 5);
    /* Hide arrows */
    hide_arrows_label_button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			hide_arrows_label_button_hbox, TRUE, TRUE, 0);
    /* Hide arrows label */
    gtk_box_pack_start(GTK_BOX(hide_arrows_label_button_hbox),
			gtk_label_new(_("Hide arrows")),
			TRUE, TRUE, 0);
    /* Hide arrows button */
    hide_arrows = gtk_check_button_new();
    GLADE_HOOKUP_OBJECT(window_config, hide_arrows, "hide_arrows");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_arrows),
        			    app->config->hide_arrows);
    gtk_box_pack_start(GTK_BOX(hide_arrows_label_button_hbox),
			hide_arrows,
			TRUE, TRUE, 5);
    /* Transparency */
    transparency_label_button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			transparency_label_button_hbox, TRUE, TRUE, 0);
    /* Transparency label */
    gtk_box_pack_start(GTK_BOX(transparency_label_button_hbox),
			gtk_label_new(_("Transparency")),
			TRUE, TRUE, 0);
    /* Transparency button */
    transparency = gtk_check_button_new();
    GLADE_HOOKUP_OBJECT(window_config, transparency, "transparency");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(transparency),
        			    app->config->transparency);
    gtk_box_pack_start(GTK_BOX(transparency_label_button_hbox),
			transparency,
			FALSE, FALSE, 5);
    /* Background color */
    /* Background label */
    gtk_box_pack_start(GTK_BOX(transparency_label_button_hbox),
			gtk_label_new(_("Background color")),
			TRUE, TRUE, 0);
    /* Background color button */
    background_color = gtk_color_button_new();
    GLADE_HOOKUP_OBJECT(window_config, background_color, "background_color");
    g_signal_connect(GTK_TOGGLE_BUTTON(transparency), "toggled",
            		    G_CALLBACK(transparency_button_toggled_handler), background_color);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(background_color), &(app->config->background_color));
    if((background_color) && app->config->transparency)
        gtk_widget_set_sensitive(background_color, FALSE);	
    else
        gtk_widget_set_sensitive(background_color, TRUE);
    gtk_box_pack_start(GTK_BOX(transparency_label_button_hbox),
			background_color,
			FALSE, FALSE, 5);
    /* Font color */
    font_color_label_button_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(interface_vbox),
			font_color_label_button_hbox, TRUE, TRUE, 0);
    /* Font color label */
    gtk_box_pack_start(GTK_BOX(font_color_label_button_hbox),
			gtk_label_new(_("Font color")),
			TRUE, TRUE, 0);
    /* Font color button */
    font_color = gtk_color_button_new();
    GLADE_HOOKUP_OBJECT(window_config, font_color, "font_color");
    gtk_color_button_set_color(GTK_COLOR_BUTTON(font_color), &(app->config->font_color));
    gtk_box_pack_start(GTK_BOX(font_color_label_button_hbox),
			font_color,
			FALSE, FALSE, 5);
/* Units tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			table = gtk_table_new(1, 3, FALSE),
        			label = gtk_label_new(_("Units")));

/* Update tab */
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			table = gtk_table_new(5, 2, FALSE),
        			label = gtk_label_new(_("Update")));

#ifndef RELEASE
/* Events list tab */
    memset(tmp_buff, 0, sizeof(tmp_buff));
    print_list(tmp_buff, sizeof(tmp_buff) - 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        			create_scrolled_window_with_text(tmp_buff,
						    GTK_JUSTIFY_LEFT),
        			label = gtk_label_new("Events"));
#endif
/* Bottom buttons box */
    buttons_box = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_size_request(buttons_box, -1, 60);
    /* Back buton */
    back_button = gtk_button_new_with_label(_("Back"));
    gtk_button_set_relief(GTK_BUTTON(back_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click(GTK_BUTTON(back_button), FALSE);
    g_signal_connect(back_button, "clicked",
                        G_CALLBACK(back_button_handler),
			(gpointer)window_config);
    /* About buton */
    about_button = gtk_button_new_with_label(_("About"));
    gtk_button_set_relief(GTK_BUTTON(about_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click(GTK_BUTTON(about_button), FALSE);
    g_signal_connect(about_button, "clicked",
                        G_CALLBACK(about_button_handler), NULL);
    /* Apply button */
    apply_button = gtk_button_new_with_label(_("Apply"));
    gtk_button_set_relief(GTK_BUTTON(apply_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click(GTK_BUTTON(apply_button), FALSE);
    g_signal_connect(GTK_BUTTON(apply_button), "clicked",
                        G_CALLBACK(apply_button_handler),
			(gpointer)window_config);
    /* Close button */
    close_button = gtk_button_new_with_label(_("Close"));
    gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click(GTK_BUTTON(close_button), FALSE);
    g_signal_connect(GTK_BUTTON(close_button), "clicked",
                        G_CALLBACK(close_button_handler), (gpointer)window_config);
/* Pack buttons to the buttons box */
    gtk_box_pack_start(GTK_BOX(buttons_box), back_button, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(buttons_box), apply_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttons_box), about_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttons_box), close_button, FALSE, FALSE, 10);
/* Pack items to config window */
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), buttons_box, FALSE, FALSE, 0);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
				    app->config->current_settings_page);
    gtk_widget_show_all(window_config);
/* Highlight current station */
    highlight_current_station(GTK_TREE_VIEW(station_list_view));
    gtk_entry_set_text(GTK_ENTRY(rename_entry),
			app->config->current_station_name);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),
			app->config->current_settings_page);
/* kill popup window :-) */
    if(app->popup_window)
        popup_window_destroy();
}
/*******************************************************************************/
void apply_button_handler(GtkButton *button, gpointer user_data){
    GtkWidget		*config_window = GTK_WIDGET(user_data),
		        *rename_entry = NULL,
			*visible_items_number = NULL,
			*layout_type = NULL,
			*icon_set = NULL,
			*icon_size = NULL,
			*separate = NULL,
			*hide_station_name = NULL,
			*hide_arrows = NULL,
			*transparency = NULL,
			*background_color = NULL,
			*font_color = NULL;
    gboolean		valid = FALSE;
    GtkTreeIter		iter;
    gchar		*new_station_name = NULL,
			*station_name = NULL,
			*temp_string = NULL;

/* check where the station name is changed */
    rename_entry = lookup_widget(config_window, "rename_entry");
    if(rename_entry){
	new_station_name = (gchar*)gtk_entry_get_text(GTK_ENTRY(rename_entry));
	if(strcmp(app->config->current_station_name, new_station_name)){
    	    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(app->user_stations_list),
                                                  &iter);
	    while(valid){
    		gtk_tree_model_get(GTK_TREE_MODEL(app->user_stations_list),
                        	    &iter, 0, &station_name, -1);
    		if(!strcmp(app->config->current_station_name, station_name)){
		    /* update current station name */
		    g_free(station_name);
		    gtk_list_store_remove(app->user_stations_list, &iter);
                    add_station_to_user_list(g_strdup(new_station_name),
					    app->config->current_station_id, FALSE);
		    if(app->config->current_station_name)
			g_free(app->config->current_station_name);
		    app->config->current_station_name = g_strdup(new_station_name);
        	    break;
		}
		else
		    g_free(station_name);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(app->user_stations_list),
                                    	    &iter);
	    }
	}
    }
/* visible items */
    visible_items_number = lookup_widget(config_window, "visible_items_number");
    if(visible_items_number){
	app->config->days_to_show = hildon_controlbar_get_value(visible_items_number);
	(!app->config->days_to_show) && (app->config->days_to_show = 1);
    }
/* layout type */
    layout_type = lookup_widget(config_window, "layout_type");
    if(layout_type)
	app->config->icons_layout = gtk_combo_box_get_active((GtkComboBox*)layout_type);
/* icon set */	
    icon_set = lookup_widget(config_window, "icon_set");
    if(icon_set){
	temp_string = gtk_combo_box_get_active_text(GTK_COMBO_BOX(icon_set));
	if(strcmp(app->config->icon_set, temp_string)){
	    if(app->config->icon_set)
		g_free(app->config->icon_set);
	    app->config->icon_set = g_strdup(temp_string);
	    memset(path_large_icon, 0, sizeof(path_large_icon));
	    sprintf(path_large_icon, "%s%s/", ICONS_PATH, app->config->icon_set);
	}
	g_free(temp_string);
    }
/* icon size */
    icon_size = lookup_widget(config_window, "icon_size");
    if(icon_size){
	app->config->icons_size = hildon_controlbar_get_value(icon_size);
	(!app->config->icons_size) && (app->config->icons_size = 1);
    }
/* separate */
    separate = lookup_widget(config_window, "separate");
    if(separate)
	app->config->separate = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(separate));
/* hide station name */
    hide_station_name = lookup_widget(config_window, "hide_station_name");
    if(hide_station_name)
	app->config->hide_station_name = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_station_name));
/* hide arrows */
    hide_arrows = lookup_widget(config_window, "hide_arrows");
    if(hide_arrows)
	app->config->hide_arrows = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hide_arrows));
/* transparency */
    transparency = lookup_widget(config_window, "transparency");
    if(transparency)
	app->config->transparency = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(transparency));
/* background color */
    background_color = lookup_widget(config_window, "background_color");
    if(background_color)
    	gtk_color_button_get_color(GTK_COLOR_BUTTON(background_color), &(app->config->background_color));
/* font color */
    font_color = lookup_widget(config_window, "font_color");
    if(font_color)
    	gtk_color_button_get_color(GTK_COLOR_BUTTON(font_color), &(app->config->font_color));
/* save settings */
    new_config_save(app->config);
    flag_update_station = TRUE;
    weather_frame_update(TRUE);
}
/*******************************************************************************/
void close_button_handler(GtkButton *button, gpointer user_data){
    GtkWidget	*config_window = GTK_WIDGET(user_data),
		*notebook = NULL;

    notebook = lookup_widget(config_window, "notebook");
    if(notebook)
	app->config->current_settings_page = 
		gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    gtk_widget_destroy(config_window);
}
/*******************************************************************************/
void about_button_handler(GtkButton *button, gpointer user_data){
    create_about_dialog();
}
/*******************************************************************************/
void back_button_handler(GtkButton *button, gpointer user_data){
    gtk_widget_destroy(GTK_WIDGET(user_data));
    weather_window_popup(NULL, NULL, NULL);
}
/*******************************************************************************/
void add_button_handler(GtkButton *button, gpointer user_data){
    GtkWidget		*config = GTK_WIDGET(user_data),
			*station_name_entry = NULL,
		        *station_code_entry = NULL,
			*stations = NULL;
    gchar		*pressed_button = NULL;
    GtkTreeModel	*model = NULL;
    GtkTreeIter		iter;
    gchar		*station_name = NULL,
			*station_code = NULL;

    /* get pressed button name */    
    pressed_button = (gchar*)gtk_widget_get_name(GTK_WIDGET(button));
    if(!strcmp((char*)pressed_button, "add_name") ||
	    !strcmp((char*)pressed_button, "add_code")){
	station_name_entry = lookup_widget(config, "station_name_entry");
	station_code_entry = lookup_widget(config, "station_code_entry");
    }
    else{
	stations = lookup_widget(config, "stations");
	if(stations){
	    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(stations), &iter)){
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(stations));
		gtk_tree_model_get(model, &iter, 0, &station_name,
					    1, &station_code, -1);
		add_station_to_user_list(station_name, station_code, FALSE);
		g_free(station_name);
		g_free(station_code);
		new_config_save(app->config);
		flag_update_station = TRUE;
		weather_frame_update(TRUE);
		gtk_combo_box_set_active((GtkComboBox*)stations, -1);
	    }
	}
    }
    fprintf(stderr, "\ninside handler\n");
}
/*******************************************************************************/
