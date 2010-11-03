#!/bin/bash

# Location of the vivox binary
URL="http://s3.amazonaws.com/viewer-source-downloads/install_pkgs/vivox-3.1.0001.8821-linux-20100529.tar.bz2"
ARCHIVE="${URL##*/}"

if [[ "$1" == "--force" ]]; then
	GET="wget -c --random-wait -O $ARCHIVE $URL"
else
	GET="wget -nc --random-wait $URL"
fi

echo "Looking for SLVoice..."
if [[ ! -f "bin/SLVoice" || "$1" == "--force" ]]; then
	echo "Fetching SLVoice package."
	if `$GET`; then
		echo "Extracting."
		tar -xvjf "$ARCHIVE" -C bin --strip-components 4 indra/newview/vivox-runtime/i686-linux/SLVoice
		tar -xvjf "$ARCHIVE" -C lib --strip-components 4 --wildcards indra/newview/vivox-runtime/i686-linux/lib*
	fi
else
	echo "SLVoice found."
fi
