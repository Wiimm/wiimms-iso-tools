#!/usr/bin/env bash

    #####################################################################
    ##                 __            __ _ ___________                  ##
    ##                 \ \          / /| |____   ____|                 ##
    ##                  \ \        / / | |    | |                      ##
    ##                   \ \  /\  / /  | |    | |                      ##
    ##                    \ \/  \/ /   | |    | |                      ##
    ##                     \  /\  /    | |    | |                      ##
    ##                      \/  \/     |_|    |_|                      ##
    ##                                                                 ##
    ##                        Wiimms ISO Tools                         ##
    ##                      https://wit.wiimm.de/                       ##
    ##                                                                 ##
    #####################################################################
    ##                                                                 ##
    ##   This file is part of the WIT project.                         ##
    ##   Visit https://wit.wiimm.de/ for project details and sources.   ##
    ##                                                                 ##
    ##   Copyright (c) 2009-2017 by Dirk Clemens <wiimm@wiimm.de>      ##
    ##                                                                 ##
    #####################################################################
    ##                                                                 ##
    ##   This file copies the cygwin tools and DLLs into the binary    ##
    ##   directory to prepare the distribution.                        ##
    ##                                                                 ##
    #####################################################################


#------------------------------------------------------------------------------
# setup

CYGWIN_DIR="@@CYGWIN-DIR@@"
CYGWIN_TOOLS="@@CYGWIN-TOOLS@@"
BIN_FILES="@@BIN-FILES@@"
DISTRIB_PATH="@@DISTRIB-PATH@@"

BIN_PATH="$DISTRIB_PATH/bin"
mkdir -p "$BIN_PATH" || exit 1

#------------------------------------------------------------------------------
# copy cygwin tools

for tool in $CYGWIN_TOOLS
do
    cp --preserve=time "$CYGWIN_DIR/$tool.exe" "$BIN_PATH" || exit 1
done

#------------------------------------------------------------------------------
# copy wit tools

for tool in $BIN_FILES
do
    if [[ -s ./$tool.exe ]]
    then
	CYGWIN_TOOLS="$CYGWIN_TOOLS $tool"
	ln -f ./$tool.exe "$BIN_PATH" || exit 1
    fi
done

#------------------------------------------------------------------------------
# copy needed cygwin dlls

for tool in $CYGWIN_TOOLS
do
    ldd "$BIN_PATH/$tool.exe" | grep -F "=> $CYGWIN_DIR/" | awk '{print $1}'
done | sort | uniq | while read dll
do
    cp --preserve=time "$CYGWIN_DIR/$dll" "$BIN_PATH" || exit 1
done

#------------------------------------------------------------------------------
# done

exit 0

