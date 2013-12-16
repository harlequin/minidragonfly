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
#include <string.h>

//#ifdef WIN32
//#include <io.h>
//#else
//#include <unistd.h>
//#endif


#include "sql.h"
#include "globalvars.h"
#include "log.h"

#include <sqlite3.h>

#define sleep(x) usleep(x*1000);

int
sql_exec(sqlite3 *db, const char *fmt, ...)
{
	int ret;
	char *errMsg = NULL;
	char *sql;
	va_list ap;

	va_start(ap, fmt);

	sql = sqlite3_mprintf(fmt, ap);
	//DPRINTF(E_DEBUG, L_DB_SQL, "SQL: %s\n", sql);
	ret = sqlite3_exec(db, sql, 0, 0, &errMsg);
	if( ret != SQLITE_OK )
	{
		DPRINTF(E_ERROR, L_DB_SQL, "SQL ERROR %d [%s]\n%s\n", ret, errMsg, sql);
		if (errMsg)
			sqlite3_free(errMsg);
	}
	sqlite3_free(sql);

	return ret;
}


int
sql_get_table(sqlite3 *db, const char *sql, char ***pazResult, int *pnRow, int *pnColumn)
{
	int ret;
	char *errMsg = NULL;
	//DPRINTF(E_DEBUG, L_DB_SQL, "SQL: %s\n", sql);

	ret = sqlite3_get_table(db, sql, pazResult, pnRow, pnColumn, &errMsg);
	if( ret != SQLITE_OK )
	{
		DPRINTF(E_ERROR, L_DB_SQL, "SQL ERROR %d [%s]\n\t%s\n", ret, errMsg, sql);
		if (errMsg)
			sqlite3_free(errMsg);
	}
	//DPRINTF(E_DEBUG, L_DB_SQL, "sql query was good %d\n", ret);
	return ret;
}

int
sql_get_int_field(sqlite3 *db, const char *fmt, ...)
{
	va_list		ap;
	int		counter, result;
	char		*sql;
	int		ret;
	sqlite3_stmt	*stmt;

	va_start(ap, fmt);

	sql = sqlite3_vmprintf(fmt, ap);

	DPRINTF(E_DEBUG, L_DB_SQL, "sql: %s\n", sql);

	switch (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
	{
		case SQLITE_OK:
			break;
		default:
			DPRINTF(E_ERROR, L_DB_SQL, "prepare failed: %s\n%s\n", sqlite3_errmsg(db), sql);
			sqlite3_free(sql);
			return -1;
	}

	for (counter = 0;
	     ((result = sqlite3_step(stmt)) == SQLITE_BUSY || result == SQLITE_LOCKED) && counter < 2;
	     counter++) {
		 /* While SQLITE_BUSY has a built in timeout,
		    SQLITE_LOCKED does not, so sleep */
		 if (result == SQLITE_LOCKED)
		 	sleep(1);
	}

	switch (result)
	{
		case SQLITE_DONE:
			/* no rows returned */
			ret = 0;
			break;
		case SQLITE_ROW:
			if (sqlite3_column_type(stmt, 0) == SQLITE_NULL)
			{
				ret = 0;
				break;
			}
			ret = sqlite3_column_int(stmt, 0);
			break;
		default:
			//DPRINTF(E_WARN, L_DB_SQL, "%s: step failed: %s\n%s\n", __func__, sqlite3_errmsg(db), sql);
			ret = -1;
			break;
 	}

	sqlite3_free(sql);
	sqlite3_finalize(stmt);
	return ret;
}

char *
sql_get_text_field(sqlite3 *db, const char *fmt, ...)
{
	va_list         ap;
	int             counter, result, len;
	char            *sql;
	char            *str;
	sqlite3_stmt    *stmt;

	va_start(ap, fmt);

	if (db == NULL)
	{
		DPRINTF(E_WARN, L_DB_SQL, "db is NULL\n");
		return NULL;
	}

	sql = sqlite3_vmprintf(fmt, ap);

	DPRINTF(E_DEBUG, L_DB_SQL, "sql: %s\n", sql);

	switch (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
	{
		case SQLITE_OK:
			break;
		default:
			DPRINTF(E_ERROR, L_DB_SQL, "prepare failed: %s\n%s\n", sqlite3_errmsg(db), sql);
			sqlite3_free(sql);
			return NULL;
	}
	sqlite3_free(sql);

	for (counter = 0;
	     ((result = sqlite3_step(stmt)) == SQLITE_BUSY || result == SQLITE_LOCKED) && counter < 2;
	     counter++)
	{
		/* While SQLITE_BUSY has a built in timeout,
		 * SQLITE_LOCKED does not, so sleep */
		if (result == SQLITE_LOCKED)
			sleep(1);
	}

	switch (result)
	{
		case SQLITE_DONE:
			/* no rows returned */
			str = NULL;
			break;

		case SQLITE_ROW:
			if (sqlite3_column_type(stmt, 0) == SQLITE_NULL)
			{
				str = NULL;
				break;
			}

			len = sqlite3_column_bytes(stmt, 0);
			if ((str = sqlite3_malloc(len + 1)) == NULL)
			{
				DPRINTF(E_ERROR, L_DB_SQL, "malloc failed\n");
				break;
			}

			strncpy(str, (char *)sqlite3_column_text(stmt, 0), len + 1);
			break;

		default:
			DPRINTF(E_WARN, L_DB_SQL, "SQL step failed: %s\n", sqlite3_errmsg(db));
			str = NULL;
			break;
	}

	sqlite3_finalize(stmt);
	return str;
}











int
db_upgrade(sqlite3 *db)
{

	int db_vers;
	int ret;
	int result;


	db_vers = sql_get_int_field(db, "PRAGMA user_version");

	DPRINTF(E_WARN, L_DB_SQL, "Current database version %d.\n", db_vers);

	if (db_vers < 1)
		return -1;

	if (db_vers == DB_VERSION)
		return 0;

	if (db_vers > DB_VERSION)
		return -2;


	if (db_vers < 2) {
		DPRINTF(E_WARN, L_DB_SQL, "Updating DB version to v%d.\n", 2);
		ret = sql_exec(db, "ALTER TABLE tv_episode ADD snatched_quality NUMERIC");
		if ( ret != SQLITE_OK )
			result = 2;

		ret = sql_exec(db, "ALTER TABLE tv_episode ADD location TEXT");
		if ( ret != SQLITE_OK )
			result = 2;

	}

	if (db_vers < 3) {
		DPRINTF(E_WARN, L_DB_SQL, "Updating DB version to v%d.\n", 3);
		ret = sql_exec(db, "ALTER TABLE tv_serie ADD overview TEXT");
		if ( ret != SQLITE_OK )
			result = 3;
	}

	if( db_vers < 4) {
		DPRINTF(E_WARN, L_DB_SQL, "Updating DB version to v%d.\n", 4);
		ret = sql_exec(db, "CREATE TABLE releases (id INTEGER PRIMARY KEY AUTOINCREMENT, tv_episode_id NUMERIC, name TEXT, link TEXT)");
		if ( ret != SQLITE_OK )
			result = 4;
	}

	if (db_vers < 5) {
		DPRINTF(E_WARN, L_DB_SQL, "Updating DB version to v%d.\n", 5);
		ret = sql_exec(db, "ALTER TABLE tv_episode ADD watched NUMERIC");
		if ( ret != SQLITE_OK )
			result = 5;
	}


	sql_exec(db, "PRAGMA user_version = 5");
	return result;

}
