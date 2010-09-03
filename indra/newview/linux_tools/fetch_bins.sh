#!/bin/bash

# Necessary files
if [[ "$1" == "--emkdu" ]]; then
BINS="bin/SLVoice bin/libemkdu.so lib/libortp.so lib/libvivoxsdk.so lib/libfmod-3.75.so"
else
BINS="bin/SLVoice lib/libortp.so lib/libvivoxsdk.so lib/libfmod-3.75.so"
fi

# Locations of client to use
#URL="http://download.cloud.secondlife.com/SecondLife-i686-1.23.5.136262.tar.bz2"

#default to not downloading a package with emkdu for now, since LL has asked us not to use it.
if [[ "$1" == "--emkdu" ]]; then
	URL="http://www.phoenixviewer.com/fmod-vivox-kdu.tar.bz2"
else
	URL="http://www.phoenixviewer.com/fmod-vivox.tar.bz2"
fi

ARCHIVE="${URL##*/}"
#FOLDER="${ARCHIVE%.*.*}"

missing_bins() {
	for file in $BINS; do
		if [[ ! -f "$file" ]]; then
			echo "Missing binary: ./$file."
			return 0
		fi
	done
	
	return 1
}

if [[ "$1" == "--force" ]]; then
	GET="wget -c --random-wait -O $ARCHIVE $URL"
else
	GET="wget -nc --random-wait $URL"
fi

echo "Looking for missing binaries."
if [[ `missing_bins` || "$1" == "--force" ]]; then
	echo "Fetching binary package."
	if `$GET`; then
		echo "Extracting."
#		if tar -xjv --strip-components=1 -f $ARCHIVE $FOLDER/${BINS// / $FOLDER/}; then
		if tar -xjvf $ARCHIVE; then
			echo "Binaries successfully obtained."
		fi
	fi
else
	echo "All binaries found."
fi
