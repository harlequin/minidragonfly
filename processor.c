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

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#ifdef _MSC_VER
#else
#include <dirent.h>
#endif
#include <pcre.h>

#include "globalvars.h"
#include "config.h"
#include "parser.h"
#include "processor.h"
#include "log.h"
#include "sql.h"
#include "types.h"
#include "utils.h"

#define LOG(x,y, z, ...)	DPRINTF(x, L_PROCESSOR, y, z);

int process_file ( char *data );


#ifndef WIN32
#define GetLastError() 0
#endif

/*
//  returns 1 if str ends with suffix
//int str_ends_with(const char * str, const char * suffix) {
//
//  if( str == NULL || suffix == NULL )
//    return 0;
//
//  size_t str_len = strlen(str);
//  size_t suffix_len = strlen(suffix);
//
//  if(suffix_len > str_len)
//    return 0;
//
//  return 0 == strncmp( str + str_len - suffix_len, suffix, suffix_len );
//}
*/

#ifdef _MSC_VER

int processor (const char *str) {
	HANDLE handle;
	WIN32_FIND_DATA data;

	char *buf;

	buf = malloc(strlen(str) + 2);
	sprintf(buf, "%s/*", str);

	handle = FindFirstFile(buf, &data);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("Error ... folder not found %s\n", str);
	}

	do {
		if( strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
			continue;

		if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			buf = malloc(strlen(str) + strlen(data.cFileName) + 3);
			sprintf(buf, "%s/%s", str, data.cFileName);
			processor(buf);
		} else {
			process_file(strdup(data.cFileName));
		}
	} while (FindNextFile(handle, &data) != 0);
	FindClose(handle);

	return 0;
}

#else

int processor ( const char *data ) {
		DIR *dir;
		struct dirent *dent;
		struct stat st;
		//char *buf;
		char fn[PATH_MAX];
		int len = strlen(data);

		strcpy(fn , data);
		fn[len++] = '/';

		if(!(dir = opendir(data))) {
			printf("Error ... folder not found %s\n", data);
			return 1;
		}

		while ((dent = readdir(dir))) {

			if( !strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;

			strncpyt(fn + len, dent->d_name, PATH_MAX - len + 3);

			if(stat(fn, &st) == -1) {
				LOG(E_ERROR, "Post-process can not stat file %s\n", fn);
				//printf("Error ... can not stat %s\n", fn);
				continue;
			}

			if(S_ISDIR(st.st_mode)) {
				processor( fn );
			} else {
				process_file( strdup(fn) );
			}
		}

		if(dir) closedir(dir);

		return 0;
}

#endif


int get_id (const char *str) {
	//return sql_get_int_field(db, "select tv_serie.id from tv_serie LEFT OUTER JOIN exception ON tv_serie.id = tvdb_id WHERE (tv_serie.title = '%s' or exception.name = '%s')", str, str);
	return sql_get_int_field(db, "select tv_serie.id from tv_serie LEFT OUTER JOIN exception ON tv_serie.id = tvdb_id WHERE (tv_serie.title = '%s' COLLATE NOCASE or exception.name = '%s' COLLATE NOCASE)", str, str);
}

//#define GET_SHOW_ID(x) 
//		sql_get_int_field(db, "select tv_serie.id from tv_serie LEFT OUTER JOIN exception ON tv_serie.id = tvdb_id WHERE (tv_serie.title = '%s' COLLATE NOCASE or exception.name = '%s' COLLATE NOCASE)", x, x);

/* Missing / and \ : */
char *path_escape_chars[] = {
	"*", "?", "\"", "<", ">", "|"
};

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))

void move_episode(char *tv_name, char *ep_name, parsed_episode_t *episode, char *str) {

	FILE *source;
	FILE *destination;
	size_t size;
	char buf[BUFSIZ];
	char *buffer;
	char *token = strrchr(str, '.');
	int i;

	buffer = malloc(FILENAME_MAX);

	// [Library]/[Show Name]/[Season 0x]/ [Show Name] - S[Season]E[Episode] - [EP Name]
	sprintf(buffer, "%s/%s/Season %s",
			options[OPT_LIBRARY_DIR],
			tv_name,
			episode->season_num
			);

	make_dir(buffer, 0);
	LOG(E_INFO, "Directory: %s\n", 	buffer);



	sprintf(buffer, "%s/%s - S%sE%s - %s - %s%s",
			buffer,
			tv_name,
			episode->season_num,
			episode->ep_num,
			ep_name,
			quality_names[episode->quality].name,
			token);


	LOG(E_INFO, "Location: %s\n", 	buffer);



	for (i = 0; i < ARRAY_SIZE(path_escape_chars); i++) {
		buffer = (char*) str_replace(buffer, path_escape_chars[i], "");
	}

	//buffer = escape_tag(buffer, 1);

	source = fopen(str, "rb");
	destination = fopen(buffer, "wb");

	if(!source) {
		LOG(E_ERROR, "Can't read source files '%s' abort operation.\n", str);
		return;
	}

	if(!destination) {
		LOG(E_ERROR, "Can't open destination file '%s' abort operation.\n", buffer);
		return;
	}

	while((size = fread(buf,1, BUFSIZ, source))) {
		fwrite(buf, 1, size, destination);
	}

	fclose(source);
	fclose(destination);

	LOG(E_INFO, "Copy operation successfull for '%s'.\n", buffer);

	//delete source file
	if (remove(str) == -1) {
		LOG(E_ERROR, "Error in deleting file %d.\n", GetLastError());
	}


//	int file_count = 0;
//	if(file_count == 0) {
//		if (remove( dirname(strdup(str)) ) == -1 ) {
//			LOG(E_DEBUG, "Directory '%s' can't be deleted\n", "");
//		}
//	}





	strncpyt(str, buffer, strlen(buffer) + 1);
}



void addtitle( parsed_episode_t *episode, int id, char *location ) {
	char *sql;
	char **result;
	int cols, rows, /*i,*/ ret;

	int quality = 0;

	LOG(E_DEBUG, "Adding '%s' to database\n", location);

	sql = sqlite3_mprintf("SELECT tv_episode.id, tv_serie.title, tv_episode.title, tv_episode.snatched_quality FROM tv_episode, tv_serie WHERE tv_episode.tv_series_id = tv_serie.id AND tv_episode.tv_series_id = %d AND tv_episode.season = %d AND tv_episode.episode = %d", id, atoi(episode->season_num), atoi(episode->ep_num));
	if( (sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows ) {

		if ( cols > 0 ) {
			LOG(E_DEBUG, "Found a valid database entry, try to get quality for id '%s' with quality %s\n", result[cols], result[cols+3]);

			quality = strqual(episode->extra_info);
			LOG(E_INFO, "Quality %d\n", quality);
			episode->quality = quality;

			//found entry ... look now to the quality
			if( result[cols + 3 ] != NULL) {

				if(atoi(result[cols+3]) <= quality) {
					//insert

					if(strcmp(options[OPT_KEEP_ORIGINAL_FILES], "yes") == 0) {
						move_episode(result[cols+1], result[cols+2], episode, location);
					}

					sql = sqlite3_mprintf("UPDATE tv_episode SET snatched_quality = %d, location = '%s', status = %d WHERE id = %s", strqual(episode->extra_info), location, EPISODE_STATUS_ARCHIVED, result[cols]);
					ret = sql_exec(db, sql);
					if ( ret != SQLITE_OK )
						DPRINTF(E_ERROR, L_GENERAL, "ERROR insert new episode ...", "");

				}
			} else {


				DPRINTF(E_ERROR, L_GENERAL, "Database has no item with quality\n", "");
				DPRINTF(E_ERROR, L_GENERAL, "%s\n", result[cols+1]);
				DPRINTF(E_ERROR, L_GENERAL, "%s\n", result[cols+2]);
				DPRINTF(E_ERROR, L_GENERAL, "%s\n", "");
				DPRINTF(E_ERROR, L_GENERAL, "%s\n", location);

				if(strcmp(options[OPT_KEEP_ORIGINAL_FILES], "yes") == 0) {
					move_episode(result[cols+1], result[cols+2], episode, location);
				}

				sql = sqlite3_mprintf("UPDATE tv_episode SET snatched_quality = %d, location = '%s', status = %d WHERE id = %s", strqual(episode->extra_info), location, EPISODE_STATUS_ARCHIVED, result[cols]);
				ret = sql_exec(db, sql);
				if ( ret != SQLITE_OK )
					DPRINTF(E_ERROR, L_GENERAL, "ERROR insert new episode ...", "");


			}
		}
	}
}


int process_file ( char *data ) {
	parsed_episode_t *episode;
	int id;

	char *dir;
	char *fname;
#if defined _MSC_VER
	char *drive;
	char *ext;
#endif
	episode = malloc(sizeof(parsed_episode_t));

	dir  = malloc(255);
	fname = malloc(255);
#if defined _MSC_VER	
	drive = malloc(255);	
	ext = malloc(255);
#endif


	if(is_video(data)) {

#if defined _MSC_VER
	_splitpath(data, drive, dir, fname, ext);
#else
	strncpyt(fname, (const char*) basename(strdup(data)), 255);
	strncpyt(dir, (const char*) basename( dirname(strdup(data))), 255);
#endif

		LOG(E_INFO, "File: %s\n", fname);

		episode = parse_title(fname);

		//LOG(E_INFO, "Episode parsed ...\n","");

		if(episode != NULL) {
			if(episode->series_name != NULL && episode->season_num != NULL && episode->ep_num != NULL) {

				LOG(E_INFO, "Episode parsed %s\n",episode->series_name);
				id = get_id( episode->series_name );
				if( id > 0 ) {
					addtitle( episode, id, data );

				} else {

					LOG(E_INFO, "Episode parsed 2...\n","");

					episode = parse_title( dir );
					if(episode != NULL) {
						if(episode->series_name != NULL) {
							if( (id = get_id( episode->series_name )) > 0 ) {
								addtitle( episode, id, data);
							}
						}
					} else {
						LOG(E_INFO, "Parse level 2 was null for %s\n", dir);
					}
				}
			}
		}
	}
	return PROCESSOR_RETURN_OK;
}
