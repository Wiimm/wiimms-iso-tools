#!/usr/bin/env bash
# (c) Wiimm, 2017-01-04

myname="${0##*/}"
base=wwt+wit
log=$base.log
err=$base.err

export LC_ALL=C

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	This script expect as parameters names of ISO files. ISO files are PLAIN,
	CISO, WBFS, WDF, WIA or GCZ. Each source file is subject of this test suite.

	Tests:

	  * wwt ADD and EXTRACT
	    - WBFS-files with different sector sizes: 512, 1024, 2048, 4096
	    - ADD to and EXTRACT from PLAIN ISO, CISO, WDF, WIA, GCZ, WBFS
	    - ADD from PIPE

	  * wit COPY
	    - convert to PLAIN ISO, CISO, WDF, WIA, GCZ, WBFS

	Usage:  $myname [option]... iso_file...

	Options:
	  --fast       : Enter fast mode
		       => do only WDF tests and skip verify+fst+pipe+edit tests

	  --verify     : enable  verify tests (default)
	  --no-verify  : disable verify tests
	  --fst        : enable  "EXTRACT FST" tests (default)
	  --no-fst     : disable "EXTRACT FST" tests
	  --pipe       : enable  pipe tests
	  --no-pipe    : disable pipe tests (default)
	  --edit       : enable edit tests (default)
	  --no-edit    : disable edit tests

	  --wia        : enable WIA compression tests (default)
	  --no-wia     : disable WIA compression tests
	  --gcz        : enable GCZ compression tests (default)
	  --no-gcz     : disable GCZ compression tests
	  --raw        : enable raw mode
	  --no-raw     : disable raw mode (default)

	  --test       : enter command test mode
	  --test-modes : test and print conversion modes
	---EOT---
    exit 1
fi

#
#------------------------------------------------------------------------------
# check existence of needed tools

WWT=wwt
WIT=wit
WDF=wdf
[[ -f ./wwt && -x ./wwt ]] && WWT=./wwt
[[ -f ./wit && -x ./wit ]] && WIT=./wit
[[ -f ./wdf && -x ./wdf ]] && WDF=./wdf

errtool=
for tool in $WWT $WIT $WDF diff cmp
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

#
#------------------------------------------------------------------------------
# timer function

let BASE_SEC=$(date +%s)

function get_msec()
{
    echo $(($(date "+(%s-BASE_SEC)*1000+10#%N/1000000")))
}

#-----------------------------------

function print_timer()
{
    # $1 = start time
    # $2 = end time
    # $3 = name if set
    local tim ms s m h
    let tim=$2-$1
    let ms=tim%1000
    let tim=tim/1000
    let s=tim%60
    let tim=tim/60
    let m=tim%60
    let h=tim/60
    if ((h))
    then
	printf "%u:%02u:%02u.%03u hms  %s\n" $h $m $s $ms "$3"
    elif ((m))
    then
	printf "  %2u:%02u.%03u m:s  %s\n" $m $s $ms "$3"
    else
	printf "     %2u.%03u sec  %s\n" $s $ms "$3"
    fi
}

#-----------------------------------

function print_stat()
{
    printf "%-26s : " "$(printf "$@")"
}

#
#------------------------------------------------------------------------------
# setup

function f_abort()
{
    echo
    {
	msg="###  ${myname} CANCELED  ###"
	sep="############################"
	sep="$sep$sep$sep$sep"
	sep="${sep:1:${#msg}}"
	echo ""
	echo "$sep"
	echo "$msg"
	echo "$sep"
	echo ""
	echo "remove tempdir: $tempdir"
	rm -rf "$tempdir"
	sync
	echo ""
    } >&2

    sleep 1
    exit 2
}

tempdir=
trap f_abort INT TERM HUP
tempdir="$(mktemp -d ./.$base.tmp.XXXXXX)" || exit 1

WBFS_FILE=a.wbfs
WBFS="$tempdir/$WBFS_FILE"

#WIALIST=$(echo $($WIT compr | sed 's/^/wia-/'))
WIALIST=$($WIT compr | sed 's/^/wia-/')
WDFLIST=$($WIT features wdf1 wdf2 | awk '/^+/ {print $2}' | tr 'A-Z\n' 'a-z ')
MODELIST="iso $WDFLIST $WIALIST ciso gcz wbfs"
BASEMODE="wdf1"

FAST_MODELIST="$WDFLIST"
FAST_BASEMODE="wdf1"

NOVERIFY=0
NOFST=0
NOPIPE=1
NOEDIT=0
NOWIA=0
NOGCZ=0
RAW=
IO=
OPT_TEST=0
OPT_TEST_MODES=0

FST_OPT="--psel data,update,channel,ptab0"

export WIT_OPT=
export WWT_OPT=
export WWT_WBFS=

# error codes

STAT_OK=0
STAT_TEST=1
STAT_NOISO=2
STAT_DIFF=3
ERROR=10

STAT_SUCCESS=$STAT_OK

#
#------------------------------------------------------------------------------
# test function

function test_function()
{
    # $1 = short name
    # $2 = info
    # $3... = command

    ref="$1"
    print_stat " > %s" "$2"
    shift 2
    start=$(get_msec)
    #echo; echo "$@"
    ((OPT_TEST)) || "$@" || return $ERROR
    stop=$(get_msec)
    print_timer $start $stop
    return $STAT_OK
}

#
#------------------------------------------------------------------------------
# test suite

function test_suite()
{
    # $1 = valid source

    rm -rf "$tempdir"/*
    mkdir -p "$tempdir"

    local src dest count hss

    ref="-"
    ft="$($WWT FILETYPE -H -l "$1" | awk '{print $1}')" || return $ERROR
    id6="$($WWT FILETYPE -H -l "$1" | awk '{print $2}')" || return $ERROR

    printf "\n** %s %s %s\n" "$ft" "$id6" "$1"
    [[ $id6 = "-" ]] && return $STAT_NOISO

    #----- verify source

    if ((!NOVERIFY))
    then
	test_function "VERIFY" "wit VERIFY source" \
	    $WIT VERIFY -qq "$1" \
	    || return $ERROR
    fi

    #----- test WWT

    hss=512
    rm -f "$WBFS"
    test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	$WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	|| return $ERROR

    test_function "ADD-src" "wwt ADD source" \
	$WWT_ADD -p "$WBFS" "$1" \
	|| return $ERROR

    for xmode in $MODELIST
    do
	mode="${xmode%%-*}"
	#echo "|$mode|$xmode|"
	[[ $mode = wia ]] && continue
	[[ $mode = gcz ]] && ((NOGCZ)) && continue

	dest="$tempdir/image.$mode"
	test_function "EXT-$mode" "wwt EXTRACT to $mode" \
	    $WWT -qp "$WBFS" EXTRACT "$id6=$dest" --$mode \
	    || return $ERROR

	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR
	let hss*=2

	test_function "ADD-$mode" "wwt ADD from $mode" \
	    $WWT_ADD -p "$WBFS" "$dest" \
	    || return $ERROR

	[[ $mode = $BASEMODE ]] || rm -f "$dest"
    done

    test_function "CMP" "wit CMP wbfs source" \
	$WIT $RAW -ql CMP "$WBFS" "$1" \
	|| return $STAT_DIFF


    #----- test WIT copy

    count=0
    src="$tempdir/image.$BASEMODE"
    for xmode in $MODELIST
    do
	mode="${xmode%%-*}"
	[[ $xmode = ${xmode/-} ]] && compr="" || compr="--compr ${xmode#*-}"
	[[ $mode = wia ]] && ((NOWIA)) && continue
	[[ $mode = gcz ]] && ((NOGCZ)) && continue
	[[ $mode = wbfs && $RAW != "" ]] && continue

	dest="$tempdir/copy.$xmode.$mode"

	test_function "COPY-$xmode" "wit COPY to $xmode" \
	    $WIT_CP "$src" "$dest" --$mode $compr \
	    || return $ERROR

	((count++)) && rm -f "$src"
	src="$dest"

    done

    test_function "CMP" "wit CMP copied source" \
	$WIT $RAW -ql CMP "$dest" "$1" \
	|| return $STAT_DIFF

    rm -f "$dest"


    #----- test WIT extract

    if ((!NOFST))
    then
	src="$tempdir/image.$BASEMODE"
	dest="$tempdir/fst"

	test_function "EXTR-FST0" "wit EXTRACT source" \
	    $WIT_CP "$1" "$dest/0" --fst -F :wit $FST_OPT \
	    || return $ERROR

	test_function "EXTR-FST1" "wit EXTRACT copy" \
	    $WIT_CP "$src" "$dest/1" --fst -F :wit $FST_OPT \
	    || return $ERROR

	test_function "DIF-FST1" "DIFF fst/0 fst/1" \
	    diff -rq "$dest/0" "$dest/1" \
	    || return $STAT_DIFF

	rm -rf "$dest/0"

	test_function "GEN-ISO" "wit COMPOSE" \
	    $WIT_CP "$dest/1" -D "$dest/a.wdf" --wdf --region=file \
	    || return $ERROR

	hss=512
	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR

	test_function "ADD-FST" "wwt ADD FST" \
	    $WWT_ADD -p "$WBFS" "$dest/1" --region=file \
	    || return $ERROR

	test_function "CMP" "wit CMP 2x composed" \
	    $WIT $RAW -ql CMP "$WBFS" "$dest/a.wdf" \
	    || return $STAT_DIFF

	test_function "EXTR-FST2" "wit EXTRACT" \
	    $WIT_CP "$dest/a.wdf" "$dest/2" --fst -F :wit $FST_OPT \
	    || return $ERROR

	#diff -rq "$dest/1" "$dest/2"
	((OPT_TEST)) || find "$dest" -name tmd.bin	-type f -exec rm {} \;
	((OPT_TEST)) || find "$dest" -name ticket.bin	-type f -exec rm {} \;
	find "$dest" -regextype posix-egrep \
		-regex '.*(setup.(txt|sh|bat)|align-files.txt)' \
		-type f -exec rm {} \;

	test_function "DIF-FST2" "DIFF fst/1 fst/2" \
	    diff -rq "$dest/1" "$dest/2" \
	    || return $STAT_DIFF

	rm -rf "$dest"
    fi

    #----- test WWT pipe

    if (( !NOPIPE && !OPT_TEST ))
    then

	hss=1024
	rm -f "$WBFS"
	test_function "INIT-$hss" "wwt INIT wbfs hss=$hss" \
	    $WWT INIT -q --force --size=20g --hss=$hss "$WBFS" \
	    || return $ERROR

	ref="ADD pipe"
	if ! $WDF +cat "$tempdir/image.$BASEMODE" |
	    test_function "ADD-pipe" "wwt ADD $BASEMODE from pipe" \
		$WWT_ADD -p "$WBFS" - --psel=DATA
	then
	    ref="NO-PIPE"
	    return $STAT_SUCCESS
	fi

	test_function "CMP" "wit CMP wbfs source" \
	    $WIT $RAW -ql CMP "$WBFS" "$1" --psel=DATA \
	    || return $STAT_DIFF

    fi

    #----- test EDIT

    if (( !NOEDIT && !OPT_TEST ))
    then
	src="$tempdir/image.$BASEMODE"
	opt="--id KKK"

	test_function "PATCH-WBFS" "wit PATCH to WBFS" \
	    $WIT -ql $opt COPY "$src" "$tempdir/pat.wbfs" \
	    || return $STAT_DIFF

	test_function "PATCH-WBFS" "wit PATCH/ALIGN to WDF2" \
	    $WIT -ql $opt COPY --align-wdf 1m "$src" "$tempdir/pat.wdf2" \
	    || return $STAT_DIFF

	test_function "CMP-PATCH" "wit CMP patch" \
	    $WIT -ql CMP "$tempdir/pat.wbfs" "$tempdir/pat.wdf2" \
	    || return $STAT_DIFF

	#---

	test_function "COPY-WDF2" "wit COPY to WDF2" \
	    $WIT -ql COPY "$src" "$tempdir/edit.wdf2" \
	    || return $STAT_DIFF

	test_function "EDIT-WDF2" "wit EDIT WDF2" \
	    $WIT -q EDIT "$tempdir/edit.wdf2" $opt \
	    || return $STAT_DIFF

	test_function "CMP-EDIT" "wit CMP EDIT" \
	    $WIT -ql CMP "$tempdir/pat.wdf2" "$tempdir/edit.wdf2" \
	    || return $STAT_DIFF

	test_function "WDF-CMP-EDIT" "wdf CMP EDIT" \
	    $WDF +cmp "$tempdir/pat.wdf2" "$tempdir/edit.wdf2" \
	    || return $STAT_DIFF
    fi


    #----- all tests done

    ref=-
    return $STAT_SUCCESS
}

#
#------------------------------------------------------------------------------
# print header

{
    sep="-----------------------------------"
    printf "\n\f\n%s%s\n\n" "$sep" "$sep"
    date '+%F %T'
    echo
    $WWT --version
    $WIT --version
    $WDF --version
    echo
    echo "PARAM: $*"
    echo
} | tee -a $log $err

#
#------------------------------------------------------------------------------
# main loop

opts=1

while (($#))
do
    src="$1"
    shift

    if [[ $src == --fast ]]
    then
	NOVERIFY=1
	NOFST=1
	NOPIPE=1
	NOEDIT=1
	MODELIST="$FAST_MODELIST"
	BASEMODE="$FAST_BASEMODE"
	((opts++)) || printf "\n"
	printf "## --fast : check only %s, --no-fst --no-pipe --no-edit\n" "$MODELIST"
	continue
    fi

    if [[ $src == --verify ]]
    then
	NOVERIFY=1
	((opts++)) || printf "\n"
	printf "## --verify : verify tests enabled\n"
	continue
    fi

    if [[ $src == --no-verify || $src == --noverify ]]
    then
	NOVERIFY=1
	((opts++)) || printf "\n"
	printf "## --no-verify : verify tests disabled\n"
	continue
    fi

    if [[ $src == --fst ]]
    then
	NOFST=0
	((opts++)) || printf "\n"
	printf "## --fst : fst tests enabled\n"
	continue
    fi

    if [[ $src == --no-fst || $src == --nofst ]]
    then
	NOFST=1
	((opts++)) || printf "\n"
	printf "## --no-fst : fst tests diabled\n"
	continue
    fi

    if [[ $src == --pipe ]]
    then
	NOPIPE=0
	((opts++)) || printf "\n"
	printf "## --pipe : pipe tests enabled\n"
	continue
    fi

    if [[ $src == --no-pipe || $src == --nopipe ]]
    then
	NOPIPE=1
	((opts++)) || printf "\n"
	printf "## --no-pipe : pipe tests disabled\n"
	continue
    fi

    if [[ $src == --edit ]]
    then
	NOEDIT=0
	((opts++)) || printf "\n"
	printf "## --edit : edit tests enabled\n"
	continue
    fi

    if [[ $src == --no-edit || $src == --noedit ]]
    then
	NOEDIT=1
	((opts++)) || printf "\n"
	printf "## --no-edit : edit tests disabled\n"
	continue
    fi

    if [[ $src == --wia ]]
    then
	NOWIA=0
	((opts++)) || printf "\n"
	printf "## --wia : compress WIA tests enabled\n"
	continue
    fi

    if [[ $src == --no-wia ]]
    then
	NOWIA=1
	((opts++)) || printf "\n"
	printf "## ---no-wia : compress WIA tests disabled\n"
	continue
    fi

    if [[ $src == --gcz ]]
    then
	NOGCZ=0
	((opts++)) || printf "\n"
	printf "## --gcz : compress GCZ tests enabled\n"
	continue
    fi

    if [[ $src == --no-gcz ]]
    then
	NOGCZ=1
	((opts++)) || printf "\n"
	printf "## ---no-gcz : compress GCZ tests disabled\n"
	continue
    fi

    if [[ $src == --raw ]]
    then
	RAW=--raw
	((opts++)) || printf "\n"
	printf "## --raw : raw mode enabled.\n"
	continue
    fi

    if [[ $src == --no-raw || $src == --noraw ]]
    then
	RAW=
	((opts++)) || printf "\n"
	printf "## --no-raw : raw mode disabled.\n"
	continue
    fi

    if [[ $src == --test ]]
    then
	OPT_TEST=1
	STAT_SUCCESS=$STAT_TEST
	((opts++)) || printf "\n"
	printf "## --test : test mode enabled\n"
	continue
    fi

    if [[ $src == --test-modes || $src == --testmodes ]]
    then
	STAT_SUCCESS=$STAT_TEST
	((opts++)) || printf "\n"
	printf "## --test-modes : list of modes:\n"
	for xmode in $MODELIST
	do
	    [[ $xmode = ${xmode/.} ]] && compr="" || compr="--compr ${xmode#*.}"
	    mode="${xmode%.*}"
	    echo "# $mode $compr"
	done
	continue
    fi

    #---- hidden options

    if [[ $src == --io ]]
    then
	IO="--io $( echo "$1" | tr -cd 0-9 )"
	shift
	((opts++)) || printf "\n"
	printf "## Define option : $IO\n"
	continue
    fi


    opts=0
    WWT_ADD="$WWT ADD $RAW $IO -q"
    WIT_CP="$WIT COPY $RAW $IO -q"

    total_start=$(get_msec)
    test_suite "$src"
    stat=$?
    total_stop=$(get_msec)

    print_stat " * TOTAL TIME:"
    print_timer $total_start $total_stop

    case "$stat" in
	$STAT_OK)	stat="OK" ;;
	$STAT_TEST)	stat="TEST" ;;
	$STAT_NOISO)	stat="NO-ISO" ;;
	$STAT_DIFF)	stat="DIFFER" ;;
	*)		stat="ERROR" ;;
    esac

    printf "%s %-6s %-8s %-6s %s\n" $(date +%T) "$stat" "$ref" "$id6" "$src" | tee -a $log

done 2>&1 | tee -a $err

#
#------------------------------------------------------------------------------
# term

rm -rf "$tempdir/"
exit 0

