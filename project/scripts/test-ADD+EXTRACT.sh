#!/usr/bin/env bash

myname="${0##*/}"
base=add+extract
log=$base.log
err=$base.err

usewitcmp=0
if [[ $1 = --witcmp ]]
then
    usewitcmp=1
    shift
fi

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	$myname : Test adding and extracting of ISO images with 'wwt''

	This script expect as parameters names of ISO or WDF+ISO files. These
	files are subject of adding and extracting with a temporary WBFS file.

	The test does the following (not in this order):
	  1.) Adding the ISO image with
	      a) --io=0  (use file open() and file functions)
	      b) --io=3  (use file fopen() and stream functions)
	  2.) Adding the WDF image with
	      a) --io=0
	      b) --io=3
	  3.) extracting the image with
	      a) --io=0 --iso
	      b) --io=3 --iso
	      c) --io=0 --wdf
	      d) --io=3 --wdf
	  4.) compare all exported WDF and ISO among themselves.
	  5.) compare one ISO against the source ISO
	      this may fail if the source ISO is not scrubbed.

	For each file one status line is *appended* at the end of the
	file '$log' in the following format:
	    STATUS JOB ID6 filename
	JOB is an shortcut which job failed and ID6 is the ID of the ISO image.
	STATUS is one of:
	    'NO-ISO'  : source is not an ISO.
	    'ERR'     : a command exited with an error.
	    'ERR-WDF  : comparison of WDF file failed.
	    'ERR-ISO  : comparison of ISO file failed.
	    'ERR-W+I  : comparison of one WDF against one ISO failed.
	    'DIFF'    : comparison of one output ISO against the source ISO failed.
	    'OK'      : Source and all destination files are identical.
	A full log is appended to the file '$err' ans to stdout.

	The temporary files are written to a temporary subdirectory of the current
	working directory (hoping that here is enough space). The directory will be
	is deleted at the end. The signals INT, TERM and HUP are handled.

	---EOT---

    echo "usage: $myname [--witcmp] iso_file..." >&2
    echo
    exit 1
fi

#------------------------------------------------------------------------------

WWT=wwt
WIT=wit
WDFCAT=wdf-cat
[[ -x ./wwt ]] && WWT=./wwt
[[ -x ./wit ]] && WIT=./wit
[[ -x ./wdf-cat ]] && WDFCAT=./wdf-cat

errtool=
for tool in $WWT $WIT $WDFCAT cmp
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

CMP=cmp
((usewitcmp)) && CMP="wit cmp"

#------------------------------------------------------------------------------

let BASE_SEC=$(date +%s)

function get_msec()
{
    echo $(($(date "+(%s-BASE_SEC)*1000+10#%N/1000000")))
}

#------------------------------------------------------------------------------

function print_timer()
{
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

#------------------------------------------------------------------------------

function print_stat()
{
    printf "%-35s : " "$(printf "$@")"
}

#------------------------------------------------------------------------------

function f_abort()
{
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
        echo ""
    } >&2

    sleep 1
    exit 2
}

#------------------------------------------------------------------------------
# setup

tempdir=
trap f_abort INT TERM HUP
tempdir="$(mktemp -d ./.$base.tmp.XXXXXX)" || exit 1

wbfs_file=a.wbfs
wbfs="$tempdir/$wbfs_file"

wbfs2_file=b.wbfs
wbfs2="$tempdir/$wbfs2_file"

#------------------------------------------------------------------------------
# error codes

STAT_OK=0
STAT_DIFF=1
STAT_NO_ISO=2

ERROR=10
ERR_WDF=11
ERR_ISO=12
ERR_WDF_ISO=13

#------------------------------------------------------------------------------
# tests

function test_it()
{
    total_start=$(get_msec)

    rm -f "$tempdir"/*.iso "$tempdir"/*.wdf "$tempdir"/*.wbfs
    mkdir -p "$tempdir"

    ref="-"
    ft="$($WWT FILETYPE -H -l "$1" | awk '{print $1}')" || return $ERROR
    id6="$($WWT FILETYPE -H -l "$1" | awk '{print $2}')" || return $ERROR
    [[ $id6 = "-" ]] && return $STAT_NO_ISO

    printf "\n** %s %s %s\n" "$ft" "$id6" "$1"

    #------------ JOB: ADD SOURCE (ISO or WDF) --io=0

    ref="init"
    print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
    $WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
    stop=$(get_msec); print_timer $start $stop $ref

    [[ $ft = ISO ]] && ref="AI0s" || ref="AW0s"
    print_stat " > %-4s add source($ft) --io=0" "$ref"; start=$(get_msec)
    $WWT -q -p "$wbfs" add --io=0 "$1" || return $ERROR
    stop=$(get_msec); print_timer $start $stop $ref
    
    #------------ JOB: EXTRACT ISO --io=0

    ref="XI0"
    print_stat " > %-4s extract --iso --io=0" "$ref"; start=$(get_msec)
    $WWT -q -p "$wbfs" -d "$tempdir" extract --iso --io=0 "$id6=ref.iso" || return $ERROR
    stop=$(get_msec); print_timer $start $stop $ref

    if ((!fast))
    then

	#------------ JOB: EXTRACT WBFS --io=0

	ref="XB0"
	print_stat " > %-4s extract --wbfs --io=0" "$ref"; start=$(get_msec)
	$WWT -q -p "$wbfs" -d "$tempdir" extract --wbfs --io=0 "$id6=$wbfs2_file" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	#------------ JOB: EXTRACT WDF --io=0

	ref="XW0"
	print_stat " > %-4s extract --wdf --io=0" "$ref"; start=$(get_msec)
	$WWT -q -p "$wbfs2" -d "$tempdir" extract --wdf --io=0 "$id6=ref.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	print_stat " > %-4s $CMP iso wdf" "$ref"; start=$(get_msec)
	if ((usewitcmp))
	then
	    $WIT -q cmp "$tempdir/ref.wdf" "$tempdir/ref.iso" || return $ERR_WDF_ISO
	else
	    $WDFCAT "$tempdir/ref.wdf" | cmp --quiet "$tempdir/ref.iso" || return $ERR_WDF_ISO
	fi
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: ADD ISO --io=3

	ref="init"
	print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
	$WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	ref="AI3"
	print_stat " > %-4s add iso --io=3" "$ref"; start=$(get_msec)
	$WWT -q -p "$wbfs" add --io=3 "$tempdir/ref.iso" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	#------------ JOB: EXTRACT ISO --io=3

	ref="XI3"
	print_stat " > %-4s extract --iso --io=3" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --iso --io=3 "$id6=temp.iso" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	print_stat " > %-4s $CMP iso iso" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.iso" "$tempdir/temp.iso" || return $ERR_ISO
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: EXTRACT WDF --io=3

	ref="XW3"
	print_stat " > %-4s extract --wdf --io=3" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --wdf --io=3 "$id6=temp.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	print_stat " > %-4s $CMP wdf wdf" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.wdf" "$tempdir/temp.wdf" || return $ERR_WDF
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: ADD WDF or ISO --io=0

	ref="init"
	print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
	$WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	if [[ $ft = ISO ]]
	then
	    # add iso already done -> add wdf
	    ref="AW0"
	    print_stat " > %-4s add wdf --io=0" "$ref"; start=$(get_msec)
	    $WWT -q -p "$wbfs" add --io=0 "$tempdir/ref.wdf" || return $ERROR
	    stop=$(get_msec); print_timer $start $stop $ref
	else
	    # add wdf already done -> add iso
	    ref="AI0"
	    print_stat " > %-4s add iso --io=0" "$ref"; start=$(get_msec)
	    $WWT -q -p "$wbfs" add --io=0 "$tempdir/ref.iso" || return $ERROR
	    stop=$(get_msec); print_timer $start $stop $ref
	fi

	#----- no job, just extract and compare

	print_stat " > %-4s extract --wdf --io=0" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --wdf --io=0 "$id6=temp.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	print_stat " > %-4s $CMP wdf wdf" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.wdf" "$tempdir/temp.wdf" || return $ERR_WDF
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: ADD WDF --io=3

	ref="init"
	print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
	$WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	ref="AW3"
	print_stat " > %-4s add wdf --io=3" "$ref"; start=$(get_msec)
	$WWT -q -p "$wbfs" add --io=3 "$tempdir/ref.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop $ref

	#----- no job, just extract and compare

	print_stat " > %-4s extract --wdf --io=0" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --wdf --io=0 "$id6=temp.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	print_stat " > %-4s $CMP wdf wdf" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.wdf" "$tempdir/temp.wdf" || return $ERR_WDF
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: ADD ISO --io=0 pipe

#DEL	ref="init"
#DEL	print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
#DEL	$WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
#DEL	stop=$(get_msec); print_timer $start $stop
#DEL
#DEL	ref="AI0p"
#DEL	print_stat " > %-4s add iso --io=0 pipe" "$ref"; start=$(get_msec)
#DEL	cat "$tempdir/ref.iso" | $WWT -q -p "$wbfs" add --io=0 - || return $ERROR
#DEL	stop=$(get_msec); print_timer $start $stop $ref

	#----- no job, just extract and compare

	print_stat " > %-4s extract --wdf --io=0" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --wdf --io=0 "$id6=temp.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	print_stat " > %-4s $CMP wdf wdf" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.wdf" "$tempdir/temp.wdf" || return $ERR_WDF
	stop=$(get_msec); print_timer $start $stop

	#------------ JOB: ADD ISO --io=3 pipe

#DEL	ref="init"
#DEL	print_stat " > %s $sector_size" "$ref"; start=$(get_msec)
#DEL	$WWT -q init $sector_size --force "$wbfs" --size 12 || return $ERROR
#DEL	stop=$(get_msec); print_timer $start $stop
#DEL
#DEL	ref="AI3p"
#DEL	print_stat " > %-4s add iso --io=3 pipe" "$ref"; start=$(get_msec)
#DEL	cat "$tempdir/ref.iso" | $WWT -q -p "$wbfs" add --io=3 - || return $ERROR
#DEL	stop=$(get_msec); print_timer $start $stop $ref

	#----- no job, just extract and compare

	print_stat " > %-4s extract --wdf --io=0" "$ref"; start=$(get_msec)
	$WWT -qo -p "$wbfs" -d "$tempdir" extract --wdf --io=0 "$id6=temp.wdf" || return $ERROR
	stop=$(get_msec); print_timer $start $stop

	print_stat " > %-4s $CMP wdf wdf" "$ref"; start=$(get_msec)
	$CMP --quiet "$tempdir/ref.wdf" "$tempdir/temp.wdf" || return $ERR_WDF
	stop=$(get_msec); print_timer $start $stop

    fi

    #------------ compare with source

    ref="cmp"
    print_stat " > %-4s $CMP iso source($ft)" "$ref"; start=$(get_msec)
    if [[ $ft = ISO ]] || ((usewitcmp))
    then
	$CMP --quiet "$tempdir/ref.iso" "$1" || return $STAT_DIFF
    else
	$WDFCAT "$1" | cmp --quiet "$tempdir/ref.iso" || return $STAT_DIFF
    fi
    stop=$(get_msec); print_timer $start $stop
    
    ref="+"
    return $STAT_OK
}

#------------------------------------------------------------------------------
# print header

{
    sep="--------------------------------"
    printf "\n\f\n%s%s\n\n" "$sep" "$sep"
    date '+%F %T'
    $WWT --version
    echo
} | tee -a $log $err

#------------------------------------------------------------------------------
# main loop

sector_size=
fast=0

while (($#))
do
    if [[ $1 == --sector-size ]]
    then
	sector_size="--sector-size $2"
	echo "NEW OPTION: $sector_size"
	shift
	shift
	continue
    fi

    if [[ $1 == --fast ]]
    then
	fast=1
	shift
	continue
    fi

    total_start=$(get_msec)
    test_it "$1"
    stat=$?
    total_stop=$(get_msec)

    print_stat " * TOTAL TIME:"
    print_timer $total_start $total_stop

    case "$stat" in
	$STAT_OK)	stat="OK" ;;
	$STAT_DIFF)	stat="DIFF" ;;
	$STAT_NO_ISO)	stat="NO-ISO" ;;
	$ERR_WDF)	stat="ERR-WDF" ;;
	$ERR_ISO)	stat="ERR-ISO" ;;
	$ERR_WDF_ISO)	stat="ERR-W+I" ;;
	*)		stat="ERROR" ;;
    esac
	
    printf "%-7s %-4s %-6s %s\n" "$stat" "$ref" "$id6" "$1" | tee -a $log
    shift

done 2>&1 | tee -a $err

#------------------------------------------------------------------------------

rm -rf "$tempdir"
exit 0

