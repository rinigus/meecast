/* vim: set sw=4 ts=4 et: */
/*
 * This file is part of omweather-foreca-com-stations-db
 *
 * Copyright (C) 2012 Vlad Vasilyeu
 * 	for the code
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU  General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU  General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
*/
/*******************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "meego-main.h"
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>
/*******************************************************************************/
#define buff_size 2048
static GHashTable *data = NULL;

/*******************************************************************************/
gint
parse_and_write_xml_data(const gchar *station_id, htmlDocPtr doc, const gchar *result_file){
    gchar       buff[256],
                buffer[buff_size],
                current_temperature[20],
                current_icon[10],
                current_title[1024],
                current_pressure[15],
                current_humidity[15],
                current_wind_direction[15],
                current_wind_speed[15];
    gchar       temp_buffer[buff_size];
    GSList      *forecast = NULL;
    GSList      *tmp = NULL;
    GHashTable  *day = NULL;
    gboolean    flag;
    gboolean    night_flag;
    gint        size;
    gint        i, j;
    GHashTable *hash_for_translate;
    GHashTable *hash_for_icons;
    xmlXPathContextPtr xpathCtx; 
    xmlXPathObjectPtr xpathObj = NULL; 
    xmlXPathObjectPtr xpathObj2 = NULL; 
    xmlXPathObjectPtr xpathObj3 = NULL; 
    xmlXPathObjectPtr xpathObj4 = NULL; 
    xmlXPathObjectPtr xpathObj5 = NULL; 
    xmlXPathObjectPtr xpathObj6 = NULL; 
    xmlXPathObjectPtr xpathObj7 = NULL; 
    xmlXPathObjectPtr xpathObj8 = NULL; 
    xmlXPathObjectPtr xpathObj9 = NULL; 
    xmlNodeSetPtr nodes;
    gchar       *temp_char;
    gchar       *temp_char2;
    gint        pressure; 
    gint        speed;

    gchar       *image = NULL;
    double      time_diff = 0;
    time_t      loc_time;
    time_t      utc_time;
    gint        location_timezone = 0;
    gboolean timezone_flag = FALSE;
    gboolean sunrise_flag = FALSE;
    struct tm   tmp_tm_loc = {0};
    struct tm   tmp_tm = {0};
    struct tm   current_tm = {0};
    struct tm   tm_l = {0};
    struct tm   tmp_tm2 = {0};
    struct tm   *tm;
    time_t      t_start = 0, t_end = 0,
                t_sunrise = 0, t_sunset = 0,
                current_time = 0;
    FILE        *file_out;

    fprintf(stderr,"dddddddddddddddd");
    file_out = fopen(result_file, "w");
    if (!file_out)
        return -1;
    fprintf(file_out,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<station name=\"Station name\" id=\"%s\" xmlns=\"http://omweather.garage.maemo.org/schemas\">\n", station_id);
    fprintf(file_out," <units>\n  <t>C</t>\n  <ws>m/s</ws>\n  <wg>m/s</wg>\n  <d>km</d>\n");
    fprintf(file_out,"  <h>%%</h>  \n  <p>mmHg</p>\n </units>\n");

   hash_for_icons = hash_icons_forecacom_table_create();
   /* Create xpath evaluation context */
   xpathCtx = xmlXPathNewContext(doc);
   if(xpathCtx == NULL) {
        fprintf(stderr,"Error: unable to create new XPath context\n");
         return(-1);
   }
   /* Register namespaces from list (if any) */
   xmlXPathRegisterNs(xpathCtx, (const xmlChar*)"html",
                                (const xmlChar*)"http://www.w3.org/1999/xhtml");
   /* Day weather forecast */
   /* Evaluate xpath expression */
   // xpathObj = xmlXPathEvalExpression((const xmlChar*)"/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/@title", xpathCtx);
   xpathObj = xmlXPathEvalExpression((const xmlChar*)"/html/body/div/div/table//tr/th[@colspan='3']", xpathCtx);
  
   if(xpathObj == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", "/html/body/div/div/table//tr/th[@colspan='3']/text()");
        xmlXPathFreeContext(xpathCtx); 
        return(-1);
   }

  nodes   = xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;
  if (size > 10)
    size = 10;
  fprintf(stderr, "SIZE!!!!!!!!!!!!!!: %i\n", size); 
 // xpathObj2 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0' and @title]/text()", xpathCtx);
  xpathObj2 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th[@title]/text()", xpathCtx);
 // xpathObj3 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c3']/text()", xpathCtx);
  xpathObj3 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*[@class='temp']/text()", xpathCtx);
//  xpathObj4 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c1']/div/img/@src", xpathCtx);
  xpathObj4 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*[@class='clicon']/img/@src", xpathCtx);
 // xpathObj5 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c2']/span/text()", xpathCtx);
  xpathObj5 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*[@class='cltext']/text()", xpathCtx);
 // xpathObj6 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c4']/text()", xpathCtx);
  xpathObj6 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*/text()", xpathCtx);
//  xpathObj7 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c5']/div/text()", xpathCtx);
  xpathObj7 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*/dl[@class='wind']/dd/text()", xpathCtx);
//  xpathObj8 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c5']/div/img/@title", xpathCtx);
  xpathObj8 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*/dl/dt/text()", xpathCtx);
//  xpathObj9 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/td[@class='c0']/following-sibling::*[@class='c6']/text()", xpathCtx);
  xpathObj9 = xmlXPathEvalExpression("/html/body/div/div/div/div/div/div/div/table/tbody/tr/th/following-sibling::*/text()", xpathCtx);
  /* fprintf(stderr, "Result (%d nodes):\n", size); */
  for(i = 0; i < size; ++i) {
      day = NULL;

      /* Take time: */
      if (!nodes->nodeTab[i]->children->content)
          continue;
      temp_char = strstr(nodes->nodeTab[i]->children->content, " ");
      int j = 0;
      if (temp_char != NULL){
          for (j=0; j<strlen(temp_char)-1; j++){
              if (temp_char[j] == ' ' || temp_char[j] == '\n')
                  continue; 
              else{
                  temp_char = temp_char + j;
                  break;
              }
          }
      }
      current_time = time(NULL);
      tm = localtime(&current_time);

      setlocale(LC_TIME, "POSIX");
      strptime((const char*)temp_char, "%b %d", &tmp_tm);
      setlocale(LC_TIME, "");
      /* set begin of day in localtime */
      tmp_tm.tm_year = tm->tm_year;
      tmp_tm.tm_hour = 0; tmp_tm.tm_min = 0; tmp_tm.tm_sec = 0;

      t_start = mktime(&tmp_tm);
      fprintf(file_out,"    <period start=\"%li\"", (t_start +1));
      /* set end of day in localtime */
      t_end = t_start + 3600*24 - 1;
      fprintf(file_out," end=\"%li\">\n", t_end);
 
       
      fprintf(stderr, "Day %s\n",temp_char);

#if 0
      /* Check Day and Night */
      if (xpathObj2 && !xmlXPathNodeSetIsEmpty(xpathObj2->nodesetval) && xpathObj2->nodesetval->nodeTab[i]->content && (
           !strcmp(xpathObj2->nodesetval->nodeTab[i]->content, "Ночь")||
           !strcmp(xpathObj2->nodesetval->nodeTab[i]->content, "День"))
          ){
            if (!strcmp(xpathObj2->nodesetval->nodeTab[i]->content, "Ночь"))
                night_flag = TRUE;
            /* fprintf(stderr,"Day : %s\n", xpathObj2->nodesetval->nodeTab[i]->content); */
      }else{
          continue;
      }
      /* Look up this day in hash */
      while(tmp){
          day = (GHashTable*)tmp->data;
          if (g_hash_table_lookup(day, "day_date") &&
              !strcmp(g_hash_table_lookup(day,"day_date"), buff)){
                  flag = TRUE;
                  break;
          }
          tmp = g_slist_next(tmp);
      }
#endif
 
         /* added temperature */
         if (xpathObj3 && !xmlXPathNodeSetIsEmpty(xpathObj3->nodesetval) &&
             xpathObj3->nodesetval->nodeTab[i] && xpathObj3->nodesetval->nodeTab[i]->content){
             /* fprintf (stderr, "temperature %s\n", xpathObj3->nodesetval->nodeTab[i]->content); */
             snprintf(buffer, sizeof(buffer)-1,"%s", xpathObj3->nodesetval->nodeTab[i]->content);
             memset(temp_buffer, 0, sizeof(temp_buffer));
             for (j = 0 ; (j<(strlen(buffer)) && j < buff_size); j++ ){
                 if ((uint)buffer[j] == 226 ||  buffer[j] == '-' || (buffer[j]>='0' && buffer[j]<='9')){
                     if ((uint)buffer[j] == 226){
                        sprintf(temp_buffer,"%s-",temp_buffer);
                     }
                     else
                        sprintf(temp_buffer,"%s%c",temp_buffer, buffer[j]);
                 }
             }
			 /* fprintf(stderr, "     <temperature>%s</temperature>\n", temp_buffer); */
			 fprintf(file_out,"     <temperature>%s</temperature>\n", temp_buffer); 
         }
         /* added icon */
         if (xpathObj4 && !xmlXPathNodeSetIsEmpty(xpathObj4->nodesetval) &&
             xpathObj4->nodesetval->nodeTab[i] && xpathObj4->nodesetval->nodeTab[i]->children->content){
            temp_char = strrchr((char*)xpathObj4->nodesetval->nodeTab[i]->children->content, '/');
            temp_char ++;
            /* fprintf (stderr, "icon %s %s \n", xpathObj4->nodesetval->nodeTab[i]->children->content, choose_hour_weather_icon(hash_for_icons, temp_char)); */
            //fprintf(file_out,"     <icon>%s</icon>\n",  choose_hour_weather_icon(hash_for_icons, temp_char));
         }
         /* added text */
         if (xpathObj5 && !xmlXPathNodeSetIsEmpty(xpathObj5->nodesetval) &&
             xpathObj5->nodesetval->nodeTab[i] && xpathObj5->nodesetval->nodeTab[i]->content){
             /* fprintf (stderr, "description %s\n", xpathObj5->nodesetval->nodeTab[i]->content); */
             fprintf(file_out,"     <description>%s</description>\n", hash_forecacom_table_find(hash_for_translate, xpathObj5->nodesetval->nodeTab[i]->content, FALSE));
         }
         /* added pressure */
         if (xpathObj6 && !xmlXPathNodeSetIsEmpty(xpathObj6->nodesetval) &&
             xpathObj6->nodesetval->nodeNr >= (i*5+2) &&
             xpathObj6->nodesetval->nodeTab[i*5+2] && xpathObj6->nodesetval->nodeTab[i*5+2]->content){
             pressure = atoi((char*)xpathObj6->nodesetval->nodeTab[i*5+2]->content);
             pressure = pressure * 1.333224;
			fprintf(file_out,"     <pressure>%i</pressure>\n", pressure);
         }
         /* added wind speed */
         if (xpathObj7 && !xmlXPathNodeSetIsEmpty(xpathObj7->nodesetval) &&
             xpathObj7->nodesetval->nodeTab[i] && xpathObj7->nodesetval->nodeTab[i]->content){
            /* Normalize speed to km/h from m/s */
            /* fprintf(stderr, "Wind  speed    \n"); */ 
            speed = atoi (xpathObj7->nodesetval->nodeTab[i]->content);
			fprintf(file_out,"     <wind_speed>%1.f</wind_speed>\n",  (double)(speed));
         }
         /* added wind direction */
         if (xpathObj8 && !xmlXPathNodeSetIsEmpty(xpathObj8->nodesetval) &&
             xpathObj8->nodesetval->nodeTab[i] && xpathObj8->nodesetval->nodeTab[i]->content){
             /* fprintf (stderr, "Wind direction: %s\n", xpathObj8->nodesetval->nodeTab[i]->content); */ 
             snprintf(buffer, sizeof(buffer)-1,"%s", xpathObj8->nodesetval->nodeTab[i]->content);
             /* Wind direction */
             if (!strcoll(buffer, "З"))
                  sprintf(buffer,"%s","W");
             if (!strcoll(buffer, "Ю"))
                  sprintf(buffer,"%s","S");
             if (!strcoll(buffer, "В"))
                  sprintf(buffer,"%s","E");
             if (!strcoll(buffer, "С"))
                  sprintf(buffer,"%s","N");
             if (!strcoll(buffer, "ЮЗ"))
                  sprintf(buffer,"%s","SW");
             if (!strcoll(buffer, "ЮВ"))
                  sprintf(buffer,"%s","SE");
             if (!strcoll(buffer, "СЗ"))
                  sprintf(buffer,"%s","NW");
             if (!strcoll(buffer, "СВ"))
                  sprintf(buffer,"%s","NE");
             if (!strcoll(buffer, "безветрие"))
                  sprintf(buffer,"%s","CALM");
             if (!strcoll(buffer, "Ш"))
                  sprintf(buffer,"%s","CALM");
			 fprintf(file_out,"     <wind_direction>%s</wind_direction>\n", buffer);
         }
         /* added humidity */
         if (xpathObj9 && !xmlXPathNodeSetIsEmpty(xpathObj9->nodesetval) &&
             xpathObj9->nodesetval->nodeNr >= (i*5+3) &&
             xpathObj9->nodesetval->nodeTab[i*5+3] && xpathObj9->nodesetval->nodeTab[i*5+3]->content){
            /* fprintf (stderr, "temperature %s\n", xpathObj9->nodesetval->nodeTab[i*5+3]->content); */
			fprintf(file_out,"     <humidity>%s</humidity>\n", g_strdup(xpathObj9->nodesetval->nodeTab[i*5+3]->content));
         }
	 /* added feels like */
         if (xpathObj9 && !xmlXPathNodeSetIsEmpty(xpathObj9->nodesetval) &&
             xpathObj9->nodesetval->nodeNr >= (i*5+4) &&
             xpathObj9->nodesetval->nodeTab[i*5+4] && xpathObj9->nodesetval->nodeTab[i*5+4]->content){
		snprintf(buffer, sizeof(buffer)-1,"%s", xpathObj9->nodesetval->nodeTab[i*5+4]->content);
             	memset(temp_buffer, 0, sizeof(temp_buffer));
             	for (j = 0 ; (j<(strlen(buffer)) && j < buff_size); j++ ){
                	 if (buffer[j] == '-' || (buffer[j]>='0' && buffer[j]<='9'))
                     		sprintf(temp_buffer,"%s%c",temp_buffer, buffer[j]);
            	 }
		 fprintf(file_out,"     <flike>%s</flike>\n", temp_buffer); 
         }

      fprintf(file_out,"    </period>\n");




  }	
  /* Cleanup */
  if (xpathObj)
    xmlXPathFreeObject(xpathObj);
  if (xpathObj2)
    xmlXPathFreeObject(xpathObj2);
  if (xpathObj3)
    xmlXPathFreeObject(xpathObj3);
  if (xpathObj4)
    xmlXPathFreeObject(xpathObj4);
  if (xpathObj5)
    xmlXPathFreeObject(xpathObj5);
  if (xpathObj6)
    xmlXPathFreeObject(xpathObj6);
  if (xpathObj7)
    xmlXPathFreeObject(xpathObj7);
  if (xpathObj8)
    xmlXPathFreeObject(xpathObj8);
  if (xpathObj9)
    xmlXPathFreeObject(xpathObj9);
  /* fill current data */
  utc_time = mktime(&current_tm);
  if (utc_time != -1){
      fprintf(file_out,"    <period start=\"%li\"", utc_time);
      fprintf(file_out," end=\"%li\" current=\"true\">\n", utc_time + 4*3600); 

      fprintf(file_out,"     <temperature>%s</temperature>\n", current_temperature); 
      fprintf(file_out,"     <icon>%s</icon>\n",  current_icon);
      fprintf(file_out,"     <description>%s</description>\n", current_title);
      fprintf(file_out,"     <pressure>%s</pressure>\n", current_pressure);
      fprintf(file_out,"     <wind_direction>%s</wind_direction>\n", current_wind_direction);
      fprintf(file_out,"     <humidity>%s</humidity>\n", current_humidity);
      fprintf(file_out,"     <wind_speed>%s</wind_speed>\n", current_wind_speed);
      fprintf(file_out,"    </period>\n");
  }
// Sun rise  /html/body/div/*//div/div/div/div/div[2]/ul[@class='sun']/li[1]/text() 
//
///html/body/div/*//div/div/div/div/div[2]/ul/@title
  xpathObj = xmlXPathEvalExpression((const xmlChar*)"/html/body/div/*//div/div/div/div/div[2]/ul[1]/@title", xpathCtx);
  
  if (xpathObj && !xmlXPathNodeSetIsEmpty(xpathObj->nodesetval) &&
             xpathObj->nodesetval->nodeTab[0] && xpathObj->nodesetval->nodeTab[0]->children->content){
        temp_char = strrchr((char*)xpathObj->nodesetval->nodeTab[0]->children->content, ' ');
        temp_char ++;
        strptime(temp_char, "%d.%m.%Y", &current_tm);
        current_tm.tm_min = 0;
        current_tm.tm_hour = 0;
        utc_time = mktime(&current_tm);
        fprintf(file_out,"    <period start=\"%li\"", utc_time);
        fprintf(file_out," end=\"%li\">\n", utc_time + 24*3600); 
        sunrise_flag = TRUE;
  }
  if (xpathObj)
    xmlXPathFreeObject(xpathObj);

  xpathObj = xmlXPathEvalExpression((const xmlChar*)"/html/body/div/*//div/div/div/div/div[2]/ul[@class='sun']/li[1]/text()", xpathCtx);
  if (xpathObj && !xmlXPathNodeSetIsEmpty(xpathObj->nodesetval) && xpathObj->nodesetval->nodeTab[0]->content){
      setlocale(LC_TIME, "POSIX");
      strptime(xpathObj->nodesetval->nodeTab[0]->content, "%H:%M", &current_tm);
      setlocale(LC_TIME, "");
      utc_time = mktime(&current_tm);
      fprintf(file_out,"    <sunrise>%li</sunrise>\n", utc_time);
  }
  if (xpathObj)
    xmlXPathFreeObject(xpathObj);

  xpathObj = xmlXPathEvalExpression((const xmlChar*)"/html/body/div/*//div/div/div/div/div[2]/ul[@class='sun']/li[2]/text()", xpathCtx);
  if (xpathObj && !xmlXPathNodeSetIsEmpty(xpathObj->nodesetval) && xpathObj->nodesetval->nodeTab[0]->content){
        setlocale(LC_TIME, "POSIX");
        strptime(xpathObj->nodesetval->nodeTab[0]->content, "%H:%M", &current_tm);
        setlocale(LC_TIME, "");
        utc_time = mktime(&current_tm);
        fprintf(file_out,"    <sunset>%li</sunset>\n", utc_time);
  }
  if (xpathObj)
      xmlXPathFreeObject(xpathObj);

  if (sunrise_flag)
      fprintf(file_out,"    </period>");

  /* Clean */
  g_hash_table_destroy(hash_for_translate);
  g_hash_table_destroy(hash_for_icons);
  if (xpathCtx)
    xmlXPathFreeContext(xpathCtx); 

  fclose(file_out);

  return size/4;
}

/*******************************************************************************/

convert_station_forecacom_data(const gchar *station_id_with_path, const gchar *result_file,  gboolean get_detail_data){
 
    xmlDoc  *doc = NULL;
    xmlNode *root_node = NULL;
    gint    days_number = -1;
    gchar   buffer[1024],
            *delimiter = NULL;
    
    if(!station_id_with_path)
        return -1;
/* check for new file, if it exist, than rename it */
    *buffer = 0;
    snprintf(buffer, sizeof(buffer) - 1, "%s.new", station_id_with_path);
    if(!access(buffer, R_OK))
        rename(buffer, station_id_with_path);
    /* check file accessability */
    if(!access(station_id_with_path, R_OK)){
        /* check that the file containe valid data */
        doc =  htmlReadFile(station_id_with_path, "UTF-8", 0);
        if(!doc)
            return -1;
        root_node = xmlDocGetRootElement(doc);
        if(root_node->type == XML_ELEMENT_NODE &&
                strstr((char*)root_node->name, "err")){
            xmlFreeDoc(doc);
            xmlCleanupParser();
            return -2;
        }
        else{
            /* prepare station id */
            *buffer = 0;
            delimiter = strrchr(station_id_with_path, '/');
            if(delimiter){
                delimiter++; /* delete '/' */
                snprintf(buffer, sizeof(buffer) - 1, "%s", delimiter);
                delimiter = strrchr(buffer, '.');
                if(!delimiter){
                    xmlFreeDoc(doc);
                    xmlCleanupParser();
                    return -1;
                }
                *delimiter = 0;
             //   if(get_detail_data)
             //       days_number = parse_xml_detail_data(buffer, root_node, data);
             //   else
                days_number = parse_and_write_xml_data(buffer, doc, result_file);
            }
            xmlFreeDoc(doc);
            xmlCleanupParser();
        }
    }
    else
        return -1;/* file isn't accessability */
    return days_number;
}
/*******************************************************************************/
int
main(int argc, char *argv[]){
    int result; 
    if (argc < 3) {
        fprintf(stderr, "forecacom <input_file> <output_file>\n");
        return -1;
    }
    result = convert_station_forecacom_data(argv[1], argv[2], FALSE);
    //fprintf(stderr, "\nresult = %d\n", result);
    return result;
}
