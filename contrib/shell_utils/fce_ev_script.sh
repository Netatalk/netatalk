#!/bin/sh

usage="$(basename $0) [-h] [-v version] [-e event] [-P path] [-S source path] -- FCE sample script

where:
    -h  show this help text
    -v  version
    -e  event
    -P  path
    -S  source path for events like rename/move
    -u  username
    -p  pid
    -i  event ID
"

while getopts ':hs:v:e:P:S:u:p:i:' option; do
  case "$option" in
    h) echo "$usage"
       exit
       ;;
    v) version=$OPTARG
       ;;
    e) event=$OPTARG
       ;;
    P) path=$OPTARG
       ;;
    S) srcpath=$OPTARG
       ;;
    u) user=$OPTARG
       ;;
    p) pid=$OPTARG
       ;;
    i) evid=$OPTARG
       ;;
    ?) printf "illegal option: '%s'\n" "$OPTARG" >&2
       echo "$usage" >&2
       exit 1
       ;;
  esac
done
shift $((OPTIND - 1))

printf "FCE Event: $event" >> /tmp/fce.log 
if [ -n "$version" ] ; then
    printf ", protocol: $version" >> /tmp/fce.log
fi
if [ -n "$evid" ] ; then
    printf ", ID: $evid" >> /tmp/fce.log
fi
if [ -n "$pid" ] ; then
    printf ", pid: $pid" >> /tmp/fce.log
fi
if [ -n "$user" ] ; then
    echo -n ", user: $user" >> /tmp/fce.log
fi
if [ -n "$srcpath" ] ; then
    echo -n ", source: $srcpath" >> /tmp/fce.log
fi
if [ -n "$path" ] ; then
    echo -n ", path: $path" >> /tmp/fce.log
fi
printf "\n" >> /tmp/fce.log
