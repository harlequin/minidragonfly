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

#ifndef PARSER_H_
#define PARSER_H_

#include "types.h"


typedef struct {
	const char *series_name;
	const char *season_num;
	const char *ep_num;
	const char *extra_ep_num;
	const char *extra_info;
	const char *release_group;



	char *ep_name;
	char *tv_name;

	int quality;

	unsigned int used_regex;
} parsed_episode_t;

typedef enum {
	QUALITY_NONE 			= 0,
	QUALITY_SDTV 			= 1,
	QUALITY_SDDVD 			= 1 << 1,
	QUALITY_HDTV			= 1 << 2,
	QUALITY_RAWHDTV 		= 1 << 3,
	QUALITY_FULLHDTV 		= 1 << 4,
	QUALITY_HDWEBDL			= 1 << 5,
	QUALITY_FULLHDWEBDL 	= 1 << 6,
	QUALITY_HDBLURAY 		= 1 << 7,
	QUALITY_FULLHDBLURAY 	= 1 << 8,

	/* must be the last element */
	QUALITY_UNKNOWN			= 1 << 15
} quality_e;

typedef enum {
	QUALITY_PRESETS_SD			= QUALITY_SDTV | QUALITY_SDDVD,
	QUALITY_PRESETS_HD			= QUALITY_HDTV | QUALITY_FULLHDTV | QUALITY_HDWEBDL | QUALITY_FULLHDWEBDL | QUALITY_HDBLURAY | QUALITY_FULLHDBLURAY | QUALITY_RAWHDTV,
	QUALITY_PRESETS_HD720P		= QUALITY_HDTV | QUALITY_HDWEBDL | QUALITY_HDBLURAY,
	QUALITY_PRESETS_HD1080P		= QUALITY_FULLHDTV | QUALITY_FULLHDWEBDL | QUALITY_FULLHDBLURAY,

	/* Any is a combination from the SD and HD preset */
	QUALITY_PRESETS_ANY			= QUALITY_PRESETS_SD | QUALITY_PRESETS_HD
} quality_presets_e;


typedef enum {
	LANGUAGE_ENGLISH 		= 0,
	LANGUAGE_GERMAN,

	LANGUAGE_MAX
} languages_e;


int /*boolean*/ language_compare ( const char *str, languages_e language );

typedef struct  {
	quality_e quality;
	const char *name;
} quality_names_t;

typedef struct {
	quality_presets_e quality_preset;
	const char *name;
}quality_presets_names_t;

typedef struct feed_item_struct {
	const char *title;
	const char *link;
	//tv_epidose_t *parsed_episode;
	parsed_episode_t *parsed_episode;
	struct feed_item_struct *next;
} feed_item_t;




extern quality_presets_names_t quality_presets_names[];
extern quality_names_t quality_names[];

/**
 * Query all sites ...
 *
 * Parameters:
 * 	... (additional parameters)
 * 	If the amount of parameter is greater than 0; than the specified tv show
 * 	are queried, otherwise all tv show id are taken
 */
void
query_sites (char *tv_show_id, ...);


/**
 * Parse string into a parsed_episode_t strcut
 *
 * Parameters:
 * 	str		C String
 *
 * Return Value:
 * 	parsed_episode_t
 */
parsed_episode_t *
parse_title (const char *str);


/**
 * Determinate quality in string
 * Returns a integer value for the quality which is found
 * in the C string str.
 *
 * Returns a 0 when no quality was found.
 *
 * Parameters:
 * 	str		C String
 *
 *
 * Return Value:
 * 	Returns the corresponding quality integer
 * 	If no quality was found, the function returns a 0
 */
quality_e strqual ( const char *str );


void *parse_all ();
int fetch_feed_items(const char *tvshow_id);
int parse( tv_epidose_t *tv_episode, char *in);


#endif /* PARSER_H_ */
