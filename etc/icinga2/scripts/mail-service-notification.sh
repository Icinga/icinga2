#!/usr/bin/env bash
#
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)

PROG="`basename $0`"
ICINGA2HOST="`hostname`"
MAILBIN="mail"

if [ -z "`which $MAILBIN`" ] ; then
  echo "$MAILBIN not found in \$PATH. Consider installing it."
  exit 1
fi

## Function helpers
Usage() {
cat << EOF

Required parameters:
  -d LONGDATETIME (\$icinga.long_date_time\$)
  -e SERVICENAME (\$service.name\$)
  -l HOSTNAME (\$host.name\$)
  -n HOSTDISPLAYNAME (\$host.display_name\$)
  -o SERVICEOUTPUT (\$service.output\$)
  -r USEREMAIL (\$user.email\$)
  -s SERVICESTATE (\$service.state\$)
  -t NOTIFICATIONTYPE (\$notification.type\$)
  -u SERVICEDISPLAYNAME (\$service.display_name\$)

Optional parameters:
  -4 HOSTADDRESS (\$address\$)
  -6 HOSTADDRESS6 (\$address6\$)
  -b NOTIFICATIONAUTHORNAME (\$notification.author\$)
  -c NOTIFICATIONCOMMENT (\$notification.comment\$)
  -i ICINGAWEB2URL (\$notification_icingaweb2url\$, Default: unset)
  -f MAILFROM (\$notification_mailfrom\$, requires GNU mailutils (Debian/Ubuntu) or mailx (RHEL/SUSE))
  -v (\$notification_sendtosyslog\$, Default: false)

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
while getopts 4:6:b:c:d:e:f:hi:l:n:o:r:s:t:u:v: opt
do
  case "$opt" in
    4) HOSTADDRESS=$OPTARG ;;
    6) HOSTADDRESS6=$OPTARG ;;
    b) NOTIFICATIONAUTHORNAME=$OPTARG ;;
    c) NOTIFICATIONCOMMENT=$OPTARG ;;
    d) LONGDATETIME=$OPTARG ;; # required
    e) SERVICENAME=$OPTARG ;; # required
    f) MAILFROM=$OPTARG ;;
    h) Usage ;;
    i) ICINGAWEB2URL=$OPTARG ;;
    l) HOSTNAME=$OPTARG ;; # required
    n) HOSTDISPLAYNAME=$OPTARG ;; # required
    o) SERVICEOUTPUT=$OPTARG ;; # required
    r) USEREMAIL=$OPTARG ;; # required
    s) SERVICESTATE=$OPTARG ;; # required
    t) NOTIFICATIONTYPE=$OPTARG ;; # required
    u) SERVICEDISPLAYNAME=$OPTARG ;; # required
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

## Check required parameters (TODO: better error message)
## Keep formatting in sync with mail-host-notification.sh
if [ ! "$LONGDATETIME" ] \
|| [ ! "$HOSTNAME" ] || [ ! "$HOSTDISPLAYNAME" ] \
|| [ ! "$SERVICENAME" ] || [ ! "$SERVICEDISPLAYNAME" ] \
|| [ ! "$SERVICEOUTPUT" ] || [ ! "$SERVICESTATE" ] \
|| [ ! "$USEREMAIL" ] || [ ! "$NOTIFICATIONTYPE" ]; then
  Error "Requirement parameters are missing."
fi

## Build the message's subject
SUBJECT="[$NOTIFICATIONTYPE] $SERVICEDISPLAYNAME on $HOSTDISPLAYNAME is $SERVICESTATE!"

## Build the notification message
NOTIFICATION_MESSAGE=`cat << EOF
***** Service Monitoring on $ICINGA2HOST *****

$SERVICEDISPLAYNAME on $HOSTDISPLAYNAME is $SERVICESTATE!

Info:    $SERVICEOUTPUT

When:    $LONGDATETIME
Service: $SERVICENAME
Host:    $HOSTNAME
EOF
`

## Check whether IPv4 was specified.
if [ -n "$HOSTADDRESS" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE
IPv4:    $HOSTADDRESS"
fi

## Check whether IPv6 was specified.
if [ -n "$HOSTADDRESS6" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE
IPv6:    $HOSTADDRESS6"
fi

## Check whether author and comment was specified.
if [ -n "$NOTIFICATIONCOMMENT" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE

Comment by $NOTIFICATIONAUTHORNAME:
  $NOTIFICATIONCOMMENT"
fi

## Check whether Icinga Web 2 URL was specified.
if [ -n "$ICINGAWEB2URL" ] ; then
  NOTIFICATION_MESSAGE="$NOTIFICATION_MESSAGE

$ICINGAWEB2URL/monitoring/service/show?host=$HOSTNAME&service=$SERVICENAME"
fi

## Check whether verbose mode was enabled and log to syslog.
if [ "$VERBOSE" == "true" ] ; then
  logger "$PROG sends $SUBJECT => $USEREMAIL"
fi

## Send the mail using the $MAILBIN command.
## If an explicit sender was specified, try to set it.
if [ -n "$MAILFROM" ] ; then

  ## Modify this for your own needs!

  ## Debian/Ubuntu use mailutils which requires `-a` to append the header
  if [ -f /etc/debian_version ]; then
    /usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" | $MAILBIN -a "From: $MAILFROM" -s "$SUBJECT" $USEREMAIL
  ## Other distributions (RHEL/SUSE/etc.) prefer mailx which sets a sender address with `-r`
  else
    /usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" | $MAILBIN -r "$MAILFROM" -s "$SUBJECT" $USEREMAIL
  fi

else
  /usr/bin/printf "%b" "$NOTIFICATION_MESSAGE" \
  | $MAILBIN -s "$SUBJECT" $USEREMAIL
fi
