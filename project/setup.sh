#!/usr/bin/env bash

revision=0

[[ -s revision.sh ]] && . revision.sh

if [[ -d .svn ]] && which svn >/dev/null 2>&1
then
    revision="$( svn info . | awk '$1=="Revision:" {print $2}' )"
    if which svnversion >/dev/null 2>&1
    then
	rev="$(svnversion|sed 's/.*://')"
	(( ${revision//[!0-9]/} < ${rev//[!0-9]/} )) && revision=$rev
    fi
    echo "revision=$revision" > revision.sh
fi

revision_num="${revision//[!0-9]/}"
revision_next=$revision_num
[[ $revision = $revision_num ]] || let revision_next++

tim=($(date '+%s %Y-%m-%d %T'))
defines=

have_fuse=0
[[ $NO_FUSE != 1 && -r /usr/include/fuse.h || -r /usr/local/include/fuse.h ]] \
	&& have_fuse=1

have_md5=0
[[ -r /usr/include/openssl/md5.h ]] && have_md5=1

have_sha=0
[[ -r /usr/include/openssl/sha.h ]] && have_sha=1

have_zlib=0
if [[ $NO_ZLIB != 1 && -r /usr/include/zlib.h || -r /usr/local/include/zlib.h ]]
then
    have_zlib=1
    defines="$defines -DHAVE_ZLIB=1"
fi

if [[ $M32 = 1 ]]
then
    force_m32=1
    have_fuse=0
    xflags="-m32"
    defines="$defines -DFORCE_M32=1"
else
    force_m32=0
    xflags=
fi

[[ -r /usr/include/bits/fcntl.h ]] \
	&& grep -qw fallocate /usr/include/bits/fcntl.h \
	&& defines="$defines -DHAVE_FALLOCATE=1"

[[ -r /usr/include/fcntl.h ]] \
	&& grep -qw posix_fallocate /usr/include/fcntl.h \
	&& defines="$defines -DHAVE_POSIX_FALLOCATE=1"

[[ -r /usr/include/linux/fiemap.h ]] \
	&& grep -qw fiemap_extent /usr/include/linux/fiemap.h \
	&& defines="$defines -DHAVE_FIEMAP=1"

[[ $STATIC = 1 ]] || STATIC=0

#--------------------------------------------------

GCC_VERSION="$( gcc --version | head -n1 | sed 's/([^)]*)//'|awk '{print $2}' )"
[[ $GCC_VERSION > 7 ]] && xflags+=" -fdiagnostics-color=always"

gcc $xflags -E -DPRINT_SYSTEM_SETTINGS system.c \
	| awk -F= '/^result_/ {printf("%s := %s\n",substr($1,8),gensub(/"/,"","g",$2))}' \
	> Makefile.setup

#--------------------------------------------------

INSTALL_PATH=/usr/local

if [[ -d $INSTALL_PATH/bin ]]
then
    HAVE_INSTBIN=1
    INSTBIN=$INSTALL_PATH/bin
else
    HAVE_INSTBIN=0
    INSTBIN=/tmp
fi

if [[ -d $INSTALL_PATH/bin32 ]]
then
    HAVE_INSTBIN_32=1
    INSTBIN_32=$INSTALL_PATH/bin32
else
    HAVE_INSTBIN_32=0
    INSTBIN_32=/tmp
fi

if [[ -d $INSTALL_PATH/bin64 ]]
then
    HAVE_INSTBIN_64=1
    INSTBIN_64=$INSTALL_PATH/bin64
elif [[ -d $INSTALL_PATH/bin-x86_64 ]]
then
    HAVE_INSTBIN_64=1
    INSTBIN_64=$INSTALL_PATH/bin-x86_64
else
    HAVE_INSTBIN_64=0
    INSTBIN_64=/tmp
fi

#--------------------------------------------------

cat <<- __EOT__ >> Makefile.setup

	REVISION	:= $revision
	REVISION_NUM	:= $revision_num
	REVISION_NEXT	:= $revision_next
	BINTIME		:= ${tim[0]}
	DATE		:= ${tim[1]}
	TIME		:= ${tim[2]}

	GCC_VERSION	:= $GCC_VERSION
	FORCE_M32	:= $force_m32
	HAVE_FUSE	:= $have_fuse
	HAVE_ZLIB	:= $have_zlib
	HAVE_MD5	:= $have_md5
	HAVE_SHA	:= $have_sha
	STATIC		:= $STATIC
	XFLAGS		+= $xflags
	DEFINES1	:= $defines

	HAVE_INSTBIN	:= $HAVE_INSTBIN
	HAVE_INSTBIN_32	:= $HAVE_INSTBIN_32
	HAVE_INSTBIN_64	:= $HAVE_INSTBIN_64

	INSTALL_PATH	:= $INSTALL_PATH
	INSTBIN		:= $INSTBIN
	INSTBIN_32	:= $INSTBIN_32
	INSTBIN_64	:= $INSTBIN_64

	__EOT__

