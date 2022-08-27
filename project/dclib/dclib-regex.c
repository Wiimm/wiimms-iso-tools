
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
 *        Copyright (c) 2012-2022 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include "dclib-regex.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			some macro definitions		///////////////
///////////////////////////////////////////////////////////////////////////////

#undef REGEX_FREE
#undef REOPT_ICASE
#undef MATCH_BEG
#undef MATCH_END

#if DCLIB_USE_PCRE
    #define REGEX_FREE pcre_free
    #define REOPT_ICASE PCRE_CASELESS
    #define MATCH_BEG(i) match[2*i]
    #define MATCH_END(i) match[2*i+1]
#else
    #define REGEX_FREE regfree
    #define REOPT_ICASE REG_ICASE
    #define MATCH_BEG(i) match[i].rm_so
    #define MATCH_END(i) match[i].rm_eo
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Regex_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeRegex ( Regex_t *re )
{
    DASSERT(re);
    memset(re,0,sizeof(*re));
    re->re_list = re->re_pool;
    re->re_size = sizeof(re->re_pool)/sizeof(*re->re_pool);
}

///////////////////////////////////////////////////////////////////////////////

void ResetRegex ( Regex_t *re )
{
    DASSERT(re);

    RegexElem_t *e, *eend = re->re_list + re->re_used;
    for ( e = re->re_list; e < eend; e++ )
    {
     #if DCLIB_USE_PCRE
	if (e->regex)
	    pcre_free(e->regex);
     #else
	regfree(&e->regex);
     #endif

	FreeString(e->pattern);
	FreeString(e->replace.ptr);
	FREE(e->repl);
    }

    if ( re->re_list != re->re_pool )
	FREE(re->re_list);

    InitializeRegex(re);
}

///////////////////////////////////////////////////////////////////////////////

static RegexElem_t * GetNextRegexElem ( Regex_t *re )
{
    DASSERT(re);
    if ( re->re_used >= re->re_size )
    {
	re->re_size = 3*re->re_size/2 + 10;
	if ( re->re_list == re->re_pool )
	{
	    re->re_list = MALLOC( sizeof(*re->re_list) * re->re_size );
	    memcpy( re->re_list, re->re_pool, sizeof(*re->re_list) * re->re_used );
	}
	else
	    re->re_list = REALLOC( re->re_list, sizeof(*re->re_list) * re->re_size );
    }

    RegexElem_t *e = re->re_list + re->re_used;
    memset(e,0,sizeof(*e));
    return e;
}

///////////////////////////////////////////////////////////////////////////////

static RegexReplace_t * GetNextRegexRepl
	( RegexElem_t *elem, char *ptr, int len, int ref )
{
    DASSERT(elem);
    if ( !len && ref < 0 )
	return 0;

    if ( elem->repl_used >= elem->repl_size )
    {
	elem->repl_size = 3*elem->repl_size/2 + 10;
	elem->repl = REALLOC( elem->repl, sizeof(*elem->repl) * elem->repl_size );
    }

    RegexReplace_t *repl = elem->repl + elem->repl_used++;
    repl->str.ptr = ptr;
    repl->str.len = len;
    repl->ref     = ref;
    PRINT("REPL[%d]: |%.*s| %d\n",elem->repl_used-1,len,ptr,ref);
    return repl;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanRegex ( Regex_t *re, bool init_re, ccp regex )
{
    DASSERT(re);
    if (init_re)
	InitializeRegex(re);
    else
	ResetRegex(re);

    if ( !regex || !*regex )
	return ERR_NOTHING_TO_DO;

    struct { FastBuf_t b; char space[500]; } temp;
    InitializeFastBuf(&temp,sizeof(temp));

    //--- scanning

    enumError err = ERR_OK;
    for(;;)
    {
	while ( isspace((int)*regex) || *regex == ';' )
	    regex++;
	if (!*regex)
	    break;

	noPRINT("RE: ANALYZE #%u/%u: %s\n",re->re_used, re->re_size,regex);

	char sep = *regex++;
	while ( *regex && *regex != sep )
	{
	    char ch = *regex++;
	    if ( ch == '\\' && *regex == sep )
		ch = *regex++;
	    AppendCharFastBuf(&temp.b,ch);
	}

	if ( *regex != sep )
	{
	    err = ERR_SYNTAX;
	    goto exit;
	}

	ccp repl = ++regex;
	while ( *regex && *regex != sep )
	    regex++;
	const uint repl_len = regex - repl;

	RegexElem_t *elem = GetNextRegexElem(re);

	if ( *regex == sep )
	{
	    regex++;
	    while ( *regex && *regex != ';' )
	      switch (*regex++)
	      {
		case 'g': elem->global = true; break;
		case 'i': elem->icase = true; elem->opt |= REOPT_ICASE; break;
	      }
	}

	PRINT("RE: PATTERN #%u/%u: [%s] %s -> %.*s : %s\n",
		re->re_used, re->re_size,
		GetFastBufStatus(&temp.b),
		GetFastBufString(&temp.b),
		repl_len, repl, regex );

	// store data as strings
	elem->pattern = MEMDUP(temp.b.buf,temp.b.ptr-temp.b.buf);
	elem->replace.ptr = repl_len ? MEMDUP(repl,repl_len) : EmptyString;
	elem->replace.len = repl_len;
	re->re_used++;
	ClearFastBuf(&temp.b);
    }


    //--- finalize

    re->valid = true;

    uint e;
    RegexElem_t *elem;
    for ( e = 0, elem = re->re_list; e < re->re_used; e++, elem++ )
    {
     #if DCLIB_USE_PCRE

	ccp errptr;
	int erroffset;
	re->regex = pcre_compile(elem->pattern,opt,&errptr,&erroffset,0);
	if (!re->regex)
	{
	    PRINT("PCRE error: %s: %s\n",errptr,pattern+erroffset);
	    return ERR_SEMANTIC;
	    err = ERR_SEMANTIC;
	    goto exit;
	}

     #else

	int stat = regcomp(&elem->regex,elem->pattern,elem->opt|REG_EXTENDED);
	if (stat)
	{
	 #if HAVE_PRINT0
	    char error[100];
	    regerror(stat,&re->regex,errormsizeof(error));
	    PRINT("PCRE error: %s: %s\n",errptr,pattern+erroffset);
	 #endif
	    re->valid = false;
	    err = ERR_SEMANTIC;
	}
	else
	    elem->valid = true;
     #endif


	//--- analyse replace

	char *ptr   = (char*)elem->replace.ptr;
	char *dest  = ptr;
	char *start = dest;
	char *end   = ptr + elem->replace.len;

	while ( ptr < end )
	{
	    while ( ptr < end && *ptr != '\\' && *ptr != '$' )
		*dest++ = *ptr++;
	    if ( ptr == end )
	    {
		GetNextRegexRepl(elem,start,dest-start,-1);
		break;
	    }

	    if ( *ptr == ptr[1] )
	    {
		ptr++;
		*dest++ = *ptr++;
	    }
	    else if ( ptr[1] >= '0' && ptr[1] <= '9' )
	    {
		GetNextRegexRepl(elem,start,dest-start,ptr[1]-'0');
		start = dest;
		ptr += 2;
	    }
	    else if ( ptr[1] == '{' )
	    {
		char *p = ptr+2;
		if ( *p >= '0' && *p <= '9' )
		{
		    uint num = *p++ - '0';
		    if ( *p >= '0' && *p <= '9' )
			num = 10*num +  *p++ - '0';
		    if ( p < end && *p == '}' )
		    {
			GetNextRegexRepl(elem,start,dest-start,num);
			start = dest;
			ptr = p+1;
			continue;
		    }
		}
		*dest++ = *ptr++;
	    }
	    else
		*dest++ = *ptr++;
	}

	if ( dest != EmptyString )
	    *dest = 0;
	elem->replace.len = dest - elem->replace.ptr;
    }

    PRINT("RE: n=%d, valid=%d\n",re->re_used,re->valid);

  exit:
    ResetFastBuf(&temp.b);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

int ReplaceRegex
(
    Regex_t	*re,		// valid Regex_t
    FastBuf_t	*res,		// return buffer, cleared
    ccp		src,
    int		src_len		// -1: use strlen()
)
{
    DASSERT(re);
    DASSERT(res);


    //--- sanity checks

    if (!res)
	return -ERR_INVALID_DATA;

    ClearFastBuf(res);
    res->buf[0] = 0;

    if ( !re || !re->valid )
	return -ERR_INVALID_DATA;

    if ( !src || !*src )
	return 0;

    if ( src_len < 0 )
	src_len = strlen(src);


    //--- prepare data

    enumError err = ERR_OK;
    uint count = 0;

    struct { FastBuf_t b; char space[500]; } temp;
    InitializeFastBuf(&temp,sizeof(temp));
    FastBuf_t *dest = res;

 #if DCLIB_USE_PCRE
    DASSERT(re->regex);
    enum { N_MATCH = 100 };
    int match[N_MATCH];
 #else
    enum { N_MATCH = 100 };
    regmatch_t match[N_MATCH];
 #endif


    //--- exec RE

    uint e;
    RegexElem_t *elem;
    for ( e = 0, elem = re->re_list; e < re->re_used; e++, elem++ )
    {
	uint pos = 0, copied = 0;
	while( pos < src_len )
	{
	    memset(match,0,sizeof(match));

	 #if DCLIB_USE_PCRE
	    int stat = pcre_exec(re->regex,0,src,src_len,pos,0,match,N_MATCH);
	    noPRINT("RE: stat=%2d : %2d %2d : %2d %2d : %2d %2d\n",
		    stat, match[0],match[1], match[2],match[3], match[4],match[5] );
	    if ( stat < 0 )
	    {
		if ( stat == PCRE_ERROR_NOMATCH )
		    break;
		err = -ERR_ERROR;
		goto exit;
	    }
	    const uint match_beg = match[0];
	    const uint match_end = match[1];

	 #else

	    int stat = regexec(&elem->regex,src+pos,N_MATCH,match,0);
	    PRINT("RE: pos=%2d, stat=%2d : %2d %2d : %2d %2d : %2d %2d\n",
		    pos, stat, match[0].rm_so,match[0].rm_eo,
		    match[1].rm_so,match[1].rm_eo, match[2].rm_so,match[2].rm_eo );
	    if (stat)
	    {
		if ( stat == REG_NOMATCH )
		    break;
		err = -ERR_ERROR;
		goto exit;
	    }
	    const uint match_beg = match[0].rm_so + pos;
	    const uint match_end = match[0].rm_eo + pos;

	 #endif

	    if ( match_beg > copied )
		AppendFastBuf(dest,src+copied,match_beg-copied);
	    copied = match_end;

	    RegexReplace_t *repl = elem->repl;
	    RegexReplace_t *repl_end = repl + elem->repl_used;
	    for ( ; repl < repl_end; repl++ )
	    {
		AppendMemFastBuf(dest,repl->str);
	     #if DCLIB_USE_PCRE
	     #error
	     #else
		if ( repl->ref >= 0 && repl->ref < N_MATCH )
		{
		    const regmatch_t *m = match + repl->ref;
		    if ( m->rm_so >= 0 )
			AppendFastBuf( dest,
				src + m->rm_so + pos,
				m->rm_eo - m->rm_so );
		}
	     #endif
	    }

	    pos = match_end + ( match_beg == match_end );
	    count++;

	    if (!elem->global)
		break;
	}

	if ( src_len > copied )
	    AppendFastBuf(dest,src+copied,src_len-copied);

	PRINT("DEST: %s\n",GetFastBufString(dest));
	src = GetFastBufString(dest);
	src_len = GetFastBufLen(dest);
	dest = dest == res ? &temp.b : res;
	ClearFastBuf(dest);
    }

  exit:;
    AssignFastBuf(res,src,src_len);
    return err < 0 ? err : count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

