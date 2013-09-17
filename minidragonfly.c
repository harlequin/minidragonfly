/*
/ * The MIT License (MIT)
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
#include <string.h>
#include "mongoose.h"
#include <sqlite3.h>

#ifndef WIN32
#include <linux/limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#else

#endif

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif


#include "log.h"
#include "globalvars.h"
#include "sql.h"
#include "rpc.h"
#include "parser.h"
#include "version.h"
#include "utils.h"
#include "processor.h"

#if SQLITE_VERSION_NUMBER < 3005001
# warning "Your SQLite3 library appears to be too old!  Please use 3.5.1 or newer."
# define sqlite3_threadsafe() 0
#endif

#define MAX_CONFIG 40


char *configuration_file;
char *processor_location;

unsigned int daemonize = 0;


void print_backtrace()
{
#ifdef HAVE_BACKTRACE
	printf("Segmentation fault, tracing...\n");
	
	void *array[100];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, 100);
	strings = backtrace_symbols(array, size);

	// first trace to screen
	printf("Obtained %zd stack frames\n", size);
	for (i = 0; i < size; i++)
	{
		printf("%s\n", strings[i]);
	}

	// then trace to log
//	error("Segmentation fault, tracing...");
//	error("Obtained %zd stack frames", size);
//	for (i = 0; i < size; i++)
//	{
//		error("%s", strings[i]);
//	}

	free(strings);
#else
	printf("Segmentation fault");
#endif
}

#ifdef WIN32

void run_shutdown(int signum) {
	DPRINTF(E_WARN, L_GENERAL, "Shutdown Application\n", "");
	quitting = 1;
}

BOOL WINAPI run_shutdown_win(DWORD dwCtrlType) {

	switch(dwCtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		case CTRL_LOGOFF_EVENT:
		run_shutdown(1);
	}
	/* return true to avoid direct stop */
	return TRUE;
}

#else

/* Handler for the SIGTERM signal (kill) SIGINT is also handled */

static void sigterm(int sig) {
	signal(sig, SIG_IGN );
	DPRINTF(E_DEBUG, L_GENERAL, "received signal %d, good-bye\n", sig);
	quitting = 1;
}

#endif


#ifdef WIN32
LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e) {
	//DPRINTF(E_DEBUG, L_GENERAL, "Set windows handler...\n", "");
	print_backtrace();
	//printStack();
	return 0;// EXCEPTION_CONTINUE_SEARCH;
}
#endif

void signal_handler(int sig) {
#ifndef WIN32
	switch(sig) {
		case SIGSEGV:
		signal(SIGSEGV, SIG_DFL);
		print_backtrace();
		break;
	}
#endif
}

static int init(int argc, char ** argv) {

#ifdef WIN32
	SetUnhandledExceptionFilter(unhandled_handler);
	SetConsoleCtrlHandler(&run_shutdown_win, TRUE);
#else
	
	struct sigaction sa;
	
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigterm;
	if (sigaction(SIGTERM, &sa, NULL )) {
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set SIGTERM handler. EXITING.\n");
	}
	if (sigaction(SIGINT, &sa, NULL )) {
		DPRINTF(E_FATAL, L_GENERAL, "Failed to set SIGINT handler. EXITING.\n");
	}
	if (signal(SIGPIPE, SIG_IGN ) == SIG_ERR ) {
		DPRINTF(E_FATAL, L_GENERAL,
				"Failed to ignore SIGPIPE signals. EXITING.\n");
	}	

        signal(SIGSEGV, signal_handler);

        




#endif
	return 0;
}

static void print_version () {
	DPRINTF(E_WARN, L_GENERAL, "Starting " APPLICATION_NAME " version " MINIDRAGONFLY_VERSION "\n", "");
	DPRINTF(E_WARN, L_GENERAL, "** Development version\n", "");
	//DPRINTF(E_WARN, L_GENERAL, "**   %s\n", build_git_sha);
	//DPRINTF(E_WARN, L_GENERAL, "**   %s\n", build_git_time);
	//DPRINTF(E_WARN, L_GENERAL, "**   sqlite version %s\n", sqlite3_libversion());
	//DPRINTF(E_WARN, L_GENERAL, "**   mongoose version %s\n", mg_version());
}


static int open_db(void) {
	//char path[PATH_MAX];
	char path[MAX_PATH];
	int new_db = 0;
	//int ret;

	snprintf(path, sizeof(path), "%s/files.db", options[OPT_CACHE_DIR]);
	/*
	 if( access(path, F_OK) != 0 )
	 {
	 new_db = 1;
	 make_dir(db_path, S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
	 }
	 */

	if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL ) != SQLITE_OK)
	//if( sqlite3_open(path, &db) != SQLITE_OK )
	{
		DPRINTF(E_FATAL, L_GENERAL,
				"ERROR: Failed to open sqlite database!  Exiting...\n", "");
	}

	sqlite3_busy_timeout(db, 5000);
	sql_exec(db, "pragma page_size = 4096");
	sql_exec(db, "pragma journal_mode = OFF");
	sql_exec(db, "pragma synchronous = OFF;");
	sql_exec(db, "pragma default_cache_size = 8192;");

	return new_db;
}

void print_help() {

	printf(
		"Usage:\n"
		"\t%s\n"
		"\t\t[-h]        Print the help\n"
		"\t\t[-V]        Print version\n"
		"\t\t[-p file]   Post process file or directory\n"
		"\t\t[-c file]   config file\n",
		SERVER_NAME
	);
}

void command_line_processing(int argc, char **argv) {
	int i;
	//char *directory;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			DPRINTF(E_ERROR, L_GENERAL, "Unknown option: %s\n", argv[i]);
		} else if (strcmp(argv[i], "--help") == 0) {
			print_help();
			break;
		} else
			switch (argv[i][1]) {
			case 'V':
				printf("Version " MINIDRAGONFLY_VERSION "\n");
				exit(0);
				break;
			case 'h':
				print_help();
				exit(0);

			case 'p':
				processor_location = malloc(MAX_PATH);
				strncpyt(processor_location, argv[++i], MAX_PATH);
				break;

			case 'c':
				configuration_file = malloc(MAX_PATH);
				strncpyt(configuration_file, argv[++i], MAX_PATH);
				break;

			case 'd':
				daemonize = 1;
				break;
				
			default:
				DPRINTF(E_ERROR, L_GENERAL, "Unknown option: %s\n", argv[i]);
				break;

//			case 'l':
//				if (i + 1 < argc) {
//					char *name;
//					name = (char*) malloc(255);
//					//strncpyt(name, argv[++i], 255);
//					//search_for_series_by_name(name);
//					exit(0);
//				} else {
//					DPRINTF(E_ERROR, L_GENERAL,
//							"Option -%c takes one argument.\n", argv[i][1]);
//					exit(0);
//				}
//				break;
//
//			case 'a':
//				if (i + 1 < argc) {
//					DPRINTF(E_ERROR, L_GENERAL,
//							"Option -%c takes one argument.\n", argv[i][1]);
//				} else {
//					DPRINTF(E_ERROR, L_GENERAL,
//							"Option -%c takes one argument.\n", argv[i][1]);
//				}
//				break;
//			case 'V':
//				printf("Version " MINISCAN_VERSION "\n");
//				exit(0);
//				break;
//			default:
//				DPRINTF(E_ERROR, L_GENERAL, "Unknown option: %s\n", argv[i])
//				;
//				break;
			}
	}
}

int log_message_callback(const struct mg_connection *conn, const char *message) {
	DPRINTF(E_INFO, L_GENERAL, "mongoose error: %s\n", message);
	return 0;
}

static char *sdup(const char *str) {
  char *p;
  if ((p = (char *) malloc(strlen(str) + 1)) != NULL) {
    strcpy(p, str);
  }
  return p;
}

static void set_option( char **options, const char *name, const char *value ) {
	int i;

	for(i = 0; i < MAX_CONFIG - 3; i++) {
		if(options[i] == NULL) {
			options[i] = sdup(name);
			options[i+1] = sdup(value);
			options[i+2] = NULL;
			break;
		}
	}
}

static struct mg_context *start_webserver(void) {
	struct mg_context *context;
	struct mg_callbacks callbacks;
	char *url_rewrite;
	char **opt;
	int i;

	opt = malloc(MAX_CONFIG);
	url_rewrite = malloc(255);
	sprintf(url_rewrite, "/cache=%s", options[OPT_CACHE_DIR]);

	opt[0] = NULL;

	set_option( opt, "document_root", options[OPT_WEB_DIR] );
	set_option( opt, "listening_ports", options[OPT_CONTROL_PORT] );
	//set_option( options, "ssl_certificate", "ssl.pem" );
	set_option( opt, "url_rewrite_patterns", url_rewrite);

	//set_option( options, "authentication_domain", "miniscan.com");
	//set_option( options, "protect_uri","/*");
	//set_option( options, "global_auth_file", ".htpasswd");

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_request = begin_request_handler;
	callbacks.log_message = log_message_callback;

	//mg_modify_passwords_file( ".htpasswd", "mydomain.com", "root", "root" );

	context = mg_start(&callbacks, NULL,(const char **) opt);

	for(i = 0; opt[i] != NULL; i++)
		free(opt[i]);
	free(url_rewrite);

	return context;
}

#ifdef MAKE_SEGFAULT
void make_segfault() {
	char* N = NULL;
	strcpy(N, "");
}
#endif


int main(int argc, char **argv) {
	char *buf;
	int i, ret;
	struct mg_context *ctx;

	char *conf_locations[] = {
#ifndef WIN32
			"/usr/local/etc/minidragonfly.conf",
#endif
			"minidragonfly.conf",
			NULL
	};

	//shall be init when user want to save to file
	command_line_processing(argc, argv);
	

#ifdef HAVE_FORK

	if (daemonize) {
		int pid = fork();
		if (pid > 0)
			exit(0);
		else if (pid < 0) {
			printf("Unable to daemonize.\n");
			exit(-1);
		} else {
			setsid();
			for (i = getdtablesize(); i >= 0; i--)
				close(i);
			i = open("/dev/null", O_RDWR);
			dup(i);
			dup(i);
			signal(SIGCHLD, SIG_IGN);
			signal(SIGTSTP, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
		}
	}
#endif
	
	
	/*
	 * if configuration parameter was not set
	 * try to find it on the local system
	 * Unix:
	 * > /usr/local/etc/minidragonfly.conf
	 * > minidragonfly.conf
	 *
	 * Windows:
	 * > minidragonfly.conf
	 */
	if(!configuration_file) {
		buf = malloc(MAX_PATH);
		for ( i = 0; conf_locations[i] != NULL; i++) {
			strncpyt( buf , conf_locations[i], strlen(conf_locations[i]) + 1);
			printf(">> %s\n", buf);
			if ( access( buf, F_OK ) == 0) {
				configuration_file = s_malloc(MAX_PATH);
				strncpyt( configuration_file , conf_locations[i], strlen(conf_locations[i]) + 1);
				break;
			}
		}

		if ( !configuration_file ) {
			printf("No config file found for application.\n");
			exit(-1);
		} else {
			printf("Found config file %s\n", configuration_file);
		}
	}


	read_config_file( configuration_file, options );

#ifdef WIN32
	log_init("./minidragonfly.log", "DEBUG");
#else
	log_init("/var/log/minidragonfly.log", "DEBUG");
#endif



	for (i = 0; i < L_MAX; i++) {
		log_level[i] = atoi(options[OPT_LOG_LEVEL]);
	}



	//sanity check for config file
	/* check for tvdbapi key */
	if(strlen(options[OPT_TVDB_APIKEY]) == 0) {
		DPRINTF(E_ERROR, L_GENERAL, "No tvdbapi key found, exit application\n", "");
		return -1;
	}

	if (init(argc, argv))
		return 0;


	DPRINTF(E_INFO, L_GENERAL, "Create cache directory ...\n", "");

	//TODO REIMPLEMENT THIS MKDIR
	make_dir(options[OPT_CACHE_DIR], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	buf = malloc(255);
	sprintf(buf, "%s/hdclearart", options[OPT_CACHE_DIR]);
	make_dir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	sprintf(buf, "%s/tvthumb", options[OPT_CACHE_DIR]);
	make_dir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	free(buf);


	//db_path = s_malloc(strlen(options[OPT_CACHE_DIR]) + 1);
	//strncpyt(db_path, options[OPT_CACHE_DIR], strlen(options[OPT_CACHE_DIR]) + 1);


	print_version();
	if (!sqlite3_threadsafe()) {
		DPRINTF(E_ERROR, L_GENERAL, "SQLite library is not threadsafe!\n", "");
	}

	if (sqlite3_libversion_number() < 3005001) {
		DPRINTF(E_WARN, L_GENERAL,
				"SQLite library is old.  Please use version 3.5.1 or newer.\n", "");
	}


	DPRINTF(E_INFO, L_GENERAL, "Create library directory [%s].\n", options[OPT_LIBRARY_DIR]);
	make_dir(options[OPT_LIBRARY_DIR], 0);

	open_db();

	DPRINTF(E_DEBUG, L_GENERAL, "Database opened.\n", "");


	ret = db_upgrade(db);
	DPRINTF(E_DEBUG, L_GENERAL, "Current database version is %d.\n", ret);

	if( ret < 0 ) {
		DPRINTF(E_WARN, L_GENERAL, "new database have to be created.\n", "");
		create_database(db);
	} else if ( ret < 0 ) {
		DPRINTF(E_FATAL, L_GENERAL, "ERROR: Failed to update sqlite database [%d]!  Exiting...\n", ret);
		return 0;
	}


	DPRINTF(E_DEBUG, L_GENERAL, "Start to download exceptions.conf.\n","");
	load_exceptions();
	DPRINTF(E_DEBUG, L_GENERAL, "exceptions.conf loaded.\n","");

	//if(processor_location) {
	//	processor(processor_location);
	//	exit(0);
	//}

	DPRINTF(E_DEBUG, L_GENERAL, "Starting webserver.\n","");
	ctx = start_webserver( );
		
#ifdef MAKE_SEGFAULT
	make_segfault();
#endif

	/* main loop */
	quitting = 0;
	while (!quitting) {
		usleep(1000);
	}

	DPRINTF(E_DEBUG, L_GENERAL, "minidragonfly is shutting down ...\n", "");
	usleep(2 * 10000);
	DPRINTF(E_DEBUG, L_GENERAL, "Close webserver\n", "");
	mg_stop(ctx);
	DPRINTF(E_DEBUG, L_GENERAL, "Close database\n", "");
	sqlite3_close(db);
	DPRINTF(E_DEBUG, L_GENERAL, "Server will shutdown\n", "");

	return 0;
}
