#!/bin/bash
# Based on original E-Mail Icinga2 notification

PROG="`basename $0`"
ICINGA2HOST="`hostname`"
CURLBIN="curl"
MX_TXN="`date "+%s"`$(( RANDOM % 9999 ))"

if [ -z "`which $CURLBIN`" ] ; then
  echo "$CURLBIN not found in \$PATH. Consider installing it."
  exit 1
fi

warn_ico="⚠"
error_ico="❌"
ok_ico="🆗"
question_ico="❓"

#Set the message icon based on service state
# For nagios, replace ICINGA_ with NAGIOS_ to get the environment variables from Nagios
## Function helpers
Usage() {
cat << EOF

Required parameters:
  -d LONGDATETIME (\$icinga.long_date_time\$)
  -l HOSTNAME (\$host.name\$)
  -n HOSTDISPLAYNAME (\$host.display_name\$)
  -o HOSTOUTPUT (\$host.output\$)
  -s HOSTSTATE (\$host.state\$)
  -t NOTIFICATIONTYPE (\$notification.type\$)
  -m MATRIXROOM (\$notification_matrix_room_id\$)
  -x MATRIXSERVER (\$notification_matrix_server\$)
  -y MATRIXTOKEN (\$notification_matrix_token\$)

Optional parameters:
  -4 HOSTADDRESS (\$address\$)
  -6 HOSTADDRESS6 (\$address6\$)
  -b NOTIFICATIONAUTHORNAME (\$notification.author\$)
  -c NOTIFICATIONCOMMENT (\$notification.comment\$)
  -i ICINGAWEB2URL (\$notification_icingaweb2url\$, Default: unset)

EOF
}

Help() {
  Usage;
  exit 0;
}

Error() {
  if [ "$1" ]; then
    echo $1
  fi
  Usage;
  exit 1;
}

## Main
while getopts 4:6::b:c:d:hi:l:n:o:s:t:m:x:y: opt
do
  case "$opt" in
    4) HOSTADDRESS=$OPTARG ;;
    6) HOSTADDRESS6=$OPTARG ;;
    b) NOTIFICATIONAUTHORNAME=$OPTARG ;;
    c) NOTIFICATIONCOMMENT=$OPTARG ;;
    d) LONGDATETIME=$OPTARG ;; # required
    h) Help ;;
    i) ICINGAWEB2URL=$OPTARG ;;
    l) HOSTNAME=$OPTARG ;; # required
    n) HOSTDISPLAYNAME=$OPTARG ;; # required
    o) HOSTOUTPUT=$OPTARG ;; # required
    s) HOSTSTATE=$OPTARG ;; # required
    t) NOTIFICATIONTYPE=$OPTARG ;; # required
    m) MATRIXROOM=$OPTARG ;; # required
    x) MATRIXSERVER=$OPTARG ;; # required
    y) MATRIXTOKEN=$OPTARG ;; # required
   \?) echo "ERROR: Invalid option -$OPTARG" >&2
       Error ;;
    :) echo "Missing option argument for -$OPTARG" >&2
       Error ;;
    *) echo "Unimplemented option: -$OPTARG" >&2
       Error ;;
  esac
done

shift $((OPTIND - 1))

## Check required parameters (TODO: better error message)
if [ ! "$LONGDATETIME" ] \
|| [ ! "$HOSTNAME" ] || [ ! "$HOSTDISPLAYNAME" ] \
|| [ ! "$HOSTOUTPUT" ] || [ ! "$HOSTSTATE" ] \
|| [ ! "$NOTIFICATIONTYPE" ]; then
  Error "Requirement parameters are missing."
fi

## Build the notification message

if [ "$HOSTSTATE" = "UP" ]
then
  ICON=$ok_ico
elif [ "$HOSTSTATE" = "DOWN" ]
then
  ICON=$error_ico
fi
if [ "$HOSTSTATE" = "UNKNOWN" ]
then
  ICON=$question_ico
elif [ "$ICINGA_SERVICESTATE" = "OK" ]
then
  ICON=$ok_ico
elif [ "$ICINGA_SERVICESTATE" = "WARNING" ]
then
  ICON=$warn_ico
elif [ "$ICINGA_SERVICESTATE" = "CRITICAL" ]
then
  ICON=$error_ico
fi

NOTIFICATION_MESSAGE=$(cat << EOF
$ICON <b>HOST:</b> $HOSTDISPLAYNAME is <b>$HOSTSTATE!</b> <br/>
<b>When:</b> $LONGDATETIME<br/>
<b>Info:</b> $HOSTOUTPUT<br/>
EOF
)

## Check whether IPv4 was specified.
if [ -n "$HOSTADDRESS" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE <b>IPv4:</b> $HOSTADDRESS <br/>"
fi

## Check whether IPv6 was specified.
if [ -n "$HOSTADDRESS6" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE <b>IPv6:</b> $HOSTADDRESS6 <br/>"
fi

## Check whether author and comment was specified.
if [ -n "$NOTIFICATIONCOMMENT" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE Comment by <b>$NOTIFICATIONAUTHORNAME:</b> $NOTIFICATIONCOMMENT <br/>"
fi

## Check whether Icinga Web 2 URL was specified.
if [ -n "$ICINGAWEB2URL" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE $ICINGAWEB2URL/monitoring/host/show?host=$HOSTNAME <br/>"
fi

while read line; do
  message="${message}\n${line}"
done <<< $NOTIFICATION_MESSAGE

BODY="${message}"

/usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" | $CURLBIN -k -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' -d "{
\"msgtype\": \"m.text\",
\"body\": \"$BODY\",
\"formatted_body\": \"$BODY\",
\"format\": \"org.matrix.custom.html\"
      }" "$MATRIXSERVER/_matrix/client/unstable/rooms/$MATRIXROOM/send/m.room.message/$MX_TXN?access_token=$MATRIXTOKEN"
