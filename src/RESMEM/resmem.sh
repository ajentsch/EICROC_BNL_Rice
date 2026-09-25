#!/bin/bash

function extractNumber {
	local INPUT=$1
	if [[ $INPUT == *G ]]; then
		local GBs=${INPUT//G/}
		if ! [[ "$GBs" =~ ^[0-9]+$ ]]; then
			echo "0"
		else
			echo $((1024 * 1024 * 1024 * $GBs))
		fi
	fi
}

function extractMemMap {
	local CMDLINE=`cat /proc/cmdline`
	for PAIR in $CMDLINE; do
		if [[ $PAIR == memmap* ]]; then
			local DATA=`echo $PAIR | cut -d "=" -f 2`
		fi
	done
	echo $DATA
}


MEMMAP=$(extractMemMap) 
if [[ -n $MEMMAP ]]; then
	D1=`echo $MEMMAP | cut -d "$" -f 1`
	D2=`echo $MEMMAP | cut -d "$" -f 2`
	N1=$(extractNumber $D1)
	N2=$(extractNumber $D2)
else
	echo "No reserved memory parameters found in current kernel"
fi

if [[ -n $N1 && -n $N2 ]]; then
	printf -v N1hex '0x%X' $N1
	printf -v N2hex '0x%X' $N2
	echo "Found reserved memory for size $N1hex at offset $N2hex"
	echo "sudo insmod resmem.ko resmem_hwaddr=$N2hex resmem_length=$N1hex"
	sudo insmod resmem.ko resmem_hwaddr=$N2hex resmem_length=$N1hex
else
	echo "Error extracting reserved memory information from kernel parameters"
fi
