#!/bin/bash
## 
## Copyright 2018 Smx
##
shopt -s nocasematch
gamedir="$1"
if [ -z "$gamedir" ]; then
	echo "Usage: $0 [gamedir]"
	exit 1
fi

if [ -f "$gamedir/RUNTIME.KIX" ]; then
	datadir="$gamedir"
elif [ -f "$gamedir/DATA/RUNTIME.KIX" ]; then
	datadir="$gamedir/DATA"
else
	echo "Wrong game folder specified!"
	exit 1
fi

for kix in "$datadir"/*.KIX; do
	name=$(basename "${kix%.*}")
	kbf="${datadir}/${name}.KBF"
	if [ ! -f "$kbf" ]; then
		echo "Skipping ${name} extraction, missing kbf file"
		continue
	fi

	if [ -d "$datadir/$name" ]; then
		if [ $(ls -A "$datadir/$name" | wc -l) -gt 0 ]; then
			echo "Skipping ${name} extraction, already extracted"
			continue
		fi
	else
		mkdir "$datadir/$name"
	fi
	
	pushd "$datadir/$name" &>/dev/null
	echo "Extracting $name..."
	kunp "$kix" "$kbf"
	popd &>/dev/null
done