
/***************************************************************************
 *                                                                         *
 *                     _____     ____                                      *
 *                    |  __ \   / __ \   _     _ _____                     *
 *                    | |  \ \ / /  \_\ | |   | |  _  \                    *
 *                    | |   \ \| |      | |   | | |_| |                    *
 *                    | |   | || |      | |   | |  ___/                    *
 *                    | |   / /| |   __ | |   | |  _  \                    *
 *                    | |__/ / \ \__/ / | |___| | |_| |                    *
 *                    |_____/   \____/  |_____|_|_____/                    *
 *                                                                         *
 *                       Wiimms source code library                        *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *        Copyright (c) 2012-2018 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#ifndef DCLIB_MYSQL_H
#define DCLIB_MYSQL_H 1

#include <mysql/mysql.h>

#include "dclib-types.h"
#include "dclib-debug.h"
#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     mysql definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct st_mysql		st_mysql;
typedef struct st_mysql_res	st_mysql_res;
typedef struct st_mysql_field	st_mysql_field;

#define MYSQL_DEFAULT_PORT 3306

struct MySql_t;

typedef enum LogLevelMYSQL
{
    MYLL_SILENT,
    MYLL_ERROR,
    MYLL_QUERY_ON_ERROR,
    MYLL_QUERY_ALWAYS,
    MYLL_ALL
}
LogLevelMYSQL;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     struct MySqlStatus_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct MySqlStatus_t
{
    int		status;		// status of last operation
    ccp		message;	// pointer to last error message
    bool	alloced;	// 'message' is alloced
    char	msgbuf[120];	// buffer for short messages
}
MySqlStatus_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     struct MySqlResult_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MySqlResult_t]]

typedef struct MySqlResult_t
{
    //--- linked double-list for management

    struct MySqlResult_t *prev_result;	// NULL or link to previous result
    struct MySqlResult_t *next_result;	// NULL or link to next result
    struct MySql_t	*mysql;		// related MySql_t struct

    //--- mysql result and status

    MYSQL_RES		*myres;		// mysql result
    LogLevelMYSQL	loglevel;	// log level for mysql errors and queries
    MySqlStatus_t	status;		// last error status

    //--- rows and fields

    int			row_count;	// number of read rows
    int			field_count;	// number of fields in a row
    mem_t		*row;		// list with 'field_count' entries
    MYSQL_ROW		myrow;		// mysql row

}
MySqlResult_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MySql_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MySql_t]]

typedef struct MySql_t
{
    //--- database definitions, strings are alloced

    ccp			server;		// server
    ccp			user;		// user
    ccp			password;	// password
    ccp			database;	// database name
    ccp			prefix;		// default prefix, only for external usage
    bool		auto_connect;	// true: auto connect if closed
    bool		auto_reconnect;	// true(default): try 1 reconnect on connection lost

    //--- mysql data

    MYSQL		*mysql;		// mysql connection
    uint		field_count;	// mysql_field_count() of last query
    MySqlStatus_t	status;		// last error status

    MySqlResult_t	*first_result;	// pointer to first result
    MySqlResult_t	*pool_result;	// list with unused objects

    //--- a small temporary buffer for e.g. small results

    char		tempbuf[120];	// temporary buffer
    bool		tempbuf_used;	// true: 'tempbuf' is used

    //--- statistics

    uint	total_connect_count;	// total number of connections (usualy 1)
    uint	total_query_count;	// total number of queries
    u64		total_query_size;	// total size of all queries
    uint	total_result_count;	// total number of fetched results
    u64		total_row_count;	// total number of fetched rows
    u64		total_field_count;	// total number of fetched fields
    u64		total_field_size;	// total size of all fetched fields

    //--- both timers count the execution time of DoQueryMYSQL()
    //--- inclusive database reopen and error handling.
    u64		last_duration_usec;	// duration of last query
    u64		max_duration_usec;	// max duration of all queries
    u64		total_duration_usec;	// total duration of all queries
}
MySql_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface for MySql_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMYSQL ( MySql_t *my );
void ResetMYSQL ( MySql_t *my );
enumError OpenMYSQL ( MySql_t *my, uint loglevel );
void CloseMYSQL ( MySql_t *my );

void DefineDatabase
(
    MySql_t	*my,		// valid struct
    ccp		server,		// NULL or server name
    ccp		user,		// NULL or user name
    ccp		password,	// NULL or user password
    ccp		database	// NULL or name of database
);

void DefineDatabasePrefix
(
    MySql_t	*my,		// valid struct
    ccp		prefix		// NULL or default prefix, only for external usage
);

//-----------------------------------------------------------------------------

static inline bool IsOpenMYSQL ( MySql_t *my ) // 'my' may be NULL
{
    return my && my->mysql;
}

static inline bool AutoOpenMYSQL ( MySql_t *my, uint loglevel ) // 'my' may be NULL
{
    return my
	&& ( my->mysql || my->auto_connect && OpenMYSQL(my,loglevel) == ERR_OK );
}

//-----------------------------------------------------------------------------

enumError DoQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		query,		// query string
    int		query_length	// length of 'query', -1 if unknown
);

enumError PrintArgQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    va_list	arg		// parameters for 'format'
);

enumError PrintQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,3,4)));

//-----------------------------------------------------------------------------
// multi queries

mem_t GetFirstQuery ( ccp src, int src_len );

enumError DoMultiQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		query,		// query string
    int		query_length	// length of 'query', -1 if unknown
);

enumError PrintMultiQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,3,4)));

//-----------------------------------------------------------------------------
// temp buffer for small results as optimization

mem_t CopyMemMYSQL ( MySql_t * my, mem_t * src ); // 'src' may be NULL
void FreeMemMYSQL  ( MySql_t * my, mem_t mem );

//-----------------------------------------------------------------------------
// fetch single value as result of a query

mem_t FetchScalarMYSQL // call FreeScalarMYSQL() for the result
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel	// log level for mysql errors and queries
);

s64 FetchIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    s64		return_default	// return this value on non existent result
);

u64 FetchUIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    u64		return_default	// return this value on non existent result
);

//-----------------------------------------------------------------------------
// print query & fetch single value as result

mem_t PrintArgFetchScalarMYSQL
(
    // if res.len>0: FREE the result using FreeMemMYSQL(my,mem)

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    va_list	arg		// parameters for 'format'
);

mem_t PrintFetchScalarMYSQL
(
    // if res.len>0: FREE the result using FreeMemMYSQL(my,mem)

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,3,4)));

s64 PrintFetchIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    s64		return_default,	// return this value on non existent result
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,4,5)));

u64 PrintFetchUIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    u64		return_default,	// return this value on non existent result
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,4,5)));

//-----------------------------------------------------------------------------
// fetch single row

void * FetchSingleRowN
(
    // returns NULL on error or emtpy result,
    //		or a temporary buffer => call FREE(result)
    // following rows will be skipped

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    mem_t	*dest,		// dest buffer, unused elements are set to {0,0}
    uint	n_dest		// number of elements in 'dest'
);

int FetchSingleRowList
(
    // returns -1 on error, 0 on empty result or the number of retrieved field
    // following rows will be skipped

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    mem_list_t	*dest,		// destination list
    bool	init_dest,	// true: initialize 'dest'
    bool	replace_null	// true: replace NULL by EmptyString
);

//-----------------------------------------------------------------------------
// count tables and columns

int CountTablesMYSQL
(
    MySql_t	*my,		// valid DB connection
    ccp		table,		// like "table"; if NULL|EMPTY: count tables
    ccp		database	// database name; if NULL|EMPTY: use default DB
);

int CountColumnsMYSQL
(
    MySql_t	*my,		// valid DB connection
    ccp		column,		// column name; if NULL|EMPTY: count columns
    ccp		table,		// like "table"; if NULL|EMPTY: all tables
    ccp		database	// database name; if NULL|EMPTY: use default DB
);

//-----------------------------------------------------------------------------
// helpers

static inline int LastInsertIdMYSQL ( MySql_t *my )
	{ return mysql_insert_id(my->mysql); }

mem_t EscapeMemMYSQL ( MySql_t *my, mem_t src );
mem_t EscapeStringMYSQL ( MySql_t *my, ccp src );

struct TransferStats_t GetStatsMYSQL ( MySql_t *my );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface MySqlResult_t			///////////////
///////////////////////////////////////////////////////////////////////////////

MySqlResult_t * UseResultMYSQL ( MySql_t * my, uint loglevel );
void FreeResultMYSQL ( MySqlResult_t * res ); // res maybe NULL
void CloseResultMYSQL ( MySqlResult_t * res );

void FreeResultListMYSQL ( MySqlResult_t * first_result );

mem_t * GetNextRowMYSQL ( MySqlResult_t * res );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface for MySqlStatus_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static inline void InitializeStatusMYSQL ( MySqlStatus_t *stat )
{
    DASSERT(stat);
    memset(stat,0,sizeof(*stat));
    stat->message = stat->msgbuf;
}

void ClearStatusMYSQL ( MySqlStatus_t *stat );
uint GetStatusMYSQL   ( MySqlStatus_t *stat, MYSQL *my );

void AssignStatusMYSQL
(
    MySqlStatus_t	*stat,		// valid struct
    int			stat_code,	// error status to assign
    ccp			message,	// NULL or pointer to message
    int			copy_mode	// 0:copy message, 1:use ptr, 2:use+free ptr
);

void CopyStatusMYSQL
(
    MySqlStatus_t	*dest,		// NULL or initialized status
    bool		init_dest,	// true: initialize 'dest'
    const MySqlStatus_t	*src		// NULL (clear dest)
					// or initialized status (assign to dest)
);

void MoveStatusMYSQL
(
    MySqlStatus_t	*dest,		// NULL or initialized status
    bool		init_dest,	// true: initialize 'dest'
    MySqlStatus_t	*src		// NULL (clear dest)
					// or initialized status (move to dest)
);

void PrintStatusMYSQL
(
    MySqlStatus_t	*stat,		// valid struct
    int			stat_code,	// error status to assign
    ccp			format,		// format string for vsnprintf()
    ...					// parameters for 'format'
)
__attribute__ ((__format__(__printf__,3,4)));

//
///////////////////////////////////////////////////////////////////////////////
///////////////			test_mysql_stats()		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MySqlServerStats_t]]

typedef struct MySqlServerStats_t
{
    u32 max_connections;	// maximal used connections
    s32 uptime;			// uptime in seconds
    s64 connections;		// number of total connections
    s64 queries;		// number of total queries
}
MySqlServerStats_t;

///////////////////////////////////////////////////////////////////////////////

static inline void InitializeMySqlServerStats ( MySqlServerStats_t *stat )
	{ DASSERT(stat); memset(stat,0,sizeof(*stat)); }

enumError GetMySqlServerStats ( MySql_t *my, MySqlServerStats_t *stat );

MySqlServerStats_t * Add3MySqlServerStats
(
    // return dest
    // calculate: dest = src1 + src2

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src1,	// NULL or first source
    const MySqlServerStats_t	*src2	// NULL or second source
);

MySqlServerStats_t * Sub3MySqlServerStats
(
    // return dest
    // calculate: dest = src1 - src2

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src1,	// NULL or first source
    const MySqlServerStats_t	*src2	// NULL or second source
);

MySqlServerStats_t * Max2MySqlServerStats
(
    // return dest
    // calculate: dest := max(dest,src)

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src	// NULL or source
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_MYSQL_H

