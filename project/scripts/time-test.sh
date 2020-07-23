#!/usr/bin/env bash

myname="${0##*/}"
output=time-test.log

if [[ $# == 0 ]]
then
    cat <<- ---EOT---

	$myname : Test adding and extracting times of tools 'wbfs' and 'wwt'

	This script expect as parameters names of ISO files. These files are subject
	of timing tests with a temporary WBFS file.

	The WDF test does the following:
	  1.) Adding and extracting the ISO with WBFS.
	  2.) Adding and extracting the ISO with WWT.
	  3.) Adding and extracting a WDF with WWT.
	  4.) Extracting an ISO and a WDF with WWT --fast.

	The results (time in milliseconds) are written to stdout and *appended* to
	the end of the file '$output'.

	The temporary files are written to a temporary subdirectory of the current
	working directory (hoping that here is enough space). The directory will be
	is deleted at the end. The signals INT, TERM and HUP are handled.

	---EOT---

    echo "usage: $myname iso_file" >&2
    echo
    exit 1
fi

#------------------------------------------------------------------------------

err=
for tool in wbfs wwt
do
    which $tool >/dev/null 2>&1 || err="$err $tool"
done

if [[ $err != "" ]]
then
    echo "$myname: missing tools in PATH:$err" >&2
    exit 2
fi

#------------------------------------------------------------------------------

let BASE_SEC=$(date +%s)

function get-msec()
{
    echo $(($(date "+(%s-BASE_SEC)*1000+10#%N/1000000")))
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

    exit 2
}

#------------------------------------------------------------------------------
# setup

tempdir=
trap f_abort INT TERM HUP
tempdir="$(mktemp -d ./.${myname}.tmp.XXXXXX)" || exit 1

wbfs_file=a.wbfs
wbfs="$tempdir/$wbfs_file"

#------------------------------------------------------------------------------
# tests

cat <<- ---EOT--- | tee -a $output

	 		  WWT	  WWT	  WWT	  WWT	  WWT	  WWT
	  WBFS	  WBFS	  add	  ex	  ex -F	  add	  ex	  ex -F
	  add	  ex	  ISO	  ISO	  ISO	  WDF	  WDF	  WDF	file
	----------------------------------------------------------------------------------------
	---EOT---

while (($#))
do
    file="$1"
    id6="$(dd if="$file" bs=1 count=6)"
    shift

    echo -e "\n---------- WBFS add ----------\n"

    wwt init "$wbfs" -f -s10
    sleep 5
    start=$(get-msec)
	wbfs -p "$wbfs" add $file
    let t_wb_ad=$(get-msec)-start

    echo -e "\n---------- WBFS extract ----------\n"

    sleep 5
    start=$(get-msec)
	( cd "$tempdir"; wbfs -p "$wbfs_file" extract $id6)
    let t_wb_ex=$(get-msec)-start
    rm -f "$tempdir"/*.iso

    echo -e "\n---------- WWT add iso ----------\n"

    wwt init "$wbfs" -f -s10
    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" add "$file"
    let t_wwt_ad_iso=$(get-msec)-start

    echo -e "\n---------- WWT ectract iso ----------\n"

    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" ex -d "$tempdir" $id6 -I
    let t_wwt_ex_iso=$(get-msec)-start
    rm -f "$tempdir"/*.iso

    echo -e "\n---------- WWT ectract iso --fast ----------\n"

    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" ex -d "$tempdir" $id6 -I --fast
    let t_wwt_ex_iso_fast=$(get-msec)-start
    rm -f "$tempdir"/*.iso

    echo -e "\n---------- WWT ectract wdf --fast ----------\n"

    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" ex -d "$tempdir" $id6 -W --fast
    let t_wwt_ex_wdf_fast=$(get-msec)-start
    rm -f "$tempdir"/*.wdf

    echo -e "\n---------- WWT ectract wdf ----------\n"

    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" ex -d "$tempdir" $id6=a.wdf -W
    let t_wwt_ex_wdf=$(get-msec)-start

    echo -e "\n---------- WWT add wdf ----------\n"

    wwt init "$wbfs" -f -s10
    sleep 5
    start=$(get-msec)
	wwt -p "$wbfs" add "$tempdir/a.wdf"
    let t_wwt_ad_wdf=$(get-msec)-start
    rm -f "$tempdir"/*.wdf

    echo -e "\n---------- output ----------\n"

    printf "%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%6d\t%s\n" \
		$t_wb_ad $t_wb_ex \
		$t_wwt_ad_iso $t_wwt_ex_iso $t_wwt_ex_iso_fast \
		$t_wwt_ad_wdf $t_wwt_ex_wdf $t_wwt_ex_wdf_fast \
		"$file" | tee -a $output

done

#------------------------------------------------------------------------------

rm -rf "$tempdir"
exit 0

