#!/bin/bash

CONFIGFILE=`/root/wifibroadcast_misc/gpio-config.py`

# check if config file exists
if [ -e "/boot/$CONFIGFILE" ]; then
    # convert dos format (CR/LF) config file on fat partition to unix format
    dos2unix -n /boot/$CONFIGFILE /tmp/settings.sh  > /dev/null 2>&1
else
    echo "ERROR: Could not find $CONFIGFILE config file!" 1>&2
fi
