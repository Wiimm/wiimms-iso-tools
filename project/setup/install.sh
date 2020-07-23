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
    ##   This file installs the distribution.                          ##
    ##                                                                 ##
    #####################################################################


#------------------------------------------------------------------------------
# sudo?

try_sudo=1
if [[ $1 = --no-sudo ]]
then
    try_sudo=0
    shift
else
    uid="$(id -u 2>/dev/null)" || try_sudo=0
    [[ $uid = 0 ]] && try_sudo=0
fi

if ((try_sudo))
then
    echo "*** need root privileges to install => try sudo ***"
    sudo "$0" --no-sudo "$@"
    exit $?
fi

#------------------------------------------------------------------------------
# settings

BASE_PATH="@@INSTALL-PATH@@"
BIN_PATH="$BASE_PATH/bin"
SHARE_PATH="@@SHARE-PATH@@"

BIN_FILES="@@BIN-FILES@@"
WDF_LINKS="@@WDF-LINKS@@"
SHARE_FILES="@@SHARE-FILES@@"

INST_FLAGS="-p"

#------------------------------------------------------------------------------
# make?

make=0
if [[ $1 = --make ]]
then
    # it's called from make
    make=1
    shift
fi

#------------------------------------------------------------------------------
echo "*** install binaries to $BIN_PATH"

for f in $BIN_FILES
do
    [[ -f bin/$f ]] || continue
    mkdir -p "$BIN_PATH"
    install $INST_FLAGS bin/$f "$BIN_PATH/$f"
done

#------------------------------------------------------------------------------
echo "*** create wdf links"

for f in $WDF_LINKS
do
    [[ -f "$BIN_PATH/wdf"  ]] && ln -f "$BIN_PATH/wdf"  "$BIN_PATH/$f"
done

#------------------------------------------------------------------------------
echo "*** install share files to $SHARE_PATH"

for f in $SHARE_FILES
do
    mkdir -p "$SHARE_PATH"
    install $INST_FLAGS -m 644 share/$f "$SHARE_PATH/$f"
done

if [[ -f ./load-titles.sh ]]
then
    mkdir -p "$SHARE_PATH"
    install $INST_FLAGS -m 755 ./load-titles.sh "$SHARE_PATH/load-titles.sh"
fi

#------------------------------------------------------------------------------
# load titles

((make)) || ./load-titles.sh

#------------------------------------------------------------------------------

exit 0

