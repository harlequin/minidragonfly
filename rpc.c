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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <zlib.h>
#include <json/json.h>

#include <json/json_object_private.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "mongoose.h"
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

// -- http://sickbeard.com/api/#episode

//episode	 			displays the information of a specific episode.
//episode.search	 	initiate a manual search for a specific episode.
//episode.setstatus	 	set the status of an episode.

//exceptions	 display scene exceptions for all or a given show.
//future	 display the upcoming episodes.

//history	 display sickbeard's downloaded/snatched history.
//history.clear	 clear sickbeard's history.
//history.trim	 trim sickbeard's history.

//logs	 view sickbeard's log.


//show	 display information for a given shown.
//show.addexisting	 add a show in sickbeard with an existing folder.
//show.addnew	 add a show in sickbeard.
//show.cache	 display if the poster/banner sickbeard's image cache is valid.
//show.delete	 delete a show from sickbeard.
//show.getbanner	 retrieve the shows banner from sickbeard's cache.
//show.getposter	 retrieve the shows poster from sickbeard's cache.
//show.getquality	 get quality settings for a show in sickbeard.
//show.pause	 set a show's paused state in sickbeard.
//show.refresh	 refresh a show in sickbeard.
//show.seasonlist	 display the season list for a given show.
//show.seasons	 display a listing of episodes for all or a given season.
//show.setquality	 set quality for a show in sickbeard.
//show.stats	 display episode statistics for a given show.
//show.update	 update a show in sickbeard.
//shows	 			display all shows in sickbeard.
//shows.stats	 display the global episode and show statistics.


//sb	 display misc sickbeard related information.
//sb.addrootdir	 add a root (parent) directory.
//sb.checkscheduler	 query the sickbeard scheduler.
//sb.deleterootdir	 delete a root (parent) directory.
//sb.forcesearch	 force the episode search early.
//sb.getdefaults	 get sickbeard user defaults.
//sb.getmessages	 get messages from the notification queue.
//sb.getrootdirs	 get root (parent) directories set by user.
//sb.pausebacklog	 pause the backlog search.
//sb.ping	 check to see if sickbeard is running.
//sb.restart	 restart sickbeard.
//sb.searchtvdb	 search tvdb for a show name or tvdbid.
//sb.setdefaults	 set sickbeard user defaults.
//sb.shutdown	 shutdown sickbeard.




void add_series_by_id(struct mg_connection *conn,int id, char *language, int status, int quality, int match, char *location);
void parse_tvdb_xml(const char *buffer, int bufsize, struct NameValueParserData *data);


void rpc_response( int result, char **data ) {

	sprintf( *data, "{\"version\":\"" MINIDRAGONFLY_VERSION "\", \"hash\":\"%s\",\"result\":true,\"data\":%s}",
			build_git_sha, strdup(*data));

	sprintf( *data,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s",
		strlen(*data), strdup(*data));
}




typedef struct {
	char *response;
	char *warning;
} response_struct;







void version( char *response ) {
	strcpy(response, "\"\"");
}



void process( char *response, char **params, size_t size) {
	DPRINTF(E_DEBUG, L_WEBSERVER, "Process %s\n", params[0]);
	if(size == 0) {
		strcpy(response, "\"not enough parameter\"");
	}

	if(processor(params[0]) == 0) {
		strcpy(response, "\"Location will be scanned\"");
	} else {
		strcpy(response, "\"can not start location\"");
	}
}

void minidragonfly_shutdown (char *response) {
	strcpy(response, "\"minidragonfly is shutting down\"");
	quitting = 1;
}

void delete(char *response, char **params, size_t size) {
	char *id;

	if(size == 0) {
		strcpy(response, "\"not enough parameters\"");
	}

	id = params[0];
	DPRINTF(E_INFO, L_WEBSERVER, "Deleting tv show %s\n", id);

	sql_exec(db, sqlite3_mprintf("DELETE FROM tv_serie WHERE id = %s", id));
	sql_exec(db, sqlite3_mprintf("DELETE FROM tv_episode WHERE tv_series_id = %s", id));
	sql_exec(db, sqlite3_mprintf("DELETE FROM recent_news WHERE tv_show_id = %s", id));

	strcpy(response, "\"tv show deleted\"");
}


void search(char *response, char **params, size_t size) {

	if(size == 0) {
		/*//search all*/
		fetch_feed_items(NULL);
	} else if(size == 1) {
		fetch_feed_items(params[0]);
		/*//search for special id*/
	} else {
		strcpy(response,"\"too many parameters\"");
	}

}

void minidragonfly_searchtvdb(char *response, char **params, size_t size) {

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

	char *series_name;


	const char *search_token;


	if(size == 1) {



		search_token = params[0];
		chunk = s_malloc(sizeof(struct memory_struct));
		hasName = FALSE;
		hasId = FALSE;
		buffer = (char *) s_malloc(255);
		DPRINTF(E_DEBUG, L_GENERAL, "Searching for %s.\n", search_token);

		search_token = str_replace(search_token, " ", "%20");
		sprintf(buffer, "http://www.thetvdb.com/api/GetSeries.php?seriesname=%s",search_token);
		DPRINTF(E_DEBUG, L_GENERAL, "%s.\n", buffer);
		download(buffer, chunk);

		if(chunk->size > 0)	{

			result = json_object_new_boolean(TRUE);
			json_object_object_add(obj, "result", result);
			jarray = json_object_new_array();

			DPRINTF(E_DEBUG, L_GENERAL, "Chunk size is %d.\n", chunk->size);

			parse_tvdb_xml( chunk->memory , chunk->size, &xml);

			series = json_object_new_object();
			for(nv = xml.head.lh_first; nv != NULL; nv = nv->entries.le_next) {

				if(strcmp(nv->name, "SeriesName") == 0) {
					/* replace the returned &amp; with the real char */
					series_name = s_malloc(strlen(nv->value));
					series_name = str_replace(nv->value, "&amp;", "&");

					jstring = json_object_new_string(series_name);
					json_object_object_add(series,"name", jstring);
					hasName = TRUE;
				}

				if(strcmp(nv->name, "seriesid") == 0) {
					jstring = json_object_new_string(nv->value);
					json_object_object_add(series,"id", jstring);
					//poster = s_malloc(255);
					//sprintf(poster, "http://www.thetvdb.com/banners/posters/%s-1.jpg", nv->value);
					//json_object_object_add(series,"poster", json_object_new_string(poster));
					hasId = TRUE;
				}

				if(hasName && hasId ) {
					json_object_array_add(jarray,series);
					series = json_object_new_object();
					hasName = FALSE;
					hasId = FALSE;
				}
			}

			json_object_object_add(obj,"series", jarray);


			strcpy( response, json_object_to_json_string(jarray));



		} else {
			strcpy(response, "\" no series found ...\"");
		}

		if(chunk->memory)
			free(chunk->memory);
		free(buffer);


	} else {
		strcpy(response,"\"parameters\"");
	}
}


/**
 * Parameter:
 * 	tvdbid		- Required
 * 	sort		- Optional 		asc, [desc]
 */
void show_seasonlist(char *response, char **params, size_t size) {
	const char *id;
	char **result;
	char *sql;
	int cols, rows, i;

	if( size == 0) {
		strcpy(response, "\"wrong parameter\"");
	}

	id = params[0];

	sql = sqlite3_mprintf("SELECT tv_episode.season FROM tv_episode WHERE tv_episode.tv_series_id = '%s' GROUP BY tv_episode.season", id);

	strcpy( response, "[");
	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {

			strcat( response, result[i]);
			if (i < rows * cols + cols - 1)
				strcat( response, ",");
		}
	}
	strcat( response, "]");

	sqlite3_free_table(result);
	free(sql);
}


void show_addnew(char* response, char **params, size_t size) {
	const char *tvdbid;
	const char *language;
	const char *status;
	const char *quality;

	if ( size == 0 ) {
		strcpy( response, "\"error\"");
	} else {
		tvdbid = params[0];
		language =params[1];
		status = params[2];
		quality = params[3];

		add_series_by_id(NULL, atoi(tvdbid), (char*) language, atoi(status), atoi(quality), 0, "");
		//void add_series_by_id(struct mg_connection *conn,int id, char *language, int status, int quality, int match, char *location);
		strcpy( response, "\"ok\"");
	}

	//add_series_by_id(struct mg_connection *conn,int id, char *language, int status, int quality, int match, char *location)
}

void shows(char* response) {
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
				FROM tv_serie ORDER BY tv_serie.title ASC");

		json_object_object_add(obj, "result", json_object_new_boolean(TRUE));
			array = json_object_new_array();


		if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
			for (i = cols; i < rows * cols + cols; i += cols) {

				json_object *json_languages = json_object_new_array();
				sql = sqlite3_mprintf("SELECT DISTINCT tv_episode.language FROM tv_episode WHERE tv_episode.tv_series_id = %s", result[i+1]);
				DPRINTF(E_DEBUG, L_GENERAL, "sql: %s\n", sql);
				if ((sql_get_table(db, sql, &languages, &lrows, &lcols) == SQLITE_OK) && lrows) {
					for (j = lcols; j < lrows * lcols + lcols; j += lcols) {
						DPRINTF(E_DEBUG, L_GENERAL, "Found language\n", "");
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


		sqlite3_free_table(result);

		//strcpy("[");
		strcpy( response, json_object_to_json_string(array));
		//strcpy("]");
}


#define _X(x,y) "\"" x "\":\"" y "\""

void episodes(char* response, char **params, size_t size) {
	char **result;
	char *sql;
	int cols, rows;
	int i;
	const char *id;
	json_object *array;
	json_object *series;
	char *items;

	const char *JSON_EPISODE_ITEM =
		"{"
			_X("id", "%s") ","
			_X("title", "%s") ","
			_X("season", "%s") ","
			_X("episode", "%s") ","
			_X("status", "%s") ","
			_X("language", "%s")
		"}%s";



	if(size == 0) {
		strcpy(response, "\"wrong parameter\"");
		return;
	}


	items = s_malloc(1);
	id = params[0];
	DPRINTF(E_DEBUG, L_GENERAL, "List episodes for %s\n", id);

	/*											0				1					2					3					4			5					6						7							8        */
	sql = sqlite3_mprintf("SELECT tv_episode.title, tv_episode.season, tv_episode.episode, tv_episode.status, tv_episode.id, tv_episode.nzb, tv_episode.language, tv_episode.snatched_quality, tv_episode.watched FROM tv_episode WHERE tv_episode.tv_series_id = '%s' ORDER BY tv_episode.season DESC, tv_episode.episode DESC", id);

	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {

			char *buffer;
			size_t size;
			buffer = malloc(1024);
			size = sprintf(buffer, JSON_EPISODE_ITEM,
					result[i+4],
					result[i],
					result[i+1],
					result[i+2],
					result[i+3],
					result[i+6],
					((i < rows * cols)) ? "," : "" );

			if( strlen(items) < ( strlen(items) + size + 2 ) ) {
				items = realloc( items, strlen(items) + size + 2);
				if(!items) {
					DPRINTF(E_FATAL, L_WEBSERVER, "Realloc failed \n","");
				}
			}
			size = sprintf(items, "%s%s", items, buffer);
		}
	}

	sprintf(response, "[%s]", items);

	sqlite3_free_table(result);
}



void stats(char* response) {
	int tv_shows_count = 0;

	tv_shows_count = sql_get_int_field(db, "SELECT COUNT(tv_serie.id) FROM tv_serie");
	sprintf( response, "{\"shows_total\":\"%d\"}", tv_shows_count);
}

void ui_data(char* response) {
	int i;

	/* episode states */
	sprintf( response, "{ \"episode_states\": [");
	for(i = 0; i < EPISODE_STATUS_MAX; i++) {
		sprintf( response, "%s{\"name\":\"%s\", \"value\":\"%d\"}",response, episode_status_names[i], i);
		if( i < EPISODE_STATUS_MAX - 1)
			sprintf( response,"%s,", response );
	}
	sprintf( response, "%s ]", response);

	/* quality */
	sprintf( response, "%s, \"quality\": [", response);
	for(i = 0; i < 10; i++) {
		sprintf( response, "%s{\"name\":\"%s\", \"value\":\"%d\"}",response, quality_names[i].name, quality_names[i].quality);
		if( i < 10 - 1)
			sprintf( response,"%s,", response );
	}
	sprintf( response, "%s ]", response);

	/* quality presets */
	sprintf( response, "%s, \"quality_presets\": [", response);
	for(i = 0; i < 5; i++) {
		sprintf( response, "%s{\"name\":\"%s\", \"value\":\"%d\"}",response, quality_presets_names[i].name, quality_presets_names[i].quality_preset);
		if( i < 5 - 1)
			sprintf( response,"%s,", response );
	}
	sprintf( response, "%s ]}", response);
}

void news(char* response) {
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

			//if(result[i+2] == NULL)
			//	json_object_object_add(series,"overview", json_object_new_string(""));
			//else
			//	json_object_object_add(series,"overview", json_object_new_string(result[i+2]));
			json_object_array_add(array,series);
		}
	}

	sprintf( response, json_object_to_json_string(array));
	//json_object_object_add(obj,"feeds", array);
	//mg_printf( conn, response_ok(json_object_to_json_string(obj)));
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
	"stats", stats,
	"ui.data", ui_data,

	"delete", delete,
	"search", search,
	"process", process,
	"shutdown", minidragonfly_shutdown,
	"searchtvdb", minidragonfly_searchtvdb,

	"show.seasonlist", show_seasonlist,
	"show.addnew", show_addnew,
	"shows", shows,
	"episodes", episodes,
	"news", news,
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
	json_object_iter it;
	void (*callback)(char *, void *, size_t size);
	int k;
	char **params;
	const char *value;


	if( strcmp(request_info->uri, "/jsonrpc") == 0 ) {

		post_data_size = mg_read( conn, post_data, sizeof(post_data)) ;

		obj = json_tokener_parse(post_data);

		if( !is_error(obj) ) {

			json_method = json_object_object_get(obj, "method");

			if( json_method != NULL ) {
				DPRINTF(E_DEBUG, L_WEBSERVER, ">> %s\n", json_object_get_string(json_method));

				for(i = 0; json_api[i] != NULL; i++) {

					if(strcmp(json_object_get_string(json_method), json_api[i]) == 0) {
						list = json_object_get_array( json_object_object_get(obj,"params") );

						params = (char **) malloc( list->length * sizeof(char *) );
						for(k = 0; k < list->length; k++) {
							value = json_object_get_string( list->array[k] );
							params[k] = malloc( strlen( value ) );
							strcpy(params[k], value);
						}

						answer = malloc(16000);
						callback = (void*) json_api[i+1];
						callback(answer, params, list->length);

						DPRINTF(E_DEBUG, L_WEBSERVER, ">> %s\n", answer);

						mg_printf(conn, "HTTP/1.0 200 OK\r\n"
								"Content-Type: text/plain\r\n"
								"\r\n"
								"{\"version\":\"" MINIDRAGONFLY_VERSION "\", \"hash\":\"%s\",\"result\":true,\"data\":%s}",
								build_git_sha, answer
						);

						/* free all */
						for(k = 0; k < list->length; k++)
							free(params[k]);
						free(params);
						free(answer);

						return 1;
					}
				}
				/* when we are reaching this point than we have no valid method name*/
			} else {
				/* give response that no method json was given*/
			}
		} else {

			answer = s_malloc(1024);
			error_code = s_malloc(1);

			sprintf(error_code, "%d", (int) obj);
			sprintf(answer, "%s", json_tokener_errors[ (-1) * atoi(error_code) ]);
			DPRINTF(E_WARN, L_WEBSERVER, "json parse error: %s\n", answer);
			//mg_printf( conn, rpc_response(FALSE, answer));
			free(answer);
			free(error_code);
		}
		return 1;
	}

	return 0;
}


static void NameValueParserStartElt(void * d, const char * name, int l) {
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

static void NameValueParserGetData(void * d, const char * datas, int l) {
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



void parse_tvdb_xml(const char *buffer, int bufsize, struct NameValueParserData *data) {
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

void parse_episodes(struct NameValueParserData *xml, struct tv_episode_struct *tv_episodes, char *language, int status ) {
	struct tv_episode_struct *curr, *head;
	char **result;
	int cols, rows;
	char *sql;
	struct NameValue *nv;
	char *buffer;


	DPRINTF(E_DEBUG, L_WEBSERVER, "Parse episodes ...\n", "");

	buffer = s_malloc(1024);
	sql = s_malloc(1024);
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
	char *network;
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
		//mg_printf( conn, response_ok(json_object_to_json_string(obj)));
		return;
	} else {
		DPRINTF(E_DEBUG, L_GENERAL, "NOT EXISTS\n", "");
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
				series_name = str_replace(series_name, "&amp;", "&");
				DPRINTF(E_DEBUG, L_GENERAL, "Found series name %s\n", series_name);
				continue;
			}

			if(strcmp(nv->name, "Overview") == 0)  {
				overview = s_malloc(strlen(nv->value) + 1);
				strncpyt(overview, nv->value, strlen(nv->value) + 1);
			}

			if(strcmp(nv->name, "Network") == 0)  {
				network = s_malloc(strlen(nv->value) + 1);
				strncpyt(network, nv->value, strlen(nv->value) + 1);
			}

		}

		sql_exec(db, sqlite3_mprintf("INSERT INTO tv_serie VALUES (%d, '%s', '%d', '%d', '%d', '%s', '%s')",id, str_replace(series_name, "'", ""), quality, match, location, str_replace(overview, "'", ""), network));
		parse_episodes(&xml,&tv_episodes, language, status);

		download_itunes_covers(id);

	} else {
		json_object_object_add(obj, "result", json_object_new_boolean(FALSE));
	}

	//mg_printf( conn, response_ok(json_object_to_json_string(obj)));

	/*//free all variables*/
	free(uri);
	if(chunk->memory)
		free(chunk->memory);




}


