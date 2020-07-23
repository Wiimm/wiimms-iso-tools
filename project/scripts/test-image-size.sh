#!/usr/bin/env bash
# (c) Wiimm, 2010-09-24

myname="${0##*/}"
base=image-size
log=$base.log
err=$base.err
db=$base.db

WIT=wit
[[ -f ./wit && -x ./wit ]] && WIT=./wit

WDF=wdf
[[ -f ./wdf && -x ./wdf ]] && WDF=./wdf

export LC_ALL=C
C1_LIST=($($WIT compr))
C2_LIST=("")
C3_LIST=("")
PSEL=
IO=

#
#------------------------------------------------------------------------------
# built in help

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	This script expect as parameters names of ISO files. ISO files are PLAIN,
	WDF, WIA, CISO, WBFS or FST. Each source file is subject of this test suite.

	Each source is converted to several formats to find out the size of the
	output. Output formats are: WDF, PLAIN ISO, CISO, WBFS, WIA. WIA is checked
	with different compeession modes controlled by --c1 --c2 and --c3.
	Some output images may additonally compressed with bzip2, rar and 7z.

	Usage:  $myname [option]... iso_file...

	Options:
	  --wia        : Enter fast mode => do only WDF and WIA test
	  
	  --bzip2      : enable bzip2 tests (default if tool 'bzip2' found)
	  --no-bzip2   : disable bzip2 tests
	  --rar        : enable rar tests (default if tool 'rar' found)
	  --no-rar     : disable rar tests
	  --7zip       : enable 7zip tests (default if tool '7z' found)
	  --no-7zip    : disable 7zip tests
	  --all        : shortcut for --bzip2 --rar --7z

	  --data       : DATA partition only, scrub all others

	  --c1 list    : This three options define three lists of compression modes.
	  --c2 list      modes. Each element of one list is combined with each element
	  --c3 list      each element of the other lists ( c1 x c2 x c3 ). All
	                 resulting modes are normalized, sorted and repeated modes are
	                 removed. The default for --c1 are all compression methods.
	                 Options --c2 and --c3 are initial empty.

	  --decrypt    : Enable WDF/DECRYPT tests.
	  --diff       : Enable "wit DIFF" to verify WIA archives.
	  --read       : Enable read tests (conver WIA to to WDF file).
	  --dump       : 'wdf +dump a.wia >$dumpdir'

	  --db         : Enable DB mode:
	                 1.) Scan DB to find if stat is already known.
	                 2.) Append new WIA statistics to DB file '$db'.

	---EOT---
    exit 1
fi

#
#------------------------------------------------------------------------------
# check existence of needed tools

errtool=
for tool in $WIT
do
    which $tool >/dev/null 2>&1 || errtool="$errtool $tool"
done

if [[ $errtool != "" ]]
then
    echo "$myname: missing tools in PATH:$errtool" >&2
    exit 2
fi

let HAVE_BZIP2=!$(which bzip2 >/dev/null 2>&1; echo $?)
let HAVE_RAR=!$(which rar >/dev/null 2>&1; echo $?)
let HAVE_7Z=!$(which 7z >/dev/null 2>&1; echo $?)

OPT_WIA=0
OPT_BZIP2=0
OPT_RAR=0
OPT_7ZIP=0
OPT_DECRYPT=0
OPT_DATA=0
OPT_DIFF=0
OPT_READ=0
OPT_DUMP=0
OPT_DB=0

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
    last_time=$tim
    let ms=tim%1000
    let tim=tim/1000
    let s=tim%60
    let tim=tim/60
    let m=tim%60
    let h=tim/60
    if ((h))
    then
	last_ftime=$(printf "%u:%02u:%02u.%03u hms" $h $m $s $ms)
    elif ((m))
    then
	last_ftime=$(printf "  %2u:%02u.%03u m:s" $m $s $ms)
    else
	last_ftime=$(printf "     %2u.%03u sec" $s $ms)
    fi
    printf "%s  %s\n" "$last_ftime" "$3"
}

#-----------------------------------

function print_stat()
{
    printf "%-26s : " "$(printf "$@")"
}

#
#------------------------------------------------------------------------------
# calc real methods

function calc_compr_modes()
{
    #echo "--c1, N=${#C1_LIST[@]}: ${C1_LIST[*]}" >&2
    #echo "--c2, N=${#C2_LIST[@]}: ${C2_LIST[*]}" >&2
    #echo "--c3, N=${#C3_LIST[@]}: ${C3_LIST[*]}" >&2
   
    CMODES=$(
	for c1 in "${C1_LIST[@]}"
	do	
	    for c2 in "${C2_LIST[@]}"
	    do	
		for c3 in "${C3_LIST[@]}"
		do	
		    local mode="$($WIT compr --numeric --verbose "$c1$c2$c3")"
		    if [[ $mode == - ]]
		    then
			echo "Illegal compression mode: $c1$c2$c3" >&2
			exit 1
		    fi
		    echo "$mode"
		done
	    done
	done | sort -n | uniq
    )
    CMODES=$(echo $($WIT compr $CMODES))
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

dumpdir=./dumpdir.tmp

export WIT_OPT=
export WWT_OPT=

#
#------------------------------------------------------------------------------
# check_db

function check_db()
{
    # $1 = id6
    # $2 = cmode

    if ((OPT_DB))
    then
	local id6 cmode logmsg
	id6="$1"
	cmode="$2"

	DB_ID="$id6-$cmode"
	((OPT_DATA)) && DB_ID="$DB_ID-d"
	logmsg="$( grep "^$DB_ID:" $db | cut -c$((${#DB_ID}+2))- )"
	if [[ $logmsg != "" ]]
	then
	    print_stat " > %s" "$cmode"
	    printf "%s [DB]\n" "$logmsg"
	    printf "%s\n" "$logmsg" >>"$tempdir/time.write.log"
	    ((basesize)) || let basesize=$( echo "$logmsg" | awk '{print $1}' )
	    return 0
	fi
    fi
    return 1
}

#
#------------------------------------------------------------------------------
# test function

function test_function()
{
    # $1 = class for log file
    # $2 = base time
    # $3 = dest file
    # $4 = info
    # $5... = command

    local class basetime dest start stop size perc

    class="$1"
    basetime="$2"
    dest="$3"
    info="$4"
    shift 4
    print_stat " > %s" "$info"
    #-----
    sync
    sleep 1
    start=$(get_msec)
    #echo; echo "$@"
    "$@" || return 1
    sync
    let stop=$(get_msec)+basetime
    #-----
    size="$(stat -c%s "$tempdir/$dest")"
    ((basesize)) || basesize=$size
    let perc=size*10000/basesize
    if ((perc>99999))
    then
	perc="$(printf "%6u%%" $((perc/100)))"
    else
	perc="$(printf "%3u.%02u%%" $((perc/100)) $((perc%100)))"
    fi
    size="$(printf "%10d %s" $size "$perc" )"
    #print_timer $start $stop "$size"
    print_timer $start $stop >/dev/null
    logmsg=$(printf "  %s %s  %s" "$size" "$last_ftime" "$info")
    printf "%s\n" "$logmsg"
    printf "%s\n" "$logmsg" >>"$tempdir/time.$class.log"
    [[ $DB_ID = "" ]] || printf "%s:%s\n" "$DB_ID" "$logmsg" >>"$db"
    return 0
}

#
#------------------------------------------------------------------------------
# test suite

function test_suite()
{
    # $1 = valid source

    rm -rf "$tempdir"/*
    mkdir -p "$tempdir"

    local ft id6 name

    ft="$($WIT FILETYPE -H -l "$1" | awk '{print $1}')" || return 1
    id6="$($WIT FILETYPE -H -l "$1" | awk '{print $2}')" || return 1

    printf "\n** %s %s %s\n" "$ft" "$id6" "$1"
    [[ $id6 = "-" ]] && return $STAT_NOISO

    name="$($WIT ID6 --long --source "$1")"
    name="$( echo "$name" | sed 's/  /, /')"
    
    DB_ID=


    #----- START, read source once

    if ((!OPT_DB))
    then
	basesize=0
	test_function write 0 b.wdf "WDF/start" \
	    $WIT_CP "$1" --wdf "$tempdir/b.wdf" || return 1
	rm -f "$tempdir"/time.*.log
    fi


    #----- wdf

    basesize=0

    if ! check_db "$id6" "WDF"
    then
	test_function write 0 a.wdf "WDF" \
	    $WIT_CP "$1" --wdf "$tempdir/a.wdf" || return 1
	local wdf_time=$last_time

	if ((OPT_BZIP2))
	then
	    test_function write $wdf_time a.wdf.bz2 "WDF + BZIP2" \
		bzip2 --keep "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_RAR))
	then
	    test_function write $wdf_time a.wdf.rar "WDF + RAR" \
		rar a -inul "$tempdir/a.wdf.rar" "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_7ZIP))
	then
	    test_function write $wdf_time a.wdf.7z "WDF + 7Z" \
		7z a -bd "$tempdir/a.wdf.7z" "$tempdir/a.wdf" || return 1
	fi

	rm -f "$tempdir/a.wdf"*
    fi


    #----- wdf/decrypt
 
    if ((OPT_DECRYPT)) && ! check_db "$id6" "WDF/DECRYPT"
    then   
	test_function write 0 a.wdf "WDF/DECRYPT" \
	    $WIT_CP "$1" --wdf --enc decrypt "$tempdir/a.wdf" || return 1
	local wdf_time=$last_time

	if ((OPT_BZIP2))
	then
	    test_function write $wdf_time a.wdf.bz2 "WDF/DECRYPT + BZIP2" \
		bzip2 --keep "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_RAR))
	then
	    test_function write $wdf_time a.wdf.rar "WDF/DECRYPT + RAR" \
		rar a -inul "$tempdir/a.wdf.rar" "$tempdir/a.wdf" || return 1
	fi

	if ((OPT_7ZIP))
	then
	    test_function write $wdf_time a.wdf.7z "WDF/DECRYPT + 7Z" \
		7z a -bd "$tempdir/a.wdf.7z" "$tempdir/a.wdf" || return 1
	fi

	rm -f "$tempdir/a.wdf"*
    fi


    #----- wia

    for cmode in $CMODES
    do
	check_db "$id6" "WIA/$cmode" && continue

	test_function write 0 a.wia "WIA/$cmode" \
	    $WIT_CP "$1" --wia --compr $cmode "$tempdir/a.wia" || return 1
	local wdf_time=$last_time
	DB_ID=

	if ((OPT_READ))
	then
	    test_function read 0 a.wia "READ/$cmode" \
		$WIT_CP "$tempdir/a.wia" --wdf "$tempdir/read.wdf" || return 1
	    rm -f "$tempdir/read.wdf"
	fi

	((OPT_DUMP)) && $WDF +dump "$tempdir/a.wia" >"$dumpdir/$id6-$cmode.dump"

	if [[ ${cmode:0:4}  == NONE ]]
	then
	    if ((OPT_BZIP2))
	    then
		test_function write $wdf_time a.wia.bz2 "WIA/$cmode + BZIP2" \
		    bzip2 --keep "$tempdir/a.wia" || return 1
	    fi

	    if ((OPT_RAR))
	    then
		test_function write $wdf_time a.wia.rar "WIA/$cmode + RAR" \
		    rar a -inul "$tempdir/a.wia.rar" "$tempdir/a.wia" || return 1
	    fi

	    if ((OPT_7ZIP))
	    then
		test_function write $wdf_time a.wia.7z "WIA/$cmode + 7Z" \
		    7z a -bd "$tempdir/a.wia.7z" "$tempdir/a.wia" || return 1
	    fi
	fi

	if ((OPT_DIFF))
	then
	    echo " - wit DIFF orig-source a.wia"
	    wit diff $PSEL "$1" "$tempdir/a.wia" \
		|| echo "!!! $id6: wit DIFF orig-source a.wia/$cmode FAILED!" | tee -a "$log"
	fi

	rm -f "$tempdir/a.wia"*
    done


    #----- plain iso

    if ((!OPT_WIA))
    then
	test_function write 0 a.iso "PLAIN ISO" \
	    $WIT_CP "$1" --iso "$tempdir/a.iso" || return 1
	local iso_time=$last_time

	if ((OPT_BZIP2))
	then
	    test_function write iso_time a.iso.bz2 "PLAIN ISO + BZIP2" \
		bzip2 --keep "$tempdir/a.iso" || return 1
	fi

	if ((OPT_RAR))
	then
	    test_function write iso_time a.iso.rar "PLAIN ISO + RAR" \
		rar a -inul "$tempdir/a.iso.rar" "$tempdir/a.iso" || return 1
	fi

	if ((OPT_7ZIP))
	then
	    test_function write iso_time a.iso.7z "PLAIN ISO + 7Z" \
		7z a -bd "$tempdir/a.iso.7z" "$tempdir/a.iso" || return 1
	fi

	rm -f "$tempdir/a.iso"*
    fi


    #----- ciso

    if ((!OPT_WIA))
    then
	test_function write 0 a.ciso "CISO" \
	    $WIT_CP "$1" --ciso "$tempdir/a.ciso" || return 1

	rm -f "$tempdir/a.ciso"*
    fi


    #----- wbfs

    if ((!OPT_WIA))
    then
	test_function write 0 a.wbfs "WBFS" \
	    $WIT_CP "$1" --wbfs "$tempdir/a.wbfs" || return 1

	rm -f "$tempdir/a.wbfs"*
    fi


    #----- summary

    for class in write read
    do
	if [[ -s "$tempdir/time.$class.log" ]]
	then
	{
	    printf "\f\nSummary [$class,size] of %s:\n\n" "$name"
	    sort "$tempdir/time.$class.log"
	    printf "\nSummary [$class,time] of %s:\n\n" "$name"
	    sort +2 "$tempdir/time.$class.log"
	    printf "\n"

	} | tee -a "$log"
	fi
    done
     
    return 0
}

#
#------------------------------------------------------------------------------
# print header

{
    sep="-----------------------------------"
    printf "\n\f\n%s%s\n\n" "$sep" "$sep"
    date '+%F %T'
    echo
    $WIT --version
    echo
    echo "PARAM: $*"
    echo
} | tee -a $log $err

#
#------------------------------------------------------------------------------
# main loop

opts=1
calc_compr_modes

while (($#))
do
    src="$1"
    shift

    if [[ $src == --wia || $src == --fast ]] # --fast = old & obsolete
    then
	OPT_WIA=1
	((opts++)) || printf "\n"
	printf "## --wia : Do only WDF and WIA tests\n"
	continue
    fi

    if [[ $src == --bzip2 || $src == --bz2 ]]
    then
	OPT_BZIP2=$HAVE_BZIP2
	((opts++)) || printf "\n"
	printf "## --bzip2 : bzip2 tests enabled\n"
	continue
    fi

    if [[ $src == --no-bzip2 || $src == --nobzip2 || $src == --no-bz2 || $src == --nobz2 ]]
    then
	OPT_BZIP2=0
	((opts++)) || printf "\n"
	printf "## --no-bzip2 : bzip2 tests disabled\n"
	continue
    fi

    if [[ $src == --rar ]]
    then
	OPT_RAR=$HAVE_RAR
	((opts++)) || printf "\n"
	printf "## --rar : rar tests enabled\n"
	continue
    fi

    if [[ $src == --no-rar || $src == --norar ]]
    then
	OPT_RAR=0
	((opts++)) || printf "\n"
	printf "## --no-rar : rar tests disabled\n"
	continue
    fi

    if [[ $src == --7zip || $src == --7z ]]
    then
	OPT_7ZIP=$HAVE_7Z
	((opts++)) || printf "\n"
	printf "## --7zip : 7zip tests enabled\n"
	continue
    fi

    if [[ $src == --no-7zip || $src == --no7zip || $src == --no-7z || $src == --no7z ]]
    then
	OPT_7ZIP=0
	((opts++)) || printf "\n"
	printf "## --no-7zip : 7zip tests disabled\n"
	continue
    fi

    if [[ $src == --all ]]
    then
	OPT_BZIP2=$HAVE_BZIP2
	OPT_RAR=$HAVE_RAR
	OPT_7ZIP=$HAVE_7Z
	((opts++)) || printf "\n"
	printf "## --all : shortcut for --bzip2 --rar --7z\n"
	continue
    fi

    if [[ $src == --decrypt ]]
    then
	OPT_DECRYPT=1
	((opts++)) || printf "\n"
	printf "## --decrypt : WDF/DECRYPT tests enabled\n"
	continue
    fi

    if [[ $src == --diff ]]
    then
	OPT_DIFF=1
	((opts++)) || printf "\n"
	printf "## --diff : 'wit DIFF' tests enabled\n"
	continue
    fi

    if [[ $src == --read ]]
    then
	OPT_READ=1
	((opts++)) || printf "\n"
	printf "## --read : Read tests enabled\n"
	continue
    fi

    if [[ $src == --dump ]]
    then
	OPT_DUMP=1
	mkdir -p $dumpdir
	((opts++)) || printf "\n"
	printf "## --dump : 'wdf +dump a.wia >$dumpdir'\n"
	continue
    fi

    if [[ $src == --data ]]
    then
	PSEL="--psel data"
	OPT_DATA=1
	((opts++)) || printf "\n"
	printf "## --data : DATA partition only, scrub all others\n"
	continue
    fi

    if [[ $src == --c1 || $src == --compr ]]  # --compr = old & obsolete
    then
	C1_LIST=($( echo "$1" | tr ',' ' '))
	(( ${#C1_LIST[@]} )) || C1_LIST=("")
	shift
	((opts++)) || printf "\n"
	printf "## --c1 : C1 list set to: ${C1_LIST[*]}\n"
	calc_compr_modes
	printf "## compression modes are: $CMODES\n"
	continue
    fi

    if [[ $src == --c2 || $src == --level ]]  # --level = old & obsolete
    then
	C2_LIST=($( echo "$1" | tr ',' ' '))
	(( ${#C2_LIST[@]} )) || C2_LIST=("")
	shift
	((opts++)) || printf "\n"
	printf "## --c2 : C2 list set to: ${C2_LIST[*]}\n"
	calc_compr_modes
	printf "## compression modes are: $CMODES\n"
	continue
    fi

    if [[ $src == --c3 ]]
    then
	C3_LIST=($( echo "$1" | tr ',' ' '))
	(( ${#C3_LIST[@]} )) || C3_LIST=("")
	shift
	((opts++)) || printf "\n"
	printf "## --c3 : C3 list set to: ${C3_LIST[*]}\n"
	calc_compr_modes
	printf "## compression modes are: $CMODES\n"
	continue
    fi

    if [[ $src == --db ]]
    then
	OPT_DB=1
	printf "# %s - %s\n" "$(date '+%F %T')" "$($WIT --version)" >>$db
	((opts++)) || printf "\n"
	printf "## --db : DB mode enabled.\n"
	continue
    fi

    #---- hidden options

    if [[ $src == --io ]]
    then
	IO="--io $( echo "$1" | tr -cd '0-9' )"
	shift
	((opts++)) || printf "\n"
	printf "## Define option : $IO\n"
	continue
    fi


    #---- execute

    opts=0
    WIT_CP="$WIT COPY $PSEL $IO -q"

    total_start=$(get_msec)
    test_suite "$src"
    stat=$?
    total_stop=$(get_msec)

    print_stat " * TOTAL TIME:"
    print_timer $total_start $total_stop

done 2>&1 | tee -a $err

#
#------------------------------------------------------------------------------
# term

rm -rf "$tempdir/"
exit 0

