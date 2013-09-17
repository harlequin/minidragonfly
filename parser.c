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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pcre.h>
/*
//#ifdef WIN32
//#include <io.h>
//#else
//#include <unistd.h>
//#endif
*/

#include <json/json.h>


#include "globalvars.h"
#include "config.h"
#include "utils.h"
#include "log.h"
#include "sql.h"
#include "parser.h"

/*
//enum COLOR {
//  red,
//  green,
//  blue
//};
//
//char *color_name[] = {
//  red="red",
//  green="green",
//  blue="blue"
//};
*/

enum TV_SHOWS_REGEXES {
		REG_STANDARD_REPEAT = 0,
		REG_FOV_REPEAT,
		REG_STANDARD,
		NUMBER_OF_REGEXES
};

char *tvshow_regexes[] = {
		"^(?P<series_name>.+?)[. _-]+"                /*// Show_Name and separator*/
		"s(?P<season_num>\\d+)[. _-]*"                /*// S01 and optional separator*/
		"e(?P<ep_num>\\d+)"                           /*// E02 and separator*/
		"([. _-]+s(?P=season_num)[. _-]*"             /*// S01 and optional separator*/
		"e(?P<extra_ep_num>\\d+))+"                   /*// E03/etc and separator*/
		"[. _-]*((?P<extra_info>.+?)"                 /*// Source_Quality_Etc-*/
		"((?<![. _-])(?<!WEB)"                        /*// Make sure this is really the release group*/
		"-(?P<release_group>[^- ]+))?)?$",            /*// Group*/

		"^(?P<series_name>.+?)[. _-]+"                /*// Show_Name and separator*/
		"(?P<season_num>\\d+)x"                       /*// 1x*/
		"(?P<ep_num>\\d+)"                            /*// 02 and separator*/
		"([. _-]+(?P=season_num)x"                    /*// 1x*/
		"(?P<extra_ep_num>\\d+))+"                    /*// 03/etc and separator*/
		"[. _-]*((?P<extra_info>.+?)"                 /*// Source_Quality_Etc-*/
		"((?<![. _-])(?<!WEB)"                        /*// Make sure this is really the release group*/
		"-(?P<release_group>[^- ]+))?)?$",            /*// Group*/

		"^((?P<series_name>.+?)[. _-]+)?"             /*// Show_Name and separator*/
		"s(?P<season_num>\\d+)[. _-]*"                /*// S01 and optional separator*/
		"e(?P<ep_num>\\d+)"                           /*// E02 and separator*/
		"(([. _-]*e|-)"                               /*// linking e/- char*/
		"(?P<extra_ep_num>(?!(1080|720)[pi])\\d+))*"  /*// additional E03/etc*/
		"[. _-]*((?P<extra_info>.+?)"                 /*// Source_Quality_Etc-*/
		"((?<![. _-])(?<!WEB)"                        /*// Make sure this is really the release group*/
		"-(?P<release_group>[^- ]+))?)?$"             /*// Group */
};

char *nzb_sites[] = {
		"http://nzbtv.net/",
		/*"http://185.8.105.59:25991/index/www/"*/ "http://nzbs2go.com/",
		/*"https://www.miatrix.com/"*/ "http://nzbs2go.com/"};

quality_presets_names_t quality_presets_names[] = {
		{QUALITY_PRESETS_SD, "SD"},
		{QUALITY_PRESETS_HD, "HD"},
		{QUALITY_PRESETS_HD720P, "HD720P"},
		{QUALITY_PRESETS_HD1080P, "HD1080P"},
		{QUALITY_PRESETS_ANY, "Any"}
};


//Todo: N/A is changed to NA due to file movement
quality_names_t quality_names[] = {
		{QUALITY_NONE, "NA"},
		{QUALITY_SDTV, "SD TV"},
		{QUALITY_SDDVD, "SD DVD"},
		{QUALITY_HDTV, "HD TV"},
		{QUALITY_RAWHDTV, "RawHD TV"},
		{QUALITY_FULLHDTV, "1080p HD TV"},
		{QUALITY_HDWEBDL, "720p WEB-DL"},
		{QUALITY_FULLHDWEBDL, "1080p WEB-DL"},
		{QUALITY_HDBLURAY, "720p BluRay" },
		{QUALITY_FULLHDBLURAY, "1080p BluRay"},

		{QUALITY_UNKNOWN, "Unknown"}
};


const char *languages_filters[LANGUAGE_MAX][2] = {
		{"","nlsub"},
		{"german|videomann","subbed"}
};


const char *additional_lang_terms[LANGUAGE_MAX][5] = {
		{NULL},
		{"%20german", "%20by%20videomann", NULL}
};


/* ############## FUNCTIONS ############## */
int match(const char *str, const char *regex, ...) {
	va_list ap;
	int result = TRUE;

	//int i;
	//int rc = -1;
	pcre *re;
	const char *error;
	int erroffset;
	int ovector[100];
	unsigned int offset = 0;

	if(str == NULL)
		return FALSE;

	va_start(ap, regex);
	while(regex) {
		//DPRINTF(E_DEBUG, L_SCANNER, "regex %s\n", regex);
		re = pcre_compile(regex, PCRE_CASELESS, &error, &erroffset, NULL );
		if(!re) {
			DPRINTF(E_DEBUG, L_SCANNER, "regex could not compiled ... [%d] %s\n", erroffset, error);
			return FALSE;
		}

		result &= pcre_exec(re, 0, str, strlen(str), offset, 0, ovector, 100) == -1 ? FALSE : TRUE;
		regex = va_arg(ap, const char *);
	}
	va_end(ap);


	return result;

}

/**
 * Compares the language
 */
/*int*/ int language_compare ( const char *str, languages_e language ) {
	int a1 = match( str, languages_filters[language][0], NULL );
	int a2 = match( str, languages_filters[language][1], NULL );
	return a1 && !a2;
	//return ( && ! match(str, languages_filters[language][1] ));
}







void
query_sites (char *tv_show_id, ...) {
	char *query;
	//char **result;
	//int rows;
	//int cols;
	//int i;
	va_list ap;
	va_start(ap, tv_show_id);

	query = malloc(1);
	
	if(tv_show_id && strlen(tv_show_id) != 0) {

		//query database with specific ids
		DPRINTF(E_DEBUG, L_GENERAL, "Taking show id for query ...\n", "");
	} else {
		DPRINTF(E_DEBUG, L_GENERAL, "Take all\n", "");

		query = sqlite3_mprintf("SELECT id, season, episode, language FROM tv_episode WHERE status = 1");
//		if ((sql_get_table(db, query, &result, &rows, &cols) == SQLITE_OK) && rows) {
//			DPRINTF(L_GENERAL, E_DEBUG, "=> %s\n", result[i+2]);
//		}
		//json_object_object_foreachC()

//#define json_object_object_foreachC(obj,iter) 
//		 for(iter.entry = json_object_get_object(obj)->head; (iter.entry ? (iter.key = (char*)iter.entry->k, iter.val = (struct json_object*)iter.entry->v, iter.entry) : 0); iter.entry = iter.entry->next)

	}

	//while loop trough the ids



	free(query);
	va_end(ap);
}

int
get_feed_items (const char *language) {


	return FALSE;
}





parsed_episode_t *
parse_title (const char *str) {
	int rc;
	int ovector[100];
	unsigned int offset = 0;
	unsigned int len = strlen(str);
	pcre *re;
	int k;
	const char *error;
	int erroffset;
	parsed_episode_t *episode;


	str = str_replace(str, ".", " ");
	str = str_replace(str, "_", " ");

	/*//DPRINTF(E_INFO, L_GENERAL, "Parsing title '%s'\n", str);*/

	for(k = 0; k < NUMBER_OF_REGEXES; k++) {

		re = pcre_compile(tvshow_regexes[k], PCRE_CASELESS, &error, &erroffset, NULL );
		if (!re) {
			DPRINTF(E_ERROR, L_GENERAL, "Regex error [%d] %s\n", erroffset, error);
			return NULL;
		}

		/*//DPRINTF(E_INFO, L_GENERAL, "Try first regex '%s'\n", tvshow_regexes[k]);*/

		rc = pcre_exec(re, 0, str, len, offset, 0, ovector, 100);

		if(rc == -1) {
			/*//DPRINTF(E_INFO, L_GENERAL, "Failed with '%d'\n", rc);*/
			continue;
		}

		episode = malloc(sizeof(parsed_episode_t));
		episode->used_regex = k;

		pcre_get_named_substring(re, str, ovector, rc,  "series_name", &episode->series_name);
		pcre_get_named_substring(re, str, ovector, rc,  "season_num", &episode->season_num);
		pcre_get_named_substring(re, str, ovector, rc,  "ep_num", &episode->ep_num);
		pcre_get_named_substring(re, str, ovector, rc,  "extra_ep_num", &episode->extra_ep_num);
		pcre_get_named_substring(re, str, ovector, rc,  "extra_info", &episode->extra_info);
		pcre_get_named_substring(re, str, ovector, rc,  "release_group", &episode->release_group);

		/*//DPRINTF(E_INFO, L_GENERAL, "Returning episode \n");*/
		return episode;
	}

	/*//DPRINTF(E_INFO, L_GENERAL, "Returning NULL \n");*/
	return NULL;
}





quality_e strqual ( const char *str ) {
	//int i;

	DPRINTF(E_DEBUG, L_SCANNER, "Quality scanner for %s\n", str);

	/* simple method - try to find quality string
	for(i = 0; i < 11; i++ ) {
		if( match ( str, str_replace( quality_names[i].name, " ", "\\W" ), NULL) ) {
			return quality_names[i].quality;
		}
	}
*/



	/* complex method - try to find quality with regex from title */
	/* added 'webhdrip' to sd */
	/* added 'web' to sd */
	if( match( str, "(pdtv|hdtv|dsr|tvrip|webrip|webhdrip|web).(xvid|x264)", NULL) && ! match(str, "(720|1080)[pi]", NULL) ) {
		return QUALITY_SDTV;
	} else if ( match(str, "(dvdrip|bdrip)(.ws)?.(xvid|divx|x264)", NULL) && ! match(str, "(720|1080)[pi]", NULL) ) {
		return QUALITY_SDDVD;
	} else if ( match(str, "720p", "hdtv", "x264", NULL) && ! match(str,"(1080)[pi]", NULL)  ) {
		return QUALITY_HDTV;
	} else if ( match(str, "720p|1080i", "hdtv", "mpeg2", NULL)) {
		return QUALITY_RAWHDTV;
	} else if ( match(str, "1080p", "hdtv", "x264", NULL)) {
		return QUALITY_FULLHDTV;
	} else if ( match(str,"720p", "web.dl|webrip", NULL) || match(str,"720p", "itunes", "h.?264", NULL) ) {
		return QUALITY_HDWEBDL;
	} else if ( match(str,"1080p", "web.dl|webrip", NULL) || match(str,"1080p", "itunes", "h.?264", NULL) ) {
		return QUALITY_FULLHDWEBDL;
	} else if ( match(str, "720p", "bluray|hddvd", "x264", NULL)) {
		return QUALITY_HDBLURAY;
	} else if ( match(str, "1080p", "bluray|hddvd", "x264", NULL)) {
		return QUALITY_FULLHDBLURAY;


	/* Special quality settings */
	} else if(match( str, "by videomann", NULL)) {
		return QUALITY_SDTV;
	} else if(match( str, "br-xvid", NULL)) {
		return QUALITY_SDTV;


	} else {
		/* nothing was found - make a dump of string and make a new regex */
		return QUALITY_UNKNOWN;
	}
}



//Todo: replace by strncasecmp
char *strcasestr(const char *haystack, const char *needle) {
	int i;
	int matchamt = 0;

	for (i = 0; i < haystack[i]; i++) {
		if (tolower(haystack[i]) != tolower(needle[matchamt])) {
			matchamt = 0;
		}
		if (tolower(haystack[i]) == tolower(needle[matchamt])) {
			matchamt++;
			if (needle[matchamt] == 0)
				return (char *) 1;
		}
	}

	return 0;
}

void *parse_all() {
	fetch_feed_items(NULL);
	return NULL ;
}

void showcmp(feed_item_t *feed_items, char *show_id) {
	char *query;
	char **result;
	int rows;
	int cols;
	int i;
	feed_item_t *item;


	//Blackhole vars ...
	struct memory_struct *chunk;
	FILE *f;
	char *nzb;

	//DPRINTF(E_DEBUG, L_SCANNER, ">>> Start compare for show %s\n", show_id);

	/*                                  0                     1                2                    3                 4                   5        */
	query = sqlite3_mprintf("SELECT tv_episode.id, tv_episode.season, tv_episode.episode, tv_episode.language, tv_serie.quality, tv_episode.status FROM tv_episode, tv_serie WHERE tv_episode.tv_series_id = tv_serie.id AND tv_episode.tv_series_id = %s AND tv_episode.status = 1",	show_id);

	if ((sql_get_table(db, query, &result, &rows, &cols) == SQLITE_OK) && rows) {

		for (i = cols; i < rows * cols + cols; i += cols) {

			item = feed_items;
			while(item){

				/*
				//if(feed_items->title != NULL)
				//	DPRINTF(E_DEBUG, L_SCANNER,	">>> Checking: %s %sx%s [%s]\n", item->parsed_episode->series_name, item->parsed_episode->season_num, item->parsed_episode->ep_num, item->title);
				 	*/

				if(!language_compare(item->title, LANGUAGE_GERMAN)) {
					item = item->next;
					continue;
				}
				/*
				if (strcmp(result[i + 3], "de") == 0) {
					DPRINTF(E_DEBUG, L_SCANNER, ">>> Start language scan\n", "");
					//if(check_language_filter(feed_items->title, CONF_DE_INCLUDE, CONF_DE_EXCLUDE))
					if(language_compare(feed_items->title, LANGUAGE_GERMAN)) {
						item = item->next;
						continue;
					}
				}


				//DPRINTF(E_DEBUG, L_SCANNER, ">>> Language scan passed\n", "");
				 */


				if ( atoi(result[i + 1]) == atoi( item->parsed_episode->season_num )  && atoi(result[i + 2]) == atoi( item->parsed_episode->ep_num ) && atoi( result[i+5] ) == EPISODE_STATUS_WANTED /*&& 0 == item->parsed_episode->quality*/) {
					//check quality
					int db_quality = atoi(result[i+4]);
					quality_e qual;
					qual = strqual(item->title);

					DPRINTF(E_DEBUG, L_SCANNER, "Quality compare: [%d] and [%d] - \n", db_quality, qual);

					if( (db_quality & qual) == qual) {
						sql_exec(db, sqlite3_mprintf("UPDATE tv_episode SET status = 4, nzb = '%s' WHERE id = '%s'", item->title, result[i]));

							

						//Todo: implement full setting ... category, prio, clean name,
						// direct method with link
						//nzbget_appendurl(item->title, minidragonfly_options[CONF_NZBGET_CATEGORY], 0, 0, item->link);

						// another method can the black hole ...

						 
						chunk = malloc(sizeof (struct memory_struct));
						download(item->link, chunk);
						if(chunk->size > 0) {
							nzb = malloc(255);
							sprintf(nzb, "%s/%s.nzb", options[OPT_BLACK_HOLE_DIR], item->title);
							f = fopen(nzb, "wb");

							fwrite(chunk->memory, chunk->size, 1, f);

							fclose(f);
							free(nzb);
							free(chunk->memory);
						}
					}
				}
				item = item->next;
			}
		}
	}
}



/**
 * - catch all feeds (for a tv show) and store it into a list of results
 * - step through the list and find the best result - maybe a feature improvement
 *   can be that more than one result will be stored to get the best out of best
 */
int fetch_feed_items(const char *tvshow_id) {

	json_object *obj;
	array_list *list;
	parsed_episode_t *episode;
	feed_item_t *feed_item, *head;

	json_object *json_result_size;
	int result_size;

	struct memory_struct *chunk;

	int site_counter;
	int j;
	int cols, rows;
	int i;
	int offset;
	int z;

	char *key;
	char *sql;//, *episode_query;
	char *query_url;
	char **result;
	char *show_name;

	key = malloc(64);
	chunk = (struct memory_struct*) malloc(sizeof(struct memory_struct));
	query_url = malloc(1024);
	show_name = malloc(64);


	if(tvshow_id != NULL) {
		sql =
			sqlite3_mprintf(

					"SELECT "
					"  tv_serie.title,"
					"  COUNT(tv_episode.id) episodes,"
					"  COUNT(distinct tv_episode.language) languages,"
					"  tv_episode.language,"
					"  tv_serie.id,"
					"  scene_name.scene_name "
					"FROM "
					"  tv_serie "
					"LEFT OUTER JOIN scene_name ON "
					"  tv_serie.title = scene_name.show_name "
					"LEFT OUTER JOIN tv_episode ON "
					"  tv_serie.id = tv_episode.tv_series_id "
					"WHERE tv_serie.id = %s "
					"GROUP BY "
					"  tv_serie.id"
					, tvshow_id);

					/*       0                  1                            2                              3               4                 5            */
					/*
				"SELECT tv_serie.title, count(tv_episode.id), count(distinct tv_episode.language), tv_episode.language, tv_serie.id, scene_name.scene_name "
				"FROM tv_serie, tv_episode, scene_name "
				"WHERE tv_serie.id = %s AND tv_serie.id == tv_episode.tv_series_id AND tv_episode.status == 1 AND scene_name.show_name = tv_serie.title GROUP BY tv_serie.title", tvshow_id);
					 */
	} else {
		sql =
			sqlite3_mprintf(
				"SELECT tv_serie.title, count(tv_episode.id), count(distinct tv_episode.language), tv_episode.language, tv_serie.id "
				"FROM tv_serie, tv_episode "
				"WHERE %s tv_serie.id == tv_episode.tv_series_id AND tv_episode.status == 1 GROUP BY tv_serie.title");
	}


	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK)) {	// && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {


			if(result[i+5] != NULL) {
				strcpy(show_name, result[i+5]);
			} else {
				strcpy(show_name, result[i]);
			}




			DPRINTF(E_INFO, L_SCANNER, "> Searching %s episodes for tv show %s in %s language(s) %s\n", result[i + 1], show_name, result[i + 2], result[i + 3]);

			for (site_counter = 0; site_counter < 3; site_counter++) {
/*
				//TODO: implement smaller timeout
				//if(site_counter == 0)
				//	continue;
*/
				if(site_counter==0 ||site_counter==2)
					continue;



				DPRINTF(E_INFO, L_SCANNER, "> Download from site %s\n", nzb_sites[site_counter]);

				key = malloc(64);				
				//TODO: REIMPLEMENT
//				switch (site_counter) {
//					case 0:
//						key = strdup(minidragonfly_options[CONF_SITE_NZBTV_KEY]);
//						break;
//					case 1:
//						key = strdup(minidragonfly_options[CONF_SITE_NEWZNZB_KEY]);
//						break;
//					case 2:
//						key = strdup(minidragonfly_options[CONF_SITE_MIATRIX_KEY]);
//						break;
//					default:
//						break;
//				}





				for(z = 0; additional_lang_terms[1][z] != NULL; z++) {
					offset = 0;
					feed_item = NULL;
					head = NULL;
					result_size = 0;
					do {
						/*
						//castle && german || videomann
						//castle%20&&%20german%20%7C%7C%20videomann
						//%%20&&%%20german%%20%%7C%%7C%%20videomann
						*/
						sprintf(query_url, "%sapi?apikey=%s&t=tvsearch&q=%s%s&o=json&offset=%d",nzb_sites[site_counter], key, str_replace(show_name, " ", "%20"), additional_lang_terms[1][z], offset);

						if(download(query_url, chunk) == -1)
							break;

						if (chunk->size > 0) {

							DPRINTF(E_WARN, L_SCANNER, "> %s\n",query_url);
							/*
							//DPRINTF(E_DEBUG, L_GENERAL, ">>> %s\n", chunk->memory);
*/
							obj = json_tokener_parse(chunk->memory);
							if( !is_error(obj) ) {
								obj = json_object_object_get(obj, "channel");
								/*
								*	//get the total result size;
								 *
								 */
								json_result_size = json_object_object_get(obj, "response");
								if(json_result_size != NULL) {
									json_result_size = json_object_object_get(json_result_size, "@attributes");
									json_result_size = json_object_object_get(json_result_size, "total");
									result_size = json_object_get_int(json_result_size);

									offset += 100;

									if(result_size == 0) {
										DPRINTF(E_WARN, L_SCANNER, "> No result where found for requested tv show %s\n",show_name );
										break;
									}
								}

								obj = json_object_object_get(obj, "item");

								if(obj == NULL) {
									DPRINTF(E_WARN, L_SCANNER, "> No result where found for requested tv show %s\n",show_name );
									break;
								}

								list = json_object_get_array(obj);

								for (j = 0; j < list->length; j++) {
									feed_item = malloc(sizeof(feed_item_t));

									feed_item->title = json_object_get_string(json_object_object_get(list->array[j],"title"));
									feed_item->link = json_object_get_string(json_object_object_get(list->array[j],"link"));

									if(feed_item->title == NULL)
										continue;

									episode = malloc(sizeof(parsed_episode_t));
									if( (episode = parse_title(feed_item->title)) != NULL ) {
										feed_item->parsed_episode = episode;
										feed_item->next = head;
										head = feed_item;
									}
								}
								/*
								//show compare was here
								 	*/
							} else {
								DPRINTF(E_WARN, L_SCANNER, "Response from server was not a json string ...\n", "");
								break;
							}
						}

						DPRINTF(E_WARN, L_SCANNER, ">> Results: %d - %d\n", result_size, offset);
					} while( (result_size - offset) > 0 );/* //End while with offset */

					showcmp(head, result[i+4]);

				} /* END all special queries */
				/* compare the list with wanted show in database */


			}
		}
	}
	return 0;
}


