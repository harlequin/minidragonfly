/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 element
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <json/json.h>
#include "mongoose.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <zlib.h>
#include "config.h"
#include "version.h"
#include "rpc.h"
#include "types.h"
#include "minixml.h"
#include "sql.h"
#include "globalvars.h"
#include "utils.h"
#include "log.h"
#include "parser.h"

#include "processor.h"


void add_series_by_id(struct mg_connection *conn,int id, char *language, int status, int quality, int match, char *location);
void list_series(struct mg_connection *conn);
void list_episodes(struct mg_connection *conn, int id);
void search_episode(struct mg_connection *conn, const char *show_name, const char *season, const char *episode);
void get_latest_news(struct  mg_connection *conn);




const char* response_ok ( const char *data ) {
	char* result;
	int cl = strlen(data);
	result = (char*) malloc(cl + 200);
	sprintf(result,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s",
		cl, data);
	return result;
}


const char* rpc_response( int result, char *data ) {
	char *header;
	char *response;

	header = malloc(1024);
	sprintf(header, "{\"version\":\"" MINIDRAGONFLY_VERSION "\", \"hash\":\"%s\",\"result\":true,\"data\":", build_git_sha);


	response = malloc(strlen(header) + strlen(data) + 200);

	sprintf(response,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s%s}",
		strlen(header) + strlen(data) + 1, header, data);

	return response;
}




typedef struct {
	char *response;
	char *warning;
} response_struct;


void version( char *response ) {
	strcpy(response, "\"\"");
}

void process( char *response, array_list *list) {
	char *location;

	location = malloc(255);

	if(!location) {
		strcpy(response, "\"internal memory allocation error\"");
		return;
	}

	strcpy(location, json_object_get_string(list->array[0]));
	DPRINTF(E_DEBUG, L_WEBSERVER, "Process %s\n", location);
	if(processor(location) == 0) {
		strcpy(response, "\"Location will be scanned\"");
	} else {
		strcpy(response, "\"can not start location\"");
	}


	if(location)
		free(location);
}

void minidragonfly_shutdown (char *response) {
	strcpy(response, "\"minidragonfly is shutting down\"");
	quitting = 1;
}

void delete(char *response, array_list *list) {
	char *id;

	if(list->length < 1) {
		strcpy(response, "\"not enough parameters\"");
	}

	id = strdup(json_object_get_string(list->array[0]));
	DPRINTF(E_INFO, L_WEBSERVER, "Deleting tv show %s\n", id);

	sql_exec(db, sqlite3_mprintf("DELETE FROM tv_serie WHERE id = %s", id));
	sql_exec(db, sqlite3_mprintf("DELETE FROM tv_episode WHERE tv_series_id = %s", id));
	sql_exec(db, sqlite3_mprintf("DELETE FROM recent_news WHERE tv_show_id = %s", id));

	strcpy(response, "\"tv show deleted\"");
}


void search(char *response, array_list *params) {

	if(params->length == 0) {
		/*//search all*/
		fetch_feed_items(NULL);
	} else if(params->length == 1) {
		fetch_feed_items(json_object_get_string(params->array[0]));
		/*//search for special id*/
	} else {
		strcpy(response,"\"too many parameters\"");
	}

}

void config_get(char *response, array_list *params) {
	int i;
	char *v;
	char *buffer;
	buffer = malloc(1024);


	v = malloc(10);

	/*//response = 0;*/



	if(params->length == 0) {
		strcpy(response,"\"no config parameter\"");
	} else {
		/*//DPRINTF(E_DEBUG, L_GENERAL, "Inside config-get %s...\n",json_object_get_string(params->array[0]));*/

		if(strcmp("quality-names", json_object_get_string(params->array[0])) == 0) {

			strcpy(buffer, "[");
			for(i = 0; i < 10; i++) {


				strcat(buffer, "{\"name\":\"");
				strcat(buffer, quality_names[i].name);
				strcat(buffer, "\",");
				strcat(buffer, "\"value\":");
				sprintf(v, "\"%d\"", quality_names[i].quality);
				strcat(buffer, v);
				/*//buffer += sprintf(buffer, "%d", quality_presets_names[i].quality_preset);*/
				strcat(buffer, "}");

				if(i < 9)
					strcat(buffer, ",");
				/*//DPRINTF(E_DEBUG,L_GENERAL, "%s\n", quality_names[i].name);*/
			}

			strcat(buffer, "]");


		} else {

	/*//quality-names*/

			strcpy(buffer, "[");
			for(i = 0; i < 5; i++) {


				strcat(buffer, "{\"name\":\"");
				strcat(buffer, quality_presets_names[i].name);
				strcat(buffer, "\",");
				strcat(buffer, "\"value\":");
				sprintf(v, "\"%d\"", quality_presets_names[i].quality_preset);
				strcat(buffer, v);
				/*//buffer += sprintf(buffer, "%d", quality_presets_names[i].quality_preset);*/
				strcat(buffer, "}");

				if(i < 4)
					strcat(buffer, ",");
				/*//DPRINTF(E_DEBUG,L_GENERAL, "%s\n", quality_presets_names[i].name);*/
			}

			strcat(buffer, "]");
		}

		strcpy(response, buffer);
	}

}

/**
 * {
 * 	version:
 * 	result: {true/false}
 * 	data: ...
 * }
 */
void *json_api[] = {
	"version", version,
	"delete", delete,
	"search", search,
	"config", config_get,
	"process", process,
	"shutdown", minidragonfly_shutdown,
	NULL
};




int begin_request_handler(struct mg_connection *conn) {

	const struct mg_request_info *request_info = mg_get_request_info(conn);

	char post_data[1024];
	//char method[255];
	int post_data_size;
	json_object *obj;
	json_object *json_method;
	int i;
	array_list *list = NULL;


	char *error_code;
	char *answer;


	void (*callback)(char *, void *);



	if( strcmp(request_info->uri, "/jsonrpc") == 0 ) {

		post_data_size = mg_read( conn, post_data, sizeof(post_data)) ;
		DPRINTF(E_DEBUG, L_WEBSERVER, "[%d] %s\n", post_data_size, post_data);

		obj = json_tokener_parse(post_data);

		if( !is_error(obj) ) {

			json_method = json_object_object_get(obj, "method");

			if( json_method != NULL ) {
				DPRINTF(E_DEBUG, L_WEBSERVER, ">> %s\n", json_object_get_string(json_method));

				for(i = 0; json_api[i] != NULL; i++) {

					if(strcmp(json_object_get_string(json_method), json_api[i]) == 0) {
						list = json_object_get_array( json_object_object_get(obj,"params") );

						answer = malloc(1024);
						callback = (void*) json_api[i+1];
						callback(answer, list);
						/*//DPRINTF(E_DEBUG, L_WEBSERVER, "RESPONSE %s\n", answer);*/
						mg_printf( conn, rpc_response(FALSE, answer));
						return 0;
					}
				}
				/*//when we are reaching this point than we have no valid method name*/
			} else {
				/*//give response that no method json was given*/
			}
		} else {

			answer = malloc(1024);
			error_code = malloc(1);

			sprintf(error_code, "%d", (int) obj);
			sprintf(answer, "%s", json_tokener_errors[ (-1) * atoi(error_code) ]);
			DPRINTF(E_WARN, L_WEBSERVER, "json parse error: %s\n", answer);
			mg_printf( conn, rpc_response(FALSE, answer));

			free(answer);
			free(error_code);
		}









	} else if(strcmp(request_info->uri, "/json") == 0) {

		DPRINTF(E_DEBUG, L_WEBSERVER , "Query string %s\n", request_info->query_string);
		if(request_info->query_string != NULL) {

			char buffer[255];
			int res;

			res = mg_get_var(request_info->query_string, strlen(request_info->query_string),"method", buffer, 255);

			/*//we have found the method parameter*/
			if(strlen(buffer) > 0) {
				if(strcmp(buffer, "search_series_by_name") == 0) {

					/*//try to get the search term*/
					char query[255];
					char language[255];
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"q", query, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"lanuage", language, 255);

					if(strlen(query) > 0) {
						DPRINTF(E_DEBUG, L_GENERAL, "Searching series by name %s [%s]\n", query, language);
						/*//search params*/
						search_for_series_by_name(conn, query, language);
					}


				} else if(strcmp(buffer, "add_series_by_id") == 0) {
					/*//try to get the search term*/
					char query[255];
					char language[255];
					char status[255];

					char quality[255];
					char match[255];
					char location[255];

					mg_get_var(request_info->query_string, strlen(request_info->query_string),"q", query, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"language", language, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"status", status, 255);

					mg_get_var(request_info->query_string, strlen(request_info->query_string),"quality", quality, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"match", match, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"location", location, 255);

					if(strlen(query) > 0) {
						DPRINTF(E_DEBUG, L_GENERAL, "Add series by id %s [%s]\n", query, language);
						add_series_by_id(conn, atoi(query), language, atoi(status), atoi(quality), atoi(match), location);
					}


				} else if(strcmp(buffer, "list_series") == 0) {
					list_series(conn);

				} else if(strcmp(buffer, "get_latest_news") == 0) {
					get_latest_news(conn);

				} else if(strcmp(buffer, "list_episodes") == 0) {
					/*//try to get the search term*/
					char query[255];
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"q", query, 255);
					if(strlen(query) > 0) {
						list_episodes(conn, atoi(query));
					}


					/*/search_episode(struct mg_connection *conn, const char *show_name, const char *season, const char *episode)*/
				} else if(strcmp(buffer, "search_episode") == 0) {
					/*//try to get the search term*/
					char tvshow[255];
					char season[255];
					char episode[255];
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"tvshow", tvshow, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"season", season, 255);
					mg_get_var(request_info->query_string, strlen(request_info->query_string),"episode", episode, 255);
					search_episode(conn, tvshow, season, episode);
				}









			}



			if(res) {
				DPRINTF(E_DEBUG, L_GENERAL, "Webserver get method \"%s\"\n", buffer);
			} else {
				DPRINTF(E_DEBUG, L_GENERAL, "Webserver get ERROR\n");
			}
		} else {
			mg_printf( conn, response_ok("miniscan json rpc"));
		}
	}

	return 0;
}


static void
NameValueParserStartElt(void * d, const char * name, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
    if(l>63)
        l = 63;
    memcpy(data->curelt, name, l);
    data->curelt[l] = '\0';

    /* store root element */
    if(!data->head.lh_first)
    {
        struct NameValue * nv;
        nv = malloc(sizeof(struct NameValue)+l+1);
        strcpy(nv->name, "rootElement");
        memcpy(nv->value, name, l);
        nv->value[l] = '\0';
        LIST_INSERT_HEAD(&(data->head), nv, entries);
    }
}

static void
NameValueParserGetData(void * d, const char * datas, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
    struct NameValue * nv;
    if(l>1975)
        l = 1975;
    nv = malloc(sizeof(struct NameValue)+l+1);
    strncpy(nv->name, data->curelt, 64);
    nv->name[63] = '\0';
    memcpy(nv->value, datas, l);
    nv->value[l] = '\0';
    LIST_INSERT_HEAD(&(data->head), nv, entries);
}



void parse_tvdb_xml(const char *buffer, int bufsize, struct NameValueParserData *data)
{
	 	struct xmlparser parser;
	    LIST_INIT(&(data->head));
	    /* init xmlparser object */
	    parser.xmlstart = buffer;
	    parser.xmlsize = bufsize;
	    parser.data = data;
	    parser.starteltfunc = NameValueParserStartElt;
	    parser.endeltfunc = 0;
	    parser.datafunc = NameValueParserGetData;
	    parser.attfunc = 0;
	    parsexml(&parser);
}






void search_for_series_by_name(struct mg_connection *conn,char *name, char *language) {

	json_object *obj = json_object_new_object();
	json_object *result;
	json_object *jarray;
	json_object *series;
	json_object *jstring;

	char *buffer;
	char *poster;
	struct memory_struct *chunk;

	struct NameValueParserData xml;
	struct NameValue * nv;

	int hasName;
	int hasId;

	chunk = malloc(sizeof(struct memory_struct));

/*
	//int hasOverview = FALSE;
*/
	hasName = FALSE;
	hasId = FALSE;

	buffer = (char *) malloc(255);
	DPRINTF(E_DEBUG, L_GENERAL, "Searching for %s.\n", name);

	name = str_replace(name, " ", "%20");
	sprintf(buffer, "http://www.thetvdb.com/api/GetSeries.php?seriesname=%s",name);
	DPRINTF(E_DEBUG, L_GENERAL, "%s.\n", buffer);
	download(buffer, chunk);




	if(chunk->size > 0)
	{
		result = json_object_new_boolean(TRUE);
		json_object_object_add(obj, "result", result);
		jarray = json_object_new_array();

		DPRINTF(E_DEBUG, L_GENERAL, "Chunk size is %d.\n", chunk->size);

		parse_tvdb_xml( chunk->memory , chunk->size, &xml);

		series = json_object_new_object();
		for(nv = xml.head.lh_first; nv != NULL; nv = nv->entries.le_next)
		{
			if(strcmp(nv->name, "SeriesName") == 0) {
				jstring = json_object_new_string(nv->value);
				json_object_object_add(series,"name", jstring);
				hasName = TRUE;
			}

			if(strcmp(nv->name, "seriesid") == 0) {
				jstring = json_object_new_string(nv->value);
				json_object_object_add(series,"id", jstring);
				/*
				//poster
				//
				 */

				poster = malloc(255);
				sprintf(poster, "http://www.thetvdb.com/banners/posters/%s-1.jpg", nv->value);
				json_object_object_add(series,"poster", json_object_new_string(poster));

				hasId = TRUE;
			}

			/*
			if(strcmp(nv->name, "Overview") == 0) {
				json_object_object_add(series,"overview", json_object_new_string(nv->value));
				hasOverview = TRUE;
			}
			 */
			if(hasName && hasId ) {
				json_object_array_add(jarray,series);
				series = json_object_new_object();
				hasName = FALSE;
				hasId = FALSE;
				/*
				//hasOverview = FALSE;
				 *
				 */
			}
		}

		json_object_object_add(obj,"series", jarray);

	} else {
		result = json_object_new_boolean(FALSE);
		DPRINTF(E_ERROR, L_GENERAL, "Chunk size is zero\n");
	}

	mg_printf( conn, response_ok(json_object_to_json_string(obj)));

	if(chunk->memory)
		free(chunk->memory);

	free(buffer);
}















void parse_episodes(struct NameValueParserData *xml, struct tv_episode_struct *tv_episodes, char *language, int status )
{
	struct tv_episode_struct *curr, *head;
	char **result;
	int cols, rows;
	char *sql;
	struct NameValue *nv;
	char *buffer;


	DPRINTF(E_DEBUG, L_WEBSERVER, "Parse episodes ...\n", "");

	buffer = malloc(1024);
	sql = malloc(1024);
	curr = NULL;
	head = NULL;

	for(nv = xml->head.lh_first; nv != NULL; nv = nv->entries.le_next)
	{
		if( strcmp(nv->name,"seriesid") == 0)
		{

			curr = (struct tv_episode_struct *) malloc(sizeof(struct tv_episode_struct));
			curr->tv_series_id = atoi(nv->value);
			curr->status = status;
			curr->season = 0;
			curr->episode = 0;
			curr->title = "";
			curr->language = language;


			curr->next = head;
			head = curr;
		}

		if(strcmp(nv->name, "SeasonNumber") == 0) {
			curr->season = atoi(nv->value);
		}


		if(strcmp(nv->name, "EpisodeNumber") == 0) {
			curr->episode = atoi(nv->value);
		}

		if(strcmp(nv->name, "EpisodeName") == 0)
		{
			/*//printf("XML PARSER %s\n", nv->value);*/
			curr->title = nv->value;
		}

	}


	/*//DPRINTF(E_DEBUG, L_GENERAL, "start working\n");*/


	/*reset list*/
	curr = head;

	/*//DPRINTF(E_DEBUG, L_GENERAL, "List reset\n");*/
	while(curr)
	{



		/*//DPRINTF(E_DEBUG, L_GENERAL, "Insert into database %s\n", curr->title);*/

		sql = sqlite3_mprintf("SELECT * FROM tv_episode WHERE tv_series_id = %d AND season = %d AND episode = %d", curr->tv_series_id, curr->season, curr->episode);

		if( (sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows )
		{
			/*//DPRINTF(E_ERROR, L_GENERAL, "Entry found - maybe update\n");*/
		}
		else
		{
			/*
			//printf(":: TITLE: %s\n", curr->title);
			//DPRINTF(E_DEBUG, L_GENERAL, "== TITLE %s\n", curr->title);

			//if ( curr->season == 0 ) {
			//	buffer = sqlite3_mprintf("INSERT INTO tv_episode (tv_series_id, season, episode, title, status, language) VALUES (%d, '%d', '%d', \"%s\", '%d', '%s')\0", curr->tv_series_id, curr->season, curr->episode, curr->title, EPISODE_STATUS_IGNORED, curr->language);
			//} else {
			 *
			 */
				buffer = sqlite3_mprintf("INSERT INTO tv_episode (tv_series_id, season, episode, title, status, language) VALUES (%d, '%d', '%d', \"%s\", '%d', '%s')\0", curr->tv_series_id, curr->season, curr->episode, curr->title,curr->status, curr->language);

			sql_exec(db, buffer);
		}

		curr = curr->next;
	}
}


/**
 * Add a new tv show into the global library
 */
void add_series_by_id(struct mg_connection *conn, int id, char *language, int status, int quality, int match, char* location) {
	/*
	//Todo: check if series is not already in table if yes throw an error
	 *
	 */
	struct memory_struct *chunk;



	json_object *obj = json_object_new_object();
	char *uri;
	struct NameValueParserData xml;
	struct NameValue * nv;
	char *series_name = malloc(255);
	char *overview;
	struct tv_episode_struct tv_episodes;


	char **result;
	int cols, rows;

	char *sql =	sqlite3_mprintf("SELECT tv_serie.title, tv_serie.id FROM tv_serie WHERE tv_serie.id = '%d'", id);
	chunk = malloc(sizeof(struct memory_struct));
	/*//check if we have it already in database*/
	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		DPRINTF(E_DEBUG, L_GENERAL, "EXISTS\n", "");
		json_object_object_add(obj, "result", json_object_new_boolean(FALSE));
		json_object_object_add(obj,"message", json_object_new_string("TV Show already existing"));
		DPRINTF(E_DEBUG, L_GENERAL, "%s\n", json_object_to_json_string(obj));
		mg_printf( conn, response_ok(json_object_to_json_string(obj)));
		return;
	} else {
		DPRINTF(E_DEBUG, L_GENERAL, "NOT EXISTS\n");
	}
	/*
	//hdclearart
	//tvthumb
	 *
	 */
	download_fanart_clearart(id, "hdclearart");
	download_fanart_clearart(id, "tvthumb");

	if(sql_exec(db, sqlite3_mprintf("INSERT INTO recent_news VALUES (NULL, %d, 'New show added')", id )) == SQLITE_OK) {

	}








	uri = malloc(255);
	sprintf(uri, "http://www.thetvdb.com/api/%s/series/%d/all/%s.xml",options[OPT_TVDB_APIKEY], id, "en" /*language*/);

	DPRINTF(E_DEBUG, L_GENERAL, "Downloading series info for %d\n", id);
	DPRINTF(E_DEBUG, L_GENERAL, "%s\n", uri);
	download(uri , chunk);

	if(chunk->size > 0) {
		json_object_object_add(obj, "result", json_object_new_boolean(TRUE));

		parse_tvdb_xml( chunk->memory , chunk->size, &xml);

		DPRINTF(E_DEBUG, L_GENERAL, "xml is parsed ... get shows now\n", "");

		for(nv = xml.head.lh_first; nv != NULL; nv = nv->entries.le_next) {

			if(strcmp(nv->name, "SeriesName") == 0) {
				strcpy(series_name, nv->value);
				DPRINTF(E_DEBUG, L_GENERAL, "Found series name %s\n", series_name);
				continue;
			}

			if(strcmp(nv->name, "Overview") == 0)  {

				overview = s_malloc(strlen(nv->value) + 1);
/*
				//overview = malloc(strlen(nv->value) + 1);
*/
				strncpyt(overview, nv->value, strlen(nv->value) + 1);
				/*
				//strcpy(overview, nv->value);
				 *
				 */
			}

		}

		sql_exec(db, sqlite3_mprintf("INSERT INTO tv_serie VALUES (%d, '%s', '%d', '%d', '%d', '%s')",id, series_name, quality, match, location, str_replace(overview, "'", "")));
		parse_episodes(&xml,&tv_episodes, language, status);


	} else {
		json_object_object_add(obj, "result", json_object_new_boolean(FALSE));
	}

	mg_printf( conn, response_ok(json_object_to_json_string(obj)));

	/*//free all variables*/
	free(uri);
	if(chunk->memory)
		free(chunk->memory);




}

void list_series(struct mg_connection *conn) {

	char **result, **languages;
	int cols, rows, lcols, lrows;
	int i, j;

	json_object *obj = json_object_new_object();
	json_object *array;
	json_object *series;


	char *sql =	sqlite3_mprintf("SELECT tv_serie.title, tv_serie.id, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id AND status = 0) as skipped, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id AND status = 1) as wanted, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id AND status = 2) as archived, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id AND status = 3) as ignored, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id AND status = 4) as snatched, \
			(SELECT count(status) FROM tv_episode WHERE tv_episode.tv_series_id = tv_serie.id ) as total \
			FROM tv_serie ORDER BY tv_serie.title DESC");

	json_object_object_add(obj, "result", json_object_new_boolean(TRUE));
		array = json_object_new_array();


	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {

			json_object *json_languages = json_object_new_array();
			sql = sqlite3_mprintf("SELECT DISTINCT tv_episode.language FROM tv_episode WHERE tv_episode.tv_series_id = %s", result[i+1]);
			DPRINTF(E_DEBUG, L_GENERAL, "sql: %s\n", sql);
			if ((sql_get_table(db, sql, &languages, &lrows, &lcols) == SQLITE_OK) && lrows) {
				for (j = lcols; j < lrows * lcols + lcols; j += lcols) {
					DPRINTF(E_DEBUG, L_GENERAL, "Found language\n");
					json_object_array_add(json_languages, json_object_new_string(languages[j]));
				}
			}

			series = json_object_new_object();
			json_object_object_add(series,"id", json_object_new_string(result[i+1]));
			json_object_object_add(series,"name", json_object_new_string(result[i]));


			json_object_object_add(series,"skipped", json_object_new_string(result[i+2]));
			json_object_object_add(series,"wanted", json_object_new_string(result[i+3]));
			json_object_object_add(series,"archived", json_object_new_string(result[i+4]));
			json_object_object_add(series,"ignored", json_object_new_string(result[i+5]));
			json_object_object_add(series,"snatched", json_object_new_string(result[i+6]));
			json_object_object_add(series,"total", json_object_new_string(result[i+7]));


			json_object_object_add(series,"languages", json_languages);
			json_object_array_add(array,series);
			DPRINTF(E_DEBUG, L_GENERAL, "%s\n", result[i]);
		}
	}

	json_object_object_add(obj,"series", array);
	mg_printf( conn, response_ok(json_object_to_json_string(obj)));
}



/**
 * List a tv show episodes by a id
 */
void list_episodes(struct mg_connection *conn, int id) {



	char **result;
	int cols, rows;
	int i;

	json_object *obj = json_object_new_object();
	json_object *array;
	json_object *series;




	char *sql =	sqlite3_mprintf("SELECT tv_episode.title, tv_episode.season, tv_episode.episode, tv_episode.status, tv_episode.id, tv_episode.nzb, tv_episode.language, tv_episode.snatched_quality FROM tv_episode WHERE tv_episode.tv_series_id = '%d' ORDER BY tv_episode.season ASC, tv_episode.episode ASC", id);

	DPRINTF(E_DEBUG, L_GENERAL, "List episodes for %d\n", id);

		json_object_object_add(obj, "result", json_object_new_boolean(TRUE));
		array = json_object_new_array();


	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {
			series = json_object_new_object();
			json_object_object_add(series,"id", json_object_new_string(result[i+4]));
			json_object_object_add(series,"title", json_object_new_string(result[i]));
			json_object_object_add(series,"season", json_object_new_string(result[i+1]));
			json_object_object_add(series,"episode", json_object_new_string(result[i+2]));
			json_object_object_add(series,"status", json_object_new_string(result[i+3]));
			json_object_object_add(series,"language", json_object_new_string(result[i+6]));

			if(result[i+5] == NULL) {
				json_object_object_add(series,"nzb", json_object_new_string(""));
			} else {
				json_object_object_add(series,"nzb", json_object_new_string(result[i+5]));
			}

			if(result[i+7] == NULL) {
				json_object_object_add(series,"quality", json_object_new_string(""));
			} else {
				json_object_object_add(series,"quality", json_object_new_string(result[i+7]));
			}


			json_object_array_add(array,series);
		}
	}

	json_object_object_add(obj,"series", array);
	mg_printf( conn, response_ok(json_object_to_json_string(obj)));
}


void get_latest_news(struct  mg_connection *conn) {
	char **result;
	int cols, rows;
	int i;

	json_object *obj = json_object_new_object();
	json_object *array;
	json_object *series;


	char *sql =	sqlite3_mprintf("SELECT recent_news.tv_show_id, recent_news.comment, tv_serie.overview FROM recent_news, tv_serie WHERE tv_serie.id = recent_news.tv_show_id");

	json_object_object_add(obj, "result", json_object_new_boolean(TRUE));
	array = json_object_new_array();

	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {
			series = json_object_new_object();
			json_object_object_add(series,"id", json_object_new_string(result[i]));
			json_object_object_add(series,"comment", json_object_new_string(result[i+1]));
			if(result[i+2] == NULL)
				json_object_object_add(series,"overview", json_object_new_string(""));
			else
				json_object_object_add(series,"overview", json_object_new_string(result[i+2]));
			json_object_array_add(array,series);
		}
	}

	json_object_object_add(obj,"feeds", array);
	mg_printf( conn, response_ok(json_object_to_json_string(obj)));
}

void search_episode(struct mg_connection *conn, const char *show_name, const char *season, const char *episode) {
	json_object *obj = json_object_new_object();

	json_object_object_add(obj, "result", json_object_new_boolean(TRUE));
	mg_printf( conn, response_ok(json_object_to_json_string(obj)));
}
