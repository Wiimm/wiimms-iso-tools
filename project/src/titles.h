
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         https://wit.wiimm.de/                           *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit https://wit.wiimm.de/ for project details and sources.          *
 *                                                                         *
 *   Copyright (c) 2009-2017 by Dirk Clemens <wiimm@wiimm.de>              *
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

#ifndef WIT_TITELS_H
#define WIT_TITELS_H 1

#include "dclib/dclib-types.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      docu                       ///////////////
///////////////////////////////////////////////////////////////////////////////
#if 0

//---------------------------------------------------------------------------
//
//  The title data base is a list of pointers. Each pointer points to a
//  dynamic allocated memory location structured by this:
//
//      6 bytes : For the ID. The ID have 1 to 6 characters padded with NULL.
//      1 byte  : A NULL character (needed by strcmp())
//    >=1 byte  : The title, NULL terminated.
//
//  The list is orderd by the ID which is unique in the data base.
//
//  While inserting an ID an existing entry will be removed. Additional all
//  entries that begins with the new ID are removed.
//
//  While looking up for an ID, the ID itself is searched. If not found an
//  abbreviaton for that ID is searched. Binary searching is used.
//
//---------------------------------------------------------------------------

#endif
//
///////////////////////////////////////////////////////////////////////////////
///////////////                   declarations                  ///////////////
///////////////////////////////////////////////////////////////////////////////

extern int title_mode;

extern struct StringList_t * first_title_fname;
extern struct StringList_t ** append_title_fname;


///////////////////////////////////////////////////////////////////////////////

typedef struct ID_t
{
	char id[7];		// the ID padded with at least 1 NULL
	char title[2];		// the title

} ID_t;

///////////////////////////////////////////////////////////////////////////////

typedef struct ID_DB_t
{
	ID_t ** list;		// pointer to the title field
	int used;		// number of used titles in the title field
	int size;		// number of allocated pointer in 'title'

} ID_DB_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 titles interface                ///////////////
///////////////////////////////////////////////////////////////////////////////

extern ID_DB_t title_db;	// title database

// file support
void InitializeTDB();
int AddTitleFile ( ccp arg, int unused );

// title lookup
ccp GetTitle ( ccp id6, ccp default_if_failed );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			select id interface		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumSelectID
{
    SEL_F_FILE	= 1,		// flag: source is file
    SEL_F_PARAM	= 2,		// flag: '=param' allowed

    SEL_ID	= 0,
    SEL_FILE	= SEL_F_FILE,
    SEL_ID_P	=	       SEL_F_PARAM,
    SEL_FILE_P	= SEL_F_FILE | SEL_F_PARAM,
    
} enumSelectID;

///////////////////////////////////////////////////////////////////////////////

typedef enum enumSelectUsed
{
    SEL_UNUSED	= 1,
    SEL_USED	= 2,
    SEL_ALL	= SEL_UNUSED | SEL_USED

} enumSelectUsed;

///////////////////////////////////////////////////////////////////////////////


extern int disable_exclude_db;	// disable exclude db at all if > 0
extern bool include_first;	// use include rules before exclude

int  AddIncludeID ( ccp arg, int select_mode );
int  AddIncludePath ( ccp arg, int unused );

int  AddExcludeID ( ccp arg, int select_mode );
int  AddExcludePath ( ccp arg, int unused );

void SetupExcludeDB();
void DefineExcludePath ( ccp path, int max_dir_depth );

bool IsExcludeActive();
bool IsExcluded ( ccp id6 );
void DumpExcludeDB();

//
///////////////////////////////////////////////////////////////////////////////
///////////////			param id6 interface		///////////////
///////////////////////////////////////////////////////////////////////////////

extern StringField_t param_id6;	 // param id6 (without wildcard '.')
extern StringField_t param_pat;	 // param pattern (with wildcard '.')

void ClearParamDB();
int AddParamID ( ccp arg, int select_mode );
int CountParamID ();
IdItem_t * FindParamID ( ccp id6 );
int DumpParamDB ( enumSelectUsed mask, bool warn );

int SetupParamDB
(
    // return the number of valid parameters or NULL on error

    ccp		default_param,		// not NULL: use this if no param is defined
    bool	warn,			// print a warning if no param is defined
    bool	allow_arg		// allow arguments '=arg'
);

struct WBFS_t;

IdItem_t * CheckParamSlot
(
    // return NULL if no disc at slot found or disc not match or disabled
    // or a pointer to the string item

    struct WBFS_t	* wbfs,		// valid and opened WBFS
    int			slot,		// valid slot index
    bool		open_disc,	// true: open the disc
    ccp			* ret_id6,	// not NULL: store pointer to ID6 of disc
					//   The ID6 is valid until the WBFS is closed
    ccp			* ret_title	// not NULL: store pointer to title of disc
					//   The title is searched in the title db first
					//   The title may be stored in a static buffer
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 low level interface             ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum {			// all values are well orders

	IDB_NOT_FOUND,		// id not found
	IDB_ABBREV_FOUND,	// id not found but an abbreviation
	IDB_ID_FOUND,		// id found
	IDB_EXTENSION_FOUND,	// id not found but an extended version

} TDBfind_t;

int FindID   ( ID_DB_t * db, ccp id, TDBfind_t * stat, int * num_of_matching );
int InsertID ( ID_DB_t * db, ccp id, ccp title );
int RemoveID ( ID_DB_t * db, ccp id, bool remove_extended );

void DumpIDDB ( ID_DB_t * db, FILE * f );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			system menu interface		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetSystemMenu
(
    u32		version,		// system version number
    ccp		return_if_not_found	// return value if not found
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_TITELS_H

