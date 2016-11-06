#!/bin/sh
ZONE="$1"
TIME="$2"
COMMAND="TZ=${ZONE} date --date='@${TIME}'"
echo $(eval $COMMAND)
