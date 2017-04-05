#!/bin/sh

PROG="`basename $0`"
HOSTNAME="`hostname`"
MAILBIN="mail"

if [ -z "`which $MAILBIN`" ] ; then
  echo "$MAILBIN not in \$PATH. Consider installing it."
  exit 1
fi

Usage() {
cat << EOF

The following are mandatory:
  -4 HOSTADDRESS (\$address$)
  -6 HOSTADDRESS (\$address6$)
  -d LONGDATETIME (\$icinga.long_date_time$)
  -e SERVICENAME (\$service.name$)
  -l HOSTALIAS (\$host.name$)
  -n HOSTDISPLAYNAME (\$host.display_name$)
  -o SERVICEOUTPUT (\$service.output$)
  -r USEREMAIL (\$user.email$)
  -s SERVICESTATE (\$service.state$)
  -t NOTIFICATIONTYPE (\$notification.type$)
  -u SERVICEDISPLAYNAME (\$service.display_name$)

And these are optional:
  -b NOTIFICATIONAUTHORNAME (\$notification.author$)
  -c NOTIFICATIONCOMMENT (\$notification.comment$)
  -i ICINGAWEB2URL (\$notification_icingaweb2url$, Default: unset)
  -f MAILFROM (\$notification_mailfrom$, requires GNU mailutils)
e -v (\$notification_sendtosyslog$, Default: false)

EOF
exit 1;
}

while getopts 4:6:b:c:d:e:f:hi:l:n:o:r:s:t:u:v: opt
do
  case "$opt" in
    4) HOSTADDRESS=$OPTARG ;;
    6) HOSTADDRESS6=$OPTARG ;;
    b) NOTIFICATIONAUTHORNAME=$OPTARG ;;
    c) NOTIFICATIONCOMMENT=$OPTARG ;;
    d) LONGDATETIME=$OPTARG ;;
    e) SERVICENAME=$OPTARG ;;
    f) MAILFROM=$OPTARG ;;
    h) Usage ;;
    i) ICINGAWEB2URL=$OPTARG ;;
    l) HOSTALIAS=$OPTARG ;;
    n) HOSTDISPLAYNAME=$OPTARG ;;
    o) SERVICEOUTPUT=$OPTARG ;;
    r) USEREMAIL=$OPTARG ;;
    s) SERVICESTATE=$OPTARG ;;
    t) NOTIFICATIONTYPE=$OPTARG ;;
    u) SERVICEDISPLAYNAME=$OPTARG ;;
    v) VERBOSE=$OPTARG ;;
   \?) echo "ERROR: Invalid option -$OPTARG" >&2
       Usage ;;
    :) echo "Missing option argument for -$OPTARG" >&2
       Usage ;;
    *) echo "Unimplemented option: -$OPTARG" >&2
       Usage ;;
  esac
done

shift $((OPTIND - 1))

## Build the message's subject
SUBJECT="[$NOTIFICATIONTYPE] $SERVICEDISPLAYNAME on $HOSTDISPLAYNAME is $SERVICESTATE!"

## Build the notification message
NOTIFICATION_MESSAGE=`cat << EOF
***** Icinga 2 Service Monitoring on $HOSTNAME *****

==> $SERVICEDISPLAYNAME on $HOSTDISPLAYNAME is $SERVICESTATE! <==

Info?    $SERVICEOUTPUT

When?    $LONGDATETIME
Service? $SERVICENAME (aka "$SERVICEDISPLAYNAME")
Host?    $HOSTALIAS (aka "$HOSTDISPLAYNAME")
IPv4?	 $HOSTADDRESS
EOF
`

## Is this host IPv6 capable? Put its address into the message.
if [ -n "$HOSTADDRESS6" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE
IPv6?    $HOSTADDRESS6"
fi

## Are there any comments? Put them into the message.
if [ -n "$NOTIFICATIONCOMMENT" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE

Comment by $NOTIFICATIONAUTHORNAME:
  $NOTIFICATIONCOMMENT"
fi

## Are we using Icinga Web 2? Put the URL into the message.
if [ -n "$ICINGAWEB2URL" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE

Get live status:
  $ICINGAWEB2URL/monitoring/service/show?host=$HOSTALIAS&service=$SERVICENAME"
fi

## Are we verbose? Then put a message to syslog.
if [ "$VERBOSE" == "true" ] ; then
  logger "$PROG sends $SUBJECT => $USEREMAIL"
fi

## And finally: send the message using $MAILBIN command.
## Do we have an explicit sender? Then we'll use it.
if [ -n "$MAILFROM" ] ; then
  /usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" \
  | $MAILBIN -a "From: $MAILFROM" -s "$SUBJECT" $USEREMAIL
else
  /usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" \
  | $MAILBIN -s "$SUBJECT" $USEREMAIL
fi
