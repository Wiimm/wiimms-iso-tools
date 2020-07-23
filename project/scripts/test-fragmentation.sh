#!/usr/bin/env bash

dev="$1"
[[ $dev = "" ]] && dev=/dev/sdb2

rev_need=r666
log="${0##*/}.log"

max=16
fragid=PHT000
firstid=PHT001
midid="$(printf "PHT%03u" $((max|1)))"
lastid="$(printf "PHT%03u" $((2*max-1)))"
testlist="$fragid $firstid $midid $lastid"

if [[ $(wwt version | awk '{print $6}') < $rev_need ]]
then
	printf '\n*** %s: Need at least wwt revision %s\n\n' "${0##*/}" "$rev_need" >&2
	exit 1
fi

if ! wwt find -q -p "$dev"
then
	printf '\n*** %s: Not a WBFS: %s\n\n' "${0##*/}" "$dev" >&2
	exit 1
fi

{
	printf '\n---------- INIT ----------\n\n'

	wwt init $dev -qf
	rmlist=""
	for ((i=0;i<$max;i++))
	do
		wwt -p $dev -q phantom 256M 4G
		rmlist="$rmlist $(printf "PHT%03u" $((2*i)))"
	done
	wwt -p $dev -q rm $rmlist
	wwt -p $dev -q phantom 4G

	#wwt ll $dev --sort=none
	wwt -p $dev  dump -lll | sed '0,/WBFS Memory Map/ d' | grep -iv inode

	printf '\n---------- EXTRACT ----------\n\n'

	for (( i=0; i<10; i++ ))
	do
	    for id in $testlist
	    do
		    echo -n "extract $id ... "
		    start=$(date +%s)
		    wwt -p $dev -q extract $id=/dev/null --overwrite
		    end=$(date +%s)
		    printf "%3u sec\n" $((end-start))
	    done
	    echo
	done

	printf '\n---------- SUM ----------\n\n'

} | tee $log


awk='
 $1=="extract" && $5=="sec" { n[$2]++; s[$2]+=$4 }

 END { for ( i in s )
	printf("%s: %5u sec / %2u == %3u sec\n",
		i, s[i], n[i], s[i]/n[i] ); }

'

awk "$awk" $log | sort -n +4 | tee $log.tmp
cat $log.tmp >>$log
rm -f $log.tmp

