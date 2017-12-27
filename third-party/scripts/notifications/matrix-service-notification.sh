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
## Function helpers
Usage() {
cat << EOF

Required parameters:
  -d LONGDATETIME (\$icinga.long_date_time\$)
  -e SERVICENAME (\$service.name\$)
  -l HOSTNAME (\$host.name\$)
  -n HOSTDISPLAYNAME (\$host.display_name\$)
  -o SERVICEOUTPUT (\$service.output\$)
  -s SERVICESTATE (\$service.state\$)
  -t NOTIFICATIONTYPE (\$notification.type\$)
  -u SERVICEDISPLAYNAME (\$service.display_name\$)
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
while getopts 4:6:b:c:d:e:hi:l:n:o:s:t:u:m:x:y: opt
do
  case "$opt" in
    4) HOSTADDRESS=$OPTARG ;;
    6) HOSTADDRESS6=$OPTARG ;;
    b) NOTIFICATIONAUTHORNAME=$OPTARG ;;
    c) NOTIFICATIONCOMMENT=$OPTARG ;;
    d) LONGDATETIME=$OPTARG ;; # required
    e) SERVICENAME=$OPTARG ;; # required
    h) Usage ;;
    i) ICINGAWEB2URL=$OPTARG ;;
    l) HOSTNAME=$OPTARG ;; # required
    n) HOSTDISPLAYNAME=$OPTARG ;; # required
    o) SERVICEOUTPUT=$OPTARG ;; # required
    s) SERVICESTATE=$OPTARG ;; # required
    t) NOTIFICATIONTYPE=$OPTARG ;; # required
    u) SERVICEDISPLAYNAME=$OPTARG ;; # required
    m) MATRIXROOM=$OPTARG ;; # required
    x) MATRIXSERVER=$OPTARG ;; # required
    y) MATRIXTOKEN=$OPTARG ;; # required
   \?) echo "ERROR: Invalid option -$OPTARG" >&2
       Usage ;;
    :) echo "Missing option argument for -$OPTARG" >&2
       Usage ;;
    *) echo "Unimplemented option: -$OPTARG" >&2
       Usage ;;
  esac
done

shift $((OPTIND - 1))

## Check required parameters (TODO: better error message)
if [ ! "$LONGDATETIME" ] \
|| [ ! "$HOSTNAME" ] || [ ! "$HOSTDISPLAYNAME" ] \
|| [ ! "$SERVICENAME" ] || [ ! "$SERVICEDISPLAYNAME" ] \
|| [ ! "$SERVICEOUTPUT" ] || [ ! "$SERVICESTATE" ] \
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
if [ "$SERVICESTATE" = "UNKNOWN" ]
then
  ICON=$question_ico
elif [ "$SERVICESTATE" = "OK" ]
then
  ICON=$ok_ico
elif [ "$SERVICESTATE" = "WARNING" ]
then
  ICON=$warn_ico
elif [ "$SERVICESTATE" = "CRITICAL" ]
then
  ICON=$error_ico
fi

NOTIFICATION_MESSAGE=$(cat <<- EOF
$ICON <strong>Service:</strong> $SERVICEDISPLAYNAME on $HOSTDISPLAYNAME
is <strong>$SERVICESTATE.</strong>  <br/>
<strong>When:</strong> $LONGDATETIME. <br/>
<strong>Info:</strong> $SERVICENAME <br/>
EOF
)

## Check whether IPv4 was specified.
if [ -n "$HOSTADDRESS" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE <strong>IPv4:</strong> $HOSTADDRESS <br/>"
fi

## Check whether IPv6 was specified.
if [ -n "$HOSTADDRESS6" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE <strong>IPv6:</strong> $HOSTADDRESS6 <br/>"
fi

## Check whether author and comment was specified.
if [ -n "$NOTIFICATIONCOMMENT" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE <strong>Comment by $NOTIFICATIONAUTHORNAME:</strong> $NOTIFICATIONCOMMENT <br/>"
fi

## Check whether Icinga Web 2 URL was specified.
if [ -n "$ICINGAWEB2URL" ] ; then
  # Replace space with HTML
  SERVICENAME=${SERVICENAME// /%20}
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE $ICINGAWEB2URL/monitoring/service/show?host=$HOSTNAME&service=$SERVICENAME <br/>"
fi

while read line; do
  message="${message}\t${line}"
done <<< $NOTIFICATION_MESSAGE

BODY="${message}"

/usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" | $CURLBIN -k -X PUT --header 'Content-Type: application/json' --header 'Accept: application/json' -d "{
\"msgtype\": \"m.text\",
\"body\": \"$BODY\",
\"formatted_body\": \"$BODY\",
\"format\": \"org.matrix.custom.html\"
      }" "$MATRIXSERVER/_matrix/client/unstable/rooms/$MATRIXROOM/send/m.room.message/$MX_TXN?access_token=$MATRIXTOKEN"
