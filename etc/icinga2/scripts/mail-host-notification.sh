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
  -6 HOSTADDRESS6 (\$address6$)
  -d LONGDATETIME (\$icinga.long_date_time$)
  -l HOSTALIAS (\$host.name$)
  -n HOSTDISPLAYNAME (\$host.display_name$)
  -o HOSTOUTPUT (\$host.output$)
  -r USEREMAIL (\$user.email$)
  -s HOSTSTATE (\$host.state$)
  -t NOTIFICATIONTYPE (\$notification.type$)

And these are optional:
  -b NOTIFICATIONAUTHORNAME (\$notification.author$)
  -c NOTIFICATIONCOMMENT (\$notification.comment$)
  -i ICINGAWEB2URL (\$notification_icingaweb2url$, Default: unset)
  -f MAILFROM (\$notification_mailfrom$, requires GNU mailutils)
  -v (\$notification_sendtosyslog$, Default: false)

EOF

exit 1;
}

while getopts 4:6::b:c:d:f:hi:l:n:o:r:s:t:v: opt
do
  case "$opt" in
    4) HOSTADDRESS=$OPTARG ;;
    6) HOSTADDRESS6=$OPTARG ;;
    b) NOTIFICATIONAUTHORNAME=$OPTARG ;;
    c) NOTIFICATIONCOMMENT=$OPTARG ;;
    d) LONGDATETIME=$OPTARG ;;
    f) MAILFROM=$OPTARG ;;
    h) Usage ;;
    i) ICINGAWEB2URL=$OPTARG ;;
    l) HOSTALIAS=$OPTARG ;;
    n) HOSTDISPLAYNAME=$OPTARG ;;
    o) HOSTOUTPUT=$OPTARG ;;
    r) USEREMAIL=$OPTARG ;;
    s) HOSTSTATE=$OPTARG ;;
    t) NOTIFICATIONTYPE=$OPTARG ;;
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
SUBJECT="[$NOTIFICATIONTYPE] Host $HOSTDISPLAYNAME is $HOSTSTATE!"

## Build the notification message
NOTIFICATION_MESSAGE=`cat << EOF
***** Icinga 2 Host Monitoring on $HOSTNAME *****

==> $HOSTDISPLAYNAME ($HOSTALIAS) is $HOSTSTATE! <==

Info?    $HOSTOUTPUT

When?    $LONGDATETIME
Host?    $HOSTALIAS (aka "$HOSTDISPLAYNAME")
IPv4?	 $HOSTADDRESS
EOF
`

## Is this host IPv6 capable? Put its address into the message.
if [ -n "$HOSTADDRESS6" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE
IPv6?	 $HOSTADDRESS6"
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
  $ICINGAWEB2URL/monitoring/host/show?host=$HOSTALIAS"
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
