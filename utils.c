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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sqlite3.h>
#include "mongoose.h"
#include <json/json.h>

#include <curl/curl.h>

#include "utils.h"
#include "log.h"
#include "sql.h"

#include "types.h"
#include "globalvars.h"
#include "url_parser.h"


#include "config.h"


#if !HAVE_STRNDUP
/* CAW: compliant version of strndup() */
char* strndup(const char* str, size_t n)
{
  if(str) {
    size_t len = strlen(str);
    size_t nn = json_min(len,n);
    char* s = (char*)malloc(sizeof(char) * (nn + 1));

    if(s) {
      memcpy(s, str, nn);
      s[nn] = '\0';
    }

    return s;
  }

  return NULL;
}
#endif


void *s_malloc(size_t size) {
	void *res;

	res = malloc(size);
	if(!res) {
		DPRINTF(E_FATAL, L_GENERAL, "Can't allocate %d bytes!\n", size);
	}
	memset(res, 0, size);
	return res;
}




/*inline */ void
strncpyt(char *dst, const char *src, size_t len)
{
	strncpy(dst, src, len);
	dst[len-1] = '\0';
}

#if HAVE_CURL
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}



int download(const char *url, struct memory_struct *chunk) {
	 CURL *curl_handle;
	 CURLcode res;
	 char *proxy;

	 proxy = malloc(255);

	 chunk->memory = malloc(1);
	 chunk->size = 0;

	 curl_global_init(CURL_GLOBAL_ALL);

	 curl_handle = curl_easy_init();
	 curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	 curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
	 curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, chunk);
	 curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	 curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);


	 proxy = getenv("HTTP_PROXY");
	 if(proxy)
		 curl_easy_setopt(curl_handle, CURLOPT_PROXY, "iwebalfde-1.tgn.trw.com:80");

	 res = curl_easy_perform(curl_handle);

	 if(res != CURLE_OK) {
		 DPRINTF(E_DEBUG, L_GENERAL, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	 }
	 else {
		DPRINTF(E_DEBUG, L_GENERAL, "%lu bytes retrieved\n", (long)chunk->size);
	 }

	 curl_easy_cleanup(curl_handle);

	 //if(chunk.memory)
	 //   free(chunk.memory);

	 curl_global_cleanup();

	 return 0;
}

#else

int download(const char *url, struct memory_struct *chunk) {
	char ebuf[100];
	char buffer[255];

	char smallbuffer[10];
	//char bigbuffer[2048];

	int bytes_read;
	int port;
	//int i;

	struct mg_connection *conn;
	struct parsed_url *uri;
	struct parsed_url *proxy_uri;
	int proxy_port;
	struct mg_request_info *request = NULL;
	char *proxy;

	unsigned int use_ssl = 0;
	//char *req;
	//int num = 0;

	if(chunk == NULL)
		return -1;

	chunk->size = 0;
	chunk->memory = malloc(255);

	uri = malloc(sizeof(struct parsed_url));
	uri = parse_url(url);

	if(strcmp(uri->scheme, "https") == 0) {
		port = 443;
		use_ssl = 1;
	} else {
		port = 80;
		use_ssl = 0;
	}

	if(uri->port != NULL)
		port = atoi(uri->port);



	DPRINTF(E_INFO, L_GENERAL, ">> %s\n", url);


	proxy = getenv("HTTP_PROXY");
	if( proxy != NULL ) {

		proxy_uri = malloc(sizeof(struct parsed_url));
		proxy_uri = parse_url( proxy );

		proxy_port = 80;
		if(proxy_uri->port != NULL)
			proxy_port = atoi(proxy_uri->port);


		DPRINTF(E_INFO, L_GENERAL, ">> Try to connect %s:%d via proxy\n", uri->host, port);
		DPRINTF(E_INFO, L_GENERAL, ">> Proxy %s:%d \n", proxy_uri->host, proxy_port);


		ebuf[0] = '\0';
		conn = (struct mg_connection*) mg_connect(proxy_uri->host, 80, 0, ebuf, sizeof(ebuf));
		if (conn) {
			DPRINTF(E_INFO, L_GENERAL, ">>>: CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n\r\n", uri->host, port, uri->host, port);
			if( mg_printf(conn, "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n\r\n", uri->host, port, uri->host, port) <= 0 ) {
				//DPRINTF(E_INFO, L_GENERAL, "Download: %s %s\n", ebuf, GetLastError());
				return -1;
			} else {


				char response[1024];
				char c[1];
				strcpy(response, "");
				while ( (bytes_read = mg_read(conn, c, 1)) > 0) {
					strcat(response, c);


					DPRINTF(E_INFO, L_GENERAL, "<< %s\n", response);
					if(strcmp(c, "\r") == 0)
						break;
				}



				DPRINTF(E_INFO, L_GENERAL, ">> NOW SEND REQUEST\n","");
				DPRINTF(E_INFO, L_GENERAL, ">>>: GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url, uri->host);
				if( mg_printf(conn, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url, uri->host) <= 0 ) {
					DPRINTF(E_INFO, L_GENERAL, "Download: %s\n", ebuf);
					return -1;
				} else {
//					while ( (bytes_read = mg_read(conn, smallbuffer, 10)) > 0) {
//						DPRINTF(E_INFO, L_GENERAL, "< %s\n", smallbuffer);
//					}
				}
			}
		} else {
			//DPRINTF(E_INFO, L_GENERAL, "> %s\n", GetLastError());
			return -1;
		}
//
//			conn = mg_download( , proxy_port, 0, ebuf, ,
//				"GET %s HTTP/1.1\r\n"
//				"User-Agent: curl/7.29.0\r\n"
//				"Host: %s\r\n"
//				"Accept: */*\r\n"
//				"Proxy-Connection: Keep-Alive\r\n\r\n",
//				url, uri->host
//			);

	} else {
		DPRINTF(E_INFO, L_GENERAL, "Download from %s on port %d\n", uri->host, port);
		conn = mg_download(uri->host, port, use_ssl, ebuf, sizeof(ebuf), "GET %s HTTP/1.0\r\nHost: %s:%d\r\n\r\n", url, uri->host, port );
	}

	//request = mg_get_request_info(conn);

	if(NULL == conn) {
		DPRINTF(E_ERROR, L_GENERAL, "Connection closed [%s]\n", ebuf);
		return -1;
	}

	while( (bytes_read = mg_read(conn, buffer, 255)) > 0 ) {

		chunk->memory = realloc( chunk->memory, chunk->size + bytes_read +1 );
		if( chunk->memory == NULL ) {
			printf("not enough memory\n");
			exit(1);
		}

		memcpy(&(chunk->memory[chunk->size]), buffer, bytes_read);
		chunk->size += bytes_read;
		chunk->memory[chunk->size] = 0;
	}



	return 0;
}

#endif



//itunes
void download_itunes_covers(int id) {
	char **result;
	int cols, rows;
	int i;
	char *sql;
	char *search_url = "http://ax.itunes.apple.com/WebObjects/MZStoreServices.woa/wa/wsSearch?term=%s%%20Season%%20%s&media=tvShow&entity=tvSeason&attribute=tvSeasonTerm&country=us";
	struct memory_struct *chunk;
	char *uri;
	json_object *obj;
	array_list *list;
	char *destination_path;
	FILE *f;

	chunk = malloc(sizeof(struct memory_struct));


	sql = sqlite3_mprintf("SELECT DISTINCT tv_serie.title, tv_episode.season FROM tv_serie, tv_episode WHERE season > 0 AND tv_series_id = '%d' AND tv_serie.id = tv_episode.tv_series_id", id);
	if ((sql_get_table(db, sql, &result, &rows, &cols) == SQLITE_OK) && rows) {
		for (i = cols; i < rows * cols + cols; i += cols) {

			uri = malloc(1024);
			sprintf(uri, search_url, str_replace(result[i], " ", "%20") ,result[i+1]);
			download(uri , chunk);

			if(chunk->size > 0) {
				obj = json_tokener_parse(chunk->memory);
				if(obj) {
					obj = json_object_object_get(obj, "results");
					list = json_object_get_array(obj);
					if(list->length > 0) {
						obj = json_object_object_get(list->array[0], "artworkUrl100");

						destination_path = malloc(255);
						download(str_replace( json_object_get_string( obj ), "100x100", "200x200"), chunk);

						sprintf(destination_path, "%s/%s/%d-%s.png", options[OPT_CACHE_DIR] , "itunes", id, result[i+1]);
						DPRINTF(E_DEBUG, L_GENERAL, "==> TO fanart %s\n", destination_path);
						if(chunk->size > 0 ) {
							f = fopen(destination_path, "wb");
							fwrite( chunk->memory, chunk->size ,1 ,f );
							fclose(f);
						} else {
							DPRINTF(E_ERROR, L_GENERAL, "Can not download image!\n");
						}
					}
				}
			}
		}
	}
}


//hdclearart
//tvthumb

void download_fanart_clearart(int id, const char *type) {
	struct memory_struct *chunk;
	struct lh_entry *entry;

	char* uri;
	char *destination_path;

	FILE *f;

	json_object *obj;
	array_list *list;


	chunk = malloc(sizeof(struct memory_struct));
	uri = malloc(1024);
	destination_path = malloc(255);


	sprintf(uri, "http://api.fanart.tv/webservice/series/52fdc988539881c2ac1f3852ddfbfc5f/%d/json/%s/1/1", id, type);

	DPRINTF(E_DEBUG, L_GENERAL, "%s\n", uri);

	download(uri , chunk);

	if(chunk->size > 0) {
		DPRINTF(E_DEBUG, L_GENERAL, "Start parsing the json response %s\n", chunk->memory);
		obj = json_tokener_parse(chunk->memory);

		if(obj) {

			entry = json_object_get_object(obj)->head;
			DPRINTF(E_DEBUG, L_GENERAL, "%s\n", (char*)entry->k);
			obj = json_object_object_get(obj, (char*)entry->k);
			obj = json_object_object_get(obj, type);
			list = json_object_get_array(obj);
			if(list->length > 0) {
				obj = json_object_object_get(list->array[0], "url");

				DPRINTF(E_DEBUG, L_GENERAL, "Download fanart %s\n", json_object_get_string( obj ));

				download(json_object_get_string( obj ) , chunk);


				sprintf(destination_path, "%s/%s/%d.png", options[OPT_CACHE_DIR] , type, id);
				DPRINTF(E_DEBUG, L_GENERAL, "==> TO fanart %s\n", destination_path);
				if(chunk->size > 0 ) {
					f = fopen(destination_path, "wb");
					fwrite( chunk->memory, chunk->size ,1 ,f );
					fclose(f);
				} else {
					DPRINTF(E_ERROR, L_GENERAL, "Can not download image!\n");
				}
			}
		}else {
			DPRINTF(E_ERROR, L_GENERAL, "Can not download image!\n");
		}
	}


	//clean memory
	free(uri);
	free(destination_path);
	if(chunk->memory)
		free(chunk->memory);
}







int create_database(sqlite3 *db) {
	char *tv_episodes_table = "CREATE TABLE " \
			"tv_episode(id INTEGER PRIMARY KEY, tv_series_id NUMERIC, season NUMERIC, episode NUMERIC, title TEXT COLLATE NOCASE, status NUMERIC, language TEXT, nzb TEXT, snatched_quality NUMERIC, location TEXT, watched NUMERIC)";

	char *tv_serie_table = "CREATE TABLE tv_serie(id NUMERIC, title TEXT COLLATE NOCASE, quality NUMERIC, match NUMERIC, location TEXT, overview TEXT, network TEXT)";

	char *cache_table = "CREATE TABLE cache(tv_series_id NUMERIC, subject TEXT, id NUMERIC, groupid NUMERIC)";

	char *rss_feed_table = "CREATE TABLE rss_feed (id INTEGER PRIMARY KEY AUTOINCREMENT, uri TEXT, last_update NUMERIC)";

	char *recent_news_table = "CREATE TABLE recent_news (id INTEGER PRIMARY KEY AUTOINCREMENT, tv_show_id NUMERIC, comment TEXT)";

	char *scene_names_table = "CREATE TABLE scene_name (id INTEGER PRIMARY KEY AUTOINCREMENT, show_name TEXT, scene_name)";

	char *exception_table = "CREATE TABLE exception (id INTEGER PRIMARY KEY AUTOINCREMENT, tvdb_id NUMERIC, name TEXT COLLATE NOCASE)";

	char *releases_table = "CREATE TABLE releases (id INTEGER PRIMARY KEY AUTOINCREMENT, tv_episode_id NUMERIC, name TEXT, link TEXT)";

	sql_exec(db, tv_episodes_table);
	sql_exec(db, tv_serie_table);
	sql_exec(db, cache_table);
	sql_exec(db, rss_feed_table);
	sql_exec(db, recent_news_table);
	sql_exec(db, scene_names_table);
	sql_exec(db, exception_table);
	sql_exec(db, releases_table);
	sql_exec(db, "pragma user_version = 5;");
	return 0;
}


/* Code basically stolen from busybox */
int make_dir(char * path, mode_t mode) {
	char * s = path;
	char c;
	struct stat st;

	do {
		c = '\0';

		/* Bypass leading non-'/'s and then subsequent '/'s. */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;     /* Save the current char */
				*s = '\0';     /* and replace it with nul. */
				break;
			}
			++s;
		}

		if(access(path, 0/*F_OK*/) != 0) {
		
			//if (mkdir(path /*, mode*/) < 0) {


			if (mkdir(path , mode) < 0) {


				/* If we failed for any other reason than the directory
				 * already exists, output a diagnostic and return -1.*/
				if (errno != EEXIST || (stat(path, &st) < 0 || !S_ISDIR(st.st_mode))) {
					DPRINTF(E_WARN, L_GENERAL, "make_dir: cannot create directory '%s'\n", path);
					if (c)
						*s = c;
					return -1;
				}
			}
		}

		if (!c)
			return 0;

		/* Remove any inserted nul from the path. */
		*s = c;

	} while (1);

	/*added to remove warnings */
	return 0;
}



char* str_replace(const char *strbuf, const char *strold, const char *strnew) {
	char *strret, *p = NULL;
	char *posnews, *posold;
	size_t szold = strlen(strold);
	size_t sznew = strlen(strnew);
	size_t n = 1;

	if (!strbuf)
		return NULL ;
	if (!strold || !strnew || !(p = strstr(strbuf, strold)))
		return strdup(strbuf);

	while (n > 0) {
		if (!(p = strstr(p + 1, strold)))
			break;
		n++;
	}

	strret = (char*) malloc(strlen(strbuf) - (n * szold) + (n * sznew) + 1);

	p = strstr(strbuf, strold);

	strncpy(strret, strbuf, (p - strbuf));
	strret[p - strbuf] = 0;
	posold = p + szold;
	posnews = strret + (p - strbuf);
	strcpy(posnews, strnew);
	posnews += sznew;

	while (n > 0) {
		if (!(p = strstr(posold, strold)))
			break;
		strncpy(posnews, posold, p - posold);
		posnews[p - posold] = 0;
		posnews += (p - posold);
		strcpy(posnews, strnew);
		posnews += sznew;
		posold = p + szold;
	}

	strcpy(posnews, posold);
	return strret;
}



void base64_encode(const unsigned char *src, int src_len, char *dst) {
	static const char *b64 =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i, j, a, b, c;

	for (i = j = 0; i < src_len; i += 3) {
		a = src[i];
		b = i + 1 >= src_len ? 0 : src[i + 1];
		c = i + 2 >= src_len ? 0 : src[i + 2];

		dst[j++] = b64[a >> 2];
		dst[j++] = b64[((a & 3) << 4) | (b >> 4)];
		if (i + 1 < src_len) {
			dst[j++] = b64[(b & 15) << 2 | (c >> 6)];
		}
		if (i + 2 < src_len) {
			dst[j++] = b64[c & 63];
		}
	}
	while (j % 4 != 0) {
		dst[j++] = '=';
	}
	dst[j++] = '\0';
}


void nzbget_appendurl ( const char *filename, const char *category, int priority, int add_to_top, const char *url ) {

	char ebuf[100];
	//char reply[1000];
	char buf[1024];
	char *post_data;
	int use_ssl = 0;
	int len;
	struct mg_connection *conn;
	char *auth;
	char *base64auth;
	const char *post_template = "{\"method\":\"appendurl\",\"params\":[\"%s\",\"%s\",%d,false,\"%s\"]}";

	int size = strlen(post_template) + strlen(filename) + strlen(category) + 1 + strlen(url);

	auth = malloc(127);
	base64auth = malloc(255);
	post_data = malloc( size );

	strcpy(auth, "nzbget:");
	strcat(auth, options[OPT_NZBGET_PASSWORD]);

	base64_encode( (unsigned char*) auth, strlen(auth), base64auth);

	snprintf(post_data, size, post_template, filename, category, 0, url);

	DPRINTF(E_INFO, L_GENERAL, "Post data to nzbget %s\n", post_data);

	conn = mg_download(options[OPT_NZBGET_HOST], atoi(options[OPT_NZBGET_PORT]), use_ssl, ebuf, sizeof(ebuf),
			"POST /jsonrpc HTTP/1.0\r\n"
	        "Host: %s\r\n"
	        "Content-Length: %d\r\n"
			"Authorization: Basic %s\r\n"
	        "\r\n"
	        "%s",
	        options[OPT_NZBGET_HOST], strlen(post_data), base64auth, post_data);

	//Todo: check results if the adding was successfull
	if (conn != NULL) {
	  len = mg_read(conn, buf, sizeof(buf));
	  DPRINTF(E_INFO, L_GENERAL, "Answer from nzbget %s [%d]\n", buf, len);
	  mg_close_connection(conn);
	} else {
		DPRINTF(E_INFO, L_GENERAL, "nzbget post ERROR %s\n", ebuf);
	}

	if(auth)
		free(auth);
	if(base64auth)
		free(base64auth);
	if(post_data)
		free(post_data);
}




int ends_with(const char * haystack, const char * needle) {
	const char * end;
	int nlen = strlen(needle);
	int hlen = strlen(haystack);

	if( nlen > hlen )
		return 0;
 	end = haystack + hlen - nlen;

	return (strcasecmp(end, needle) ? 0 : 1);
}

int is_video(const char * file) {
	return (ends_with(file, ".mpg") || ends_with(file, ".mpeg")  ||
		ends_with(file, ".avi") || ends_with(file, ".divx")  ||
		ends_with(file, ".asf") || ends_with(file, ".wmv")   ||
		ends_with(file, ".mp4") || ends_with(file, ".m4v")   ||
		ends_with(file, ".mts") || ends_with(file, ".m2ts")  ||
		ends_with(file, ".m2t") || ends_with(file, ".mkv")   ||
		ends_with(file, ".vob") || ends_with(file, ".ts")    ||
		ends_with(file, ".flv") || ends_with(file, ".xvid")  ||
		ends_with(file, ".mov") || ends_with(file, ".3gp"));
}



unsigned int exception_counter;

static void parse_exception_line(char *str) {
	char *sql;
	int id = 0;

	str = strtok(strdup(str), ":,");
	id = atoi(str);

	while( (str = strtok(NULL, ",")) ){
		str = str_replace(str, "'", "");
		trim_blanks(str);

		DPRINTF(E_DEBUG, L_GENERAL, "Save exception '%s' for '%d'\n", str, id);

		sql = sqlite3_mprintf("INSERT INTO exception ('tvdb_id', 'name') VALUES (%d, '%s')", id, str);
		exception_counter++;
		sql_exec(db, sql);
	}
}



/**
 * Load exceptions
 * 1. User exceptions from "exceptions.conf"
 * 2. Download from official website
 * TODO: add failsafe when first item is not a id
 * TODO: add failsafe if the complete pointers are not null
 * TODO: add more return codes in combination with parse_exception line
 */
void load_exceptions() {
	char *token;
	char *t;
	FILE *f;
	char line[128];
	char *sql;
	struct memory_struct *chunk;

	sql = sqlite3_mprintf("DELETE FROM exception");
	sql_exec(db, sql);

	f = fopen( "exceptions.conf", "r" );
	if( f != NULL ) {
		while( fgets(line, sizeof line, f) != NULL) {
			parse_exception_line(line);
		}
		fclose(f);
	}

	DPRINTF(E_INFO, L_GENERAL, "Download exceptions from %s.\n", EXCEPTIONS_SITE);
	chunk = malloc(sizeof(struct memory_struct));

	download(EXCEPTIONS_SITE, chunk);

	if(chunk->size > 0) {
		t = strtok(chunk->memory, "\n");
		do {
			parse_exception_line(t);
			/*
			 * at this point we have to work with token
			 * length + 1 to get the rest of the string
			 */
		} while ( (t = strtok( t + strlen(t) + 1, "\n" )) );

	}

	DPRINTF(E_INFO, L_GENERAL, "Loaded %d exceptions into database.\n", exception_counter);

	chunk = malloc(sizeof(struct memory_struct));
	download("https://api.github.com/repos/harlequin/minidragonfly/compare/5df40aa24d5d805431903b0b2473d9f5ddaccf54...master", chunk);
	if(chunk->size > 0) {
		DPRINTF(E_INFO, L_GENERAL, "data: %s\n", chunk->memory);
	}


}
//"ahead_by": 4,
//"behind_by": 0,
