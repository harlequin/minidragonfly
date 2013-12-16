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

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "globalvars.h"
#include "log.h"
#include "types.h"

volatile short int quitting = 0;
sqlite3 *db;


newznab_site_t *newznab_site;
int newznab_sites_counter = 0;

void set_newznab_site(int index, const char* item, char* value) {
	if( index > (newznab_sites_counter - 1 ) ) {
		newznab_site = realloc( newznab_site, ++newznab_sites_counter * sizeof(newznab_site_t) );
	}

	if(strcmp(item, "active") == 0) {
		newznab_site[index].active = value;
	} else if(strcmp(item , "url") == 0) {
		newznab_site[index].url = value;
	} else if(strcmp(item , "apikey") == 0) {
		newznab_site[index].apikey = value;
	} else {
		printf("Unknown option ...\n");
		exit(-1);
	}
}


char *episode_status_names[] = {
		"Skipped",
		"Wanted",
		"Archived",
		"Ignored",
		"Snatched"
};


char *options[] = {

		/* WEB INTERFACE */
		"control_port", 			"8080",
		"web_dir", 					"./webui",
		"control_user", 			"minidragonfly",
		"control_password", 		"minidragonfly",

		/* DIRECTORIES */
		"chache_dir",				"./cache",
		"library_dir",				"./library",

		/* LOGGING */
		"log_level", 				"5",

		/* POST - PROCESS */
		"keep_original_files", 		"no",	/* Keep original files after they've been processed */
		"move_associated_files",	"no",	/* move srr/srt/sfv/etc files with the episode */
		"rename_episodes", 			"no",

		/* NZB BLACKHOLE */
		"black_hole_active",		"yes",
		"black_hole_dir",			"./nzb",

		/* NZBGET */
		"nzbget_active",			"yes",
		"nzbget_host", 				"127.0.0.1",
		"nzbget_port",				"6789",
		"nzbget_user",				"nzbget",
		"nzbget_password",			"tegbzn6789",
		"nzbget_category",			"Shows",

		/* TVDBAPI */
		"tvdb_apikey",				"",


		"ssl_certificate",			"",

		NULL,
};

unsigned char starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void trim_blanks (char *s)  {
    char *t=s, *u=s;
    while ((*t)&&((*t==' ')||(*t=='\t'))) t++;
    while (*t) *(u++)=*(t++);
    while ((u--!=s)&&((*u==' ')||(*u=='\t')||(*u=='\n')));
    *(++u)='\0';
    return;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if ( a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    //printf("%d\n", count);

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result =(char**) s_malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, "_");

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, "_");
        }
        assert(idx == count - 1);
        //printf("> %d\n", idx);
        *(result + idx) = 0;
    }

    return result;
}

char *read_config_file(const char *config, char **options /*char **opt_name, char **opt_val*/ ) {
	FILE *f;

	char line[1024];
	char *c;
	int i;

	int j;
	char *buffer;
	char *item;
	char *index;

	char **tokens;


	buffer =(char*) s_malloc(64);
	item =(char*) s_malloc(64);
	index =(char*) s_malloc(3);

	f = fopen(config, "r");
	if(!f)
		return "Can not open config file";

	while(fgets(line, 1024, f)) {

		if ((c=strchr(line,'#')))
			*c='\0';

		if ((c=strchr(line,'='))) {
			*(c++)='\0';
		   	trim_blanks (line);
		   	trim_blanks (c);

		   	i=0;
			while ((options[i])&&(strcmp(options[i],line))) i++;

			if (options[i]) {
				options[i + 1]=strdup(c);
			}

			if(starts_with( "newznab_", line )) {

				tokens = str_split(line, '_');
				if(tokens) {
					// 0      1   2
					//newznab_0_active
					set_newznab_site( atoi( *(tokens + 1) ), *(tokens + 2), strdup(c));
				}
				free(tokens);
			}
		}
	}

	fclose(f);
	return NULL;
}
