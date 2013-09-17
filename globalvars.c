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

#include "globalvars.h"
#include "log.h"

volatile short int quitting = 0;
//char *db_path = ".";
sqlite3 *db;


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
		"tvdb_apikey",				"9DAF49C96CBF8DAC",

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


char *read_config_file(const char *config, char **options /*char **opt_name, char **opt_val*/ ) {
	FILE *f;

	char line[1024];
	char *c;
	int i;

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
				printf("Options [%d] %s = %s\n",i, options[i], options[i+1]);
			}
		}
	}

	fclose(f);
	return NULL;
}
