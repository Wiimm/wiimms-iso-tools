
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

#define _GNU_SOURCE 1

#include <string.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include "dclib-mysql.h"
#include "dclib-network.h"

// http://dev.mysql.com/doc/refman/5.1/en/c-api-functions.html

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MySql_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// management

void InitializeMYSQL ( MySql_t *my )
{
 #if DEBUG
    static uint done = 0;
    if (!done)
    {
	done++;
	TRACE("- MYSQL\n");
	TRACE_SIZEOF(MySql_t);
	TRACE_SIZEOF(MySqlResult_t);
	TRACE_SIZEOF(MySqlStatus_t);
	TRACE_SIZEOF(st_mysql);
	TRACE_SIZEOF(st_mysql_res);
	TRACE_SIZEOF(st_mysql_field);
    }
 #endif

    DASSERT(my);
    memset(my,0,sizeof(*my));
    my->auto_reconnect = true;
    my->prefix = EmptyString;
    InitializeStatusMYSQL(&my->status);
}

///////////////////////////////////////////////////////////////////////////////

void ResetMYSQL ( MySql_t *my )
{
    DASSERT(my);
    CloseMYSQL(my);

    FREE((char*)my->server);
    FREE((char*)my->user);
    FREE((char*)my->password);
    FREE((char*)my->database);
    FreeString(my->prefix);

    InitializeMYSQL(my);
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenMYSQL ( MySql_t *my, uint loglevel )
{
    DASSERT(my);
    ClearStatusMYSQL(&my->status);

    if (my->mysql)
	return ERR_OK; // already connected

    if ( !my || !my->server )
    {
	if ( loglevel >= MYLL_ERROR )
	    ERROR0(ERR_MISSING_PARAM,"No MySql server defined.\n");
	return ERR_MISSING_PARAM;
    }

    NetworkHost_t host;
    ResolveHost(&host,true,my->server,MYSQL_DEFAULT_PORT,false,true);

    PRINT("MYSQL CONNECT: %s:%u -> %s\n",
		host.name, host.port, PrintIP4(0,0,host.ip4,host.port) );

    static bool lib_init_done = false;
    if (!lib_init_done)
    {
	noPRINT("mysql_library_init()\n");
	if (mysql_library_init(0,NULL,NULL))
	{
	    if ( loglevel >= MYLL_ERROR )
		ERROR0(ERR_DATABASE,"Can't initialize MySql library.\n");
	    return ERR_DATABASE;
	}
	lib_init_done = true;
    }

    my->mysql = mysql_init(NULL);
    //HEXDUMP16(0,0,my->mysql,sizeof(*my->mysql));
    if (!my->mysql)
    {
	if ( loglevel >= MYLL_ERROR )
	    ERROR0(ERR_DATABASE,"Can't initialize MySql data.\n");
	ResetHost(&host);
	return ERR_DATABASE;
    }

    noPRINT("connect(%p,%s,%s,%s,%s,%u,,)\n",
		my->mysql, host.name, my->user, my->password, my->database, host.port );
    MYSQL *stat = mysql_real_connect ( my->mysql, host.name,
			my->user, my->password, my->database,
			host.port, NULL, 0 );

    if (!stat)
    {
	GetStatusMYSQL(&my->status,my->mysql);
	if ( loglevel >= MYLL_ERROR )
	    ERROR0(ERR_CANT_CONNECT,"Can't connect to %s:\n-> %s\n",
		PrintIP4(0,0,host.ip4,host.port),my->status.message);
	ResetHost(&host);
	mysql_close(my->mysql);
	my->mysql = 0;
	return ERR_CANT_CONNECT;
    }

    my->total_connect_count++;
    ResetHost(&host);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void CloseMYSQL ( MySql_t *my )
{
    DASSERT(my);

    //--- close results

    FreeResultListMYSQL(my->first_result);
    my->first_result = 0;
    FreeResultListMYSQL(my->pool_result);
    my->pool_result = 0;


    //--- close database

    if (my->mysql)
    {
	mysql_close(my->mysql);
	my->mysql = 0;
    }


    //--- reset status

    ClearStatusMYSQL(&my->status);
}

///////////////////////////////////////////////////////////////////////////////

void DefineDatabase
(
    MySql_t	*my,		// valid struct
    ccp		server,		// NULL or server name
    ccp		user,		// NULL or user name
    ccp		password,	// NULL or user password
    ccp		database	// NULL or name of database
)
{
    DASSERT(my);
    CloseMYSQL(my);

    if (server)
    {
	FREE((char*)my->server);
	my->server = STRDUP(server);
    }

    if (user)
    {
	FREE((char*)my->user);
	my->user = STRDUP(user);
    }

    if (password)
    {
	FREE((char*)my->password);
	my->password = STRDUP(password);
    }

    if (database)
    {
	FREE((char*)my->database);
	my->database = STRDUP(database);
    }

    FreeString(my->prefix);
    my->prefix = EmptyString;
}

///////////////////////////////////////////////////////////////////////////////

void DefineDatabasePrefix
(
    MySql_t	*my,		// valid struct
    ccp		prefix		// NULL or default prefix, only for external usage
)
{
    DASSERT(my);

    FreeString(my->prefix);
    my->prefix = prefix && *prefix ? STRDUP(prefix) : EmptyString;
}

///////////////////////////////////////////////////////////////////////////////

static enumError CheckOpenMYSQL ( MySql_t *my, uint loglevel )
{
    if (AutoOpenMYSQL(my,loglevel))
	return ERR_OK;

    if ( loglevel >= MYLL_ERROR )
	ERROR0(ERR_SEMANTIC,"No database connected.\n");
    return ERR_SEMANTIC;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// queries

enumError DoQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		query,		// query string
    int		query_length	// length of 'query', -1 if unknown
)
{
    DASSERT(my);
    DASSERT(query);

    const u64 start_time = GetTimeUSec(false);

    enumError err = CheckOpenMYSQL(my,loglevel);
    if (err)
	return err;

    if ( query_length < 0 )
	query_length = strlen(query);
    my->total_query_count++;
    my->total_query_size += query_length;

    if (mysql_real_query(my->mysql,query,query_length))
    {
	TRACE("QUERY: %.*s\n",query_length,query);
	my->field_count = 0;
	GetStatusMYSQL(&my->status,my->mysql);

	bool fail = true;
	if ( my->status.status == CR_SERVER_GONE_ERROR && my->auto_reconnect )
	{
	    MySqlStatus_t save;
	    MoveStatusMYSQL(&save,true,&my->status);
	    CloseMYSQL(my);
	    if ( OpenMYSQL(my,loglevel) || mysql_real_query(my->mysql,query,query_length) )
		MoveStatusMYSQL(&my->status,false,&save);
	    else
	    {
		ClearStatusMYSQL(&save);
		fail = false;
	    }
	}

	if (fail)
	{
	    if ( loglevel >= MYLL_QUERY_ON_ERROR )
		ERROR0(ERR_DATABASE,"%s (#%d)\n--QUERY--\n%.*s\n",
			    my->status.message, my->status.status,
			    query_length, query );
	    else if ( loglevel >= MYLL_ERROR )
		ERROR0(ERR_DATABASE,"%s (#%d)\n",
			    my->status.message, my->status.status );

	    my->last_duration_usec = GetTimeUSec(false) - start_time;
	    if ( my->max_duration_usec < my->last_duration_usec )
		 my->max_duration_usec = my->last_duration_usec;
	    my->total_duration_usec += my->last_duration_usec;
	    return ERR_DATABASE;
	}
    }

    if ( loglevel >= MYLL_QUERY_ALWAYS )
	ERROR0(ERR_OK,"%.*s\n", query_length, query );

    my->field_count = mysql_field_count(my->mysql);
    ClearStatusMYSQL(&my->status);

    my->last_duration_usec = GetTimeUSec(false) - start_time;
    if ( my->max_duration_usec < my->last_duration_usec )
	 my->max_duration_usec = my->last_duration_usec;
    my->total_duration_usec += my->last_duration_usec;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintArgQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    va_list	arg		// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    char buf[10000];

    va_list arg2;
    va_copy(arg2,arg);
    int stat = vsnprintf(buf,sizeof(buf),format,arg2);
    va_end(arg2);

    if ( stat < sizeof(buf) )
	return DoQueryMYSQL(my,loglevel,buf,stat);


    //--- buffer too small, use dynamic memory

    noPRINT("PrintQueryMYSQL() -> MALLOC(%u)\n",stat+1);

    char *dest = MALLOC(stat+1);
    stat = vsnprintf(dest,stat+1,format,arg);

    const enumError err = DoQueryMYSQL(my,loglevel,dest,stat);
    FREE(dest);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    va_list arg;
    va_start(arg,format);
    enumError err = PrintArgQueryMYSQL(my,loglevel,format,arg);
    va_end(arg);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// support of multi queries

mem_t GetFirstQuery ( ccp src, int src_len )
{
    if (!src)
    {
	mem_t res = {0,0};
	return res;
    }

    ccp ptr = src;
    ccp end = ptr + ( src_len < 0 ? strlen(src) : src_len );

    //--- skip all leading blanks, controls and semicolons

    while ( ptr < end && ( *ptr >= 0 && *ptr <= ' ' || *ptr == ';' ) )
	ptr++;

    if ( ptr == end )
    {
	mem_t res = {ptr,0};
	return res;
    }

    ccp start = ptr;
    while ( ptr < end )
    {
	const char ch = *ptr++;

	if ( ch == ';' )
	{
	    mem_t res = {start,ptr-start-1};
	    return res;
	}

	if ( ch == '\\' && ptr < end )
	    ptr++;
	else if ( ch == '\'' || ch == '\"' )
	{
	    while ( ptr < end )
	    {
		const char ch2 = *ptr++;
		if ( ch2 == '\\' && ptr < end )
		    ptr++;
		else if ( ch2 == ch )
		    break;
	    }
	}
    }
    mem_t res = {start,ptr-start};
    return res;
}

///////////////////////////////////////////////////////////////////////////////

enumError DoMultiQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		query,		// query string
    int		query_length	// length of 'query', -1 if unknown
)
{
    DASSERT(my);
    DASSERT(query);

    mem_t q;
    q.ptr = query;
    q.len = query_length < 0 ? strlen(query) : query_length;

    for(;;)
    {
	mem_t q1 = GetFirstQuery(q.ptr,q.len);
	if (!q1.len)
	    break;
	const enumError err = DoQueryMYSQL(my,loglevel,q1.ptr,q1.len);
	if (err)
	    return err;
	q = BehindMem(q,q1.ptr+q1.len);
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintMultiQueryMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    char buf[10000];
    va_list arg;
    va_start(arg,format);
    int stat = vsnprintf(buf,sizeof(buf),format,arg);
    va_end(arg);

    if ( stat < sizeof(buf) )
	return DoMultiQueryMYSQL(my,loglevel,buf,stat);


    //--- buffer too small, use dynamic memory

    noPRINT("PrintQueryMYSQL() -> MALLOC(%u)\n",stat+1);

    char *dest = MALLOC(stat+1);
    va_start(arg,format);
    stat = vsnprintf(dest,stat+1,format,arg);
    va_end(arg);

    const enumError err = DoMultiQueryMYSQL(my,loglevel,dest,stat);
    FREE(dest);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// temp buffer support

mem_t CopyMemMYSQL ( MySql_t * my, mem_t * src ) // 'src' may be NULL
{
    if ( src && src->len > 0 )
    {
	if ( src->len < sizeof(my->tempbuf) && !my->tempbuf_used )
	{
	    noPRINT("MYSQL: use tempbuf(%u)\n",src->len);
	    memcpy(my->tempbuf,src->ptr,src->len);
	    my->tempbuf[src->len] = 0;
	    my->tempbuf_used = true;

	    mem_t res = { my->tempbuf, src->len };
	    return res;
	}

	return DupMem(*src);
    }

    mem_t res = {NULL,0};
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void FreeMemMYSQL ( MySql_t * my, mem_t mem )
{
    DASSERT(my);
    if ( mem.ptr == my->tempbuf )
    {
	noPRINT("MYSQL: free tempbuf\n");
	my->tempbuf_used = false;
    }
    else
	FREE((char*)mem.ptr);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// fetch single value as result of a query

mem_t FetchScalarMYSQL // FREE the result!
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel	// log level for mysql errors and queries
)
{
    DASSERT(my);
    mem_t ret = {NULL,0};

    MySqlResult_t *res = UseResultMYSQL(my,loglevel);
    if (res)
    {
	ret = CopyMemMYSQL(my,GetNextRowMYSQL(res));
	while (GetNextRowMYSQL(res)) // get all rows
	    ;
	FreeResultMYSQL(res);
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

s64 FetchIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    s64		return_default	// return this value on non existent result
)
{
    DASSERT(my);
    mem_t res = FetchScalarMYSQL(my,loglevel);
    if (!res.len)
	return return_default;

    char *end;
    const s64 val = strtoll(res.ptr,&end,10);
    FreeMemMYSQL(my,res);
    return end == res.ptr + res.len ? val : return_default;
}

///////////////////////////////////////////////////////////////////////////////

u64 FetchUIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    u64		return_default	// return this value on non existent result
)
{
    DASSERT(my);
    mem_t res = FetchScalarMYSQL(my,loglevel);
    if (!res.len)
	return return_default;

    char *end;
    const u64 val = strtoull(res.ptr,&end,10);
    FreeMemMYSQL(my,res);
    return end == res.ptr + res.len ? val : return_default;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// print query & fetch single value as result

mem_t PrintArgFetchScalarMYSQL
(
    // if res.len>0: FREE the result using FreeMemMYSQL(my,mem)

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    va_list	arg		// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    mem_t ret = {NULL,0};
    if (!PrintArgQueryMYSQL(my,loglevel,format,arg))
    {
	MySqlResult_t *res = UseResultMYSQL(my,loglevel);
	if (res)
	{
	    ret = CopyMemMYSQL(my,GetNextRowMYSQL(res));
	    while (GetNextRowMYSQL(res)) // get all rows
		;
	    FreeResultMYSQL(res);
	}
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

mem_t PrintFetchScalarMYSQL
(
    // if res.len>0: FREE the result using FreeMemMYSQL(my,mem)

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    va_list arg;
    va_start(arg,format);
    mem_t res = PrintArgFetchScalarMYSQL(my,loglevel,format,arg);
    va_end(arg);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

s64 PrintFetchIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    s64		return_default,	// return this value on non existent result
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    va_list arg;
    va_start(arg,format);
    mem_t res = PrintArgFetchScalarMYSQL(my,loglevel,format,arg);
    va_end(arg);

    if (!res.len)
	return return_default;

    char *end;
    const s64 val = strtoll(res.ptr,&end,10);
    FreeMemMYSQL(my,res);
    return end == res.ptr + res.len ? val : return_default;
}

///////////////////////////////////////////////////////////////////////////////

u64 PrintFetchUIntMYSQL
(
    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    u64		return_default,	// return this value on non existent result
    ccp		format,		// format string for vsnprintf()
    ...				// parameters for 'format'
)
{
    DASSERT(my);
    DASSERT(format);

    va_list arg;
    va_start(arg,format);
    mem_t res = PrintArgFetchScalarMYSQL(my,loglevel,format,arg);
    va_end(arg);

    if (!res.len)
	return return_default;

    char *end;
    const u64 val = strtoull(res.ptr,&end,10);
    FreeMemMYSQL(my,res);
    return end == res.ptr + res.len ? val : return_default;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
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
)
{
    DASSERT(my);
    DASSERT( dest || !n_dest );
    memset(dest,0,sizeof(*dest)*n_dest);

    char *buf = 0;
    MySqlResult_t *res = UseResultMYSQL(my,loglevel);
    if (res)
    {
	mem_t *row = GetNextRowMYSQL(res);
	if (row)
	{
	    if ( n_dest > res->field_count )
		 n_dest = res->field_count;

	    //--- count memory usage and alloc buffer

	    uint i, need = 0;
	    for ( i = 0; i < n_dest; i++ )
		if ( row[i].len )
		    need += row[i].len + 1;

	    //--- get row data

	    buf = MALLOC(need);
	    char *ptr = buf;
	    for ( i = 0; i < n_dest; i++, row++, dest++ )
	    {
		if ( row->len )
		{
		    dest->ptr = ptr;
		    memcpy(ptr,row->ptr,row->len);
		    dest->len = row->len;
		    ptr += row->len;
		    *ptr++ = 0;
		    DASSERT( ptr <= buf+need );
		}
	    }
	    DASSERT( ptr == buf+need );


	    //--- get and ignore all remaining rows

	    while (GetNextRowMYSQL(res))
		;
	}
	FreeResultMYSQL(res);
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FetchSingleRowList
(
    // returns -1 on error, 0 on empty result or the number of retrieved field
    // following rows will be skipped

    MySql_t	*my,		// valid struct
    LogLevelMYSQL loglevel,	// log level for mysql errors and queries
    mem_list_t	*dest,		// destination list
    bool	init_dest,	// true: initialize 'dest'
    bool	replace_null	// true: replace NULL by EmptyString
)
{
    DASSERT(my);
    DASSERT(dest);

    if (init_dest)
	InitializeMemList(dest);
    dest->used = 0;

    int stat = -1;
    MySqlResult_t *res = UseResultMYSQL(my,loglevel);
    if (res)
    {
	stat = 0;
	mem_t *row = GetNextRowMYSQL(res);
	if (row)
	{
	    stat = res->field_count;
	    AssignMemListN(dest,row,stat,replace_null?3:0);

	    //--- get and ignore all remaining rows
	    while (GetNextRowMYSQL(res))
		;
	}
	FreeResultMYSQL(res);
    }

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// count tables and columns

int CountTablesMYSQL
(
    MySql_t	*my,		// valid DB connection
    ccp		table,		// like "table"; if NULL|EMPTY: count tables
    ccp		database	// database name; if NULL|EMPTY: use default DB
)
{
    DASSERT(my);

    int stat = -1;
    mem_t db = EscapeStringMYSQL(my, database && *database ? database : my->database );
    mem_t tab = EscapeStringMYSQL(my, table && *table ? table : "%" );

    stat = PrintFetchIntMYSQL(my,MYLL_QUERY_ON_ERROR,-1,
		"SELECT COUNT(DISTINCT TABLE_NAME) FROM information_schema.COLUMNS\n"
		"WHERE TABLE_SCHEMA = \"%s\" && TABLE_NAME like \"%s\"\n",
		db.ptr, tab.ptr );

    FreeString(tab.ptr);
    FreeString(db.ptr);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int CountColumnsMYSQL
(
    MySql_t	*my,		// valid DB connection
    ccp		column,		// column name; if NULL|EMPTY: count columns
    ccp		table,		// like "table"; if NULL|EMPTY: all tables
    ccp		database	// database name; if NULL|EMPTY: use default DB
)
{
    DASSERT(my);

    int stat = -1;
    mem_t db  = EscapeStringMYSQL(my, database && *database ? database : my->database );
    mem_t tab = EscapeStringMYSQL(my, table && *table ? table : "%" );

    if ( column && *column )
    {
	mem_t col = EscapeStringMYSQL(my,column);
	stat = PrintFetchIntMYSQL(my,MYLL_QUERY_ON_ERROR,-1,
	    "SELECT COUNT(*) FROM information_schema.COLUMNS\n"
	    "WHERE TABLE_SCHEMA = \"%s\" && TABLE_NAME like \"%s\""
	    " && COLUMN_NAME = \"%s\"\n",
	    db.ptr, tab.ptr, col.ptr );
	FreeString(col.ptr);
    }
    else
    {
	stat = PrintFetchIntMYSQL(my,MYLL_QUERY_ON_ERROR,-1,
	    "SELECT COUNT(*) FROM information_schema.COLUMNS\n"
	    "WHERE TABLE_SCHEMA = \"%s\" && TABLE_NAME like \"%s\"\n",
	    db.ptr, tab.ptr );
    }

    FreeString(tab.ptr);
    FreeString(db.ptr);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// helpers

mem_t EscapeMemMYSQL ( MySql_t *my, mem_t src )
{
    mem_t res;
    char *buf = MALLOC(2*src.len+1);
    res.ptr = buf;
    res.len = mysql_real_escape_string(my->mysql,buf,src.ptr,src.len);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t EscapeStringMYSQL ( MySql_t *my, ccp src )
{
    mem_t res;
    const uint len = src ? strlen(src) : 0;
    char *buf = MALLOC(2*len+1);
    res.ptr = buf;
    res.len = mysql_real_escape_string(my->mysql,buf,src,len);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

TransferStats_t GetStatsMYSQL ( MySql_t *my )
{
    TransferStats_t tf;
    memset(&tf,0,sizeof(tf));

    if (my)
    {
	tf.conn_count	= my->total_connect_count;
	tf.recv_count	= my->total_row_count;
	tf.recv_size	= my->total_field_size;
	tf.send_count	= my->total_query_count;
	tf.send_size	= my->total_query_size;
    }
    return tf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     struct MySqlResult_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static void InsertResultMYSQL ( MySqlResult_t ** first, MySqlResult_t * res )
{
    DASSERT(first);
    DASSERT(res);
    DASSERT( res != *first ); // already cleared!

    //--- remove res from previous list

    if (res->prev_result)
	res->prev_result->next_result = res->next_result;
    if (res->next_result)
	res->next_result->prev_result = res->prev_result;

    //--- insert at top of the new list

    res->prev_result = 0;
    res->next_result = *first;
    if (*first)
	(*first)->prev_result = res;
    *first = res;

    FREE(res->row);
    res->row = 0;
}

///////////////////////////////////////////////////////////////////////////////

MySqlResult_t * UseResultMYSQL ( MySql_t *my, uint loglevel )
{
    DASSERT(my);
    enumError err = CheckOpenMYSQL(my,loglevel);
    if (err)
    {
	AssignStatusMYSQL(&my->status,1,"No connection to database!",1);
	return NULL;
    }

    MYSQL_RES *myres = mysql_use_result(my->mysql);
    if (!myres)
    {
	GetStatusMYSQL(&my->status,my->mysql);
	if ( loglevel >= MYLL_ERROR )
	    ERROR0(ERR_DATABASE,"%s\n",my->status.message);
	return NULL;
    }
    my->total_result_count++;

    MySqlResult_t *res;
    if (my->pool_result)
    {
	res = my->pool_result;
	my->pool_result = res->next_result;
    }
    else
	res = CALLOC(1,sizeof(*res));
    InsertResultMYSQL(&my->first_result,res);

    res->mysql = my;
    res->myres = myres;
    res->loglevel = loglevel;
    res->row_count =  0;
    res->field_count = mysql_num_fields(myres);
    res->row = CALLOC(sizeof(*res->row),res->field_count);

    GetStatusMYSQL(&res->status,my->mysql);

    return res;
}

///////////////////////////////////////////////////////////////////////////////

void FreeResultMYSQL ( MySqlResult_t * res )
{
    if (res)
    {
	MySql_t *my = res->mysql;
	DASSERT(my);

	CloseResultMYSQL(res);
	ClearStatusMYSQL(&res->status);
	if ( my->first_result == res )
	    my->first_result = res->next_result;
	InsertResultMYSQL(&my->pool_result,res);
    }
}

///////////////////////////////////////////////////////////////////////////////

void CloseResultMYSQL ( MySqlResult_t * res )
{
    DASSERT(res);
    if (res->myres)
    {
	mysql_free_result(res->myres);
	res->myres = 0;
    }
    FREE(res->row);
    res->row = 0;
    res->field_count = 0;
}

///////////////////////////////////////////////////////////////////////////////

void FreeResultListMYSQL ( MySqlResult_t * first )
{
    while (first)
    {
	MySqlResult_t *res = first;
	first = res->next_result;
	CloseResultMYSQL(res);
	ClearStatusMYSQL(&res->status);
	FREE(res);
    }
}

///////////////////////////////////////////////////////////////////////////////

mem_t * GetNextRowMYSQL ( MySqlResult_t * res )
{
    DASSERT(res);
    if ( !res || !res->myres || !res->row )
	return NULL;

    MYSQL_ROW myrow = mysql_fetch_row(res->myres);
    ulong *mylen = mysql_fetch_lengths(res->myres);
    res->myrow = myrow;
    if ( !myrow || !mylen )
    {
	FREE(res->row);
	res->row = 0;
	return NULL;
    }
    res->mysql->total_row_count++;

    mem_t *dest = res->row;
    uint n = res->field_count;
    res->mysql->total_field_count += n;
    while ( n-- > 0 )
    {
	dest->ptr = *myrow++;
	dest->len = *mylen++;
	res->mysql->total_field_size += dest->len;
	noPRINT("%3u: |%.*s|\n",dest->len,dest->len<50?dest->len:50,dest->ptr);
	dest++;
    }

    return res->row;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     struct MySqlStatus_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void ClearStatusMYSQL ( MySqlStatus_t *stat )
{
    DASSERT(stat);
    if (stat->alloced)
    {
	FREE((char*)stat->message);
	stat->alloced = false;
    }
    stat->message = stat->msgbuf;
    stat->msgbuf[0] = 0;
    stat->status = 0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetStatusMYSQL ( MySqlStatus_t *stat, MYSQL *my )
{
    DASSERT(stat);
    if (my)
	AssignStatusMYSQL(stat,mysql_errno(my),mysql_error(my),0);
    else
	ClearStatusMYSQL(stat);

    return stat->status;
}

///////////////////////////////////////////////////////////////////////////////

void AssignStatusMYSQL
(
    MySqlStatus_t	*stat,		// valid struct
    int			stat_code,	// error status to assign
    ccp			message,	// NULL or pointer to message
    int			copy_mode	// 0:copy message, 1:use ptr, 2:use+free ptr
)
{
    DASSERT(stat);
    ClearStatusMYSQL(stat);
    stat->status = stat_code;

    if (message)
    {
	if ( copy_mode > 0 )
	{
	    stat->message = message;
	    stat->alloced = copy_mode > 1;
	}
	else
	{
	    const uint len = strlen(message);
	    if ( len < sizeof(stat->msgbuf) )
	    {
		memcpy(stat->msgbuf,message,len+1);
		stat->message = stat->msgbuf;
		stat->alloced = false;
	    }
	    else
	    {
		stat->message = MEMDUP(message,len);
		stat->alloced = true;
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void CopyStatusMYSQL
(
    MySqlStatus_t	*dest,		// NULL or initialized status
    bool		init_dest,	// true: initialize 'dest'
    const MySqlStatus_t	*src		// NULL (clear dest)
					// or initialized status (assign to dest)
)
{
    if (dest)
    {
	if (init_dest)
	    InitializeStatusMYSQL(dest);

	if ( dest != src )
	{
	    if (src)
		AssignStatusMYSQL(dest,src->status,src->message,0);
	    else
		ClearStatusMYSQL(dest);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void MoveStatusMYSQL
(
    MySqlStatus_t	*dest,		// NULL or initialized status
    bool		init_dest,	// true: initialize 'dest'
    MySqlStatus_t	*src		// NULL (clear dest)
					// or initialized status (move to dest)
)
{
    if ( dest != src )
    {
	if (dest)
	{
	    if (init_dest)
		InitializeStatusMYSQL(dest);
	    else
		ClearStatusMYSQL(dest);

	    if (src)
	    {
		memcpy(dest,src,sizeof(*dest));
		if ( src->message == src->msgbuf )
		    dest->message = dest->msgbuf;
		memset(src,0,sizeof(*src));
	    }
	}
	else if (src)
	    ClearStatusMYSQL(src);
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrintStatusMYSQL
(
    MySqlStatus_t	*stat,		// valid struct
    int			stat_code,	// error status to assign
    ccp			format,		// format string for vsnprintf()
    ...					// parameters for 'format'
)
{
    DASSERT(stat);
    DASSERT(format);

    char buf[10000];
    va_list arg;
    va_start(arg,format);
    int len = vsnprintf(buf,sizeof(buf),format,arg);
    va_end(arg);

    if ( len < sizeof(buf) )
	AssignStatusMYSQL(stat,stat_code,buf,0);
    else
    {
	//--- buffer too small, use dynamic memory

	noPRINT("PrintQueryMYSQL() -> MALLOC(%u)\n",stat+1);

	char *dest = MALLOC(len+1);
	va_start(arg,format);
	len = vsnprintf(dest,len+1,format,arg);
	va_end(arg);

	AssignStatusMYSQL(stat,stat_code,dest,2);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MySqlServerStats_t		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError GetMySqlServerStats ( MySql_t *my, MySqlServerStats_t *stat )
{
    DASSERT(my);
    DASSERT(stat);
    InitializeMySqlServerStats(stat);

    static const char query[]
	= "show status where variable_name in"
	  " ('Uptime','Max_used_connections','Connections','Queries')";

    enumError err
	= DoQueryMYSQL(my,MYLL_QUERY_ON_ERROR,query,sizeof(query)-1);
    if (!err)
    {
	MySqlResult_t *res = UseResultMYSQL(my,false);
	if (res)
	{
	    for(;;)
	    {
		mem_t *row = GetNextRowMYSQL(res);
		if (!row)
		    break;

		//printf("|%s|%s|\n",row[0].ptr,row[1].ptr);
		switch(row[0].ptr[0])
		{
		 case 'C':
		    if (!strcmp(row[0].ptr,"Connections"))
			stat->connections = strtoull(row[1].ptr,0,10);
		    break;

		 case 'M':
		    if (!strcmp(row[0].ptr,"Max_used_connections"))
			stat->max_connections = strtoull(row[1].ptr,0,10);
		    break;

		 case 'Q':
		    if (!strcmp(row[0].ptr,"Queries"))
			stat->queries = strtoull(row[1].ptr,0,10);
		    break;

		 case 'U':
		    if (!strcmp(row[0].ptr,"Uptime"))
			stat->uptime = strtoull(row[1].ptr,0,10);
		    break;
		}
	    }
	    FreeResultMYSQL(res);
	}
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

MySqlServerStats_t * Add3MySqlServerStats
(
    // return dest
    // calculate: dest = src1 + src2

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src1,	// NULL or first source
    const MySqlServerStats_t	*src2	// NULL or second source
)
{
    if (dest)
    {
	if (!src1)
	{
	    if (src2)
		memcpy(dest,src2,sizeof(*dest));
	    else
		InitializeMySqlServerStats(dest);
	}
	else if (!src2)
	{
	    DASSERT(src1);
	    memcpy(dest,src1,sizeof(*dest));
	}
	else
	{
	    DASSERT(src1);
	    DASSERT(src2);

	    dest->max_connections = src1->max_connections > src2->max_connections
				  ? src1->max_connections : src2->max_connections;
	    dest->uptime      = src1->uptime      + src2->uptime;
	    dest->connections = src1->connections + src2->connections;
	    dest->queries     = src1->queries     + src2->queries;
	}
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

MySqlServerStats_t * Sub3MySqlServerStats
(
    // return dest
    // calculate: dest = src1 - src2

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src1,	// NULL or first source
    const MySqlServerStats_t	*src2	// NULL or second source
)
{
    if (dest)
    {
	if (!src1)
	{
	    if (src2)
	    {
		MySqlServerStats_t temp;
		InitializeMySqlServerStats(&temp);
		Sub3MySqlServerStats(dest,&temp,src2);
	    }
	    else
		InitializeMySqlServerStats(dest);
	}
	else if (!src2)
	{
	    DASSERT(src1);
	    memcpy(dest,src1,sizeof(*dest));
	}
	else
	{
	    DASSERT(src1);
	    DASSERT(src2);
	    dest->max_connections = src1->max_connections;
	    dest->uptime      = src1->uptime      - src2->uptime;
	    dest->connections = src1->connections - src2->connections;
	    dest->queries     = src1->queries     - src2->queries;
	}
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

MySqlServerStats_t * Max2MySqlServerStats
(
    // return dest
    // calculate: dest := max(dest,src)

    MySqlServerStats_t		*dest,	// NULL or destination (maybe same as source)
    const MySqlServerStats_t	*src	// NULL or source
)
{
    if ( src && dest )
    {
	if ( dest->max_connections < src->max_connections )
	     dest->max_connections = src->max_connections;

	if ( dest->uptime < src->uptime )
	     dest->uptime = src->uptime;

	if ( dest->connections < src->connections )
	     dest->connections = src->connections;

	if ( dest->queries < src->queries )
	     dest->queries = src->queries;
    }
    return dest;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////
