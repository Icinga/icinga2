/* Command objects */

object NotificationCommand "mail-host-notification" {
  import "plugin-notification-command"

  command = [ SysconfDir + "/icinga2/scripts/mail-host-notification.sh" ]

  env = {
    NOTIFICATIONTYPE = "$notification.type$"
    HOSTALIAS = "$host.display_name$"
    HOSTADDRESS = "$address$"
    HOSTSTATE = "$host.state$"
    LONGDATETIME = "$icinga.long_date_time$"
    HOSTOUTPUT = "$host.output$"
    NOTIFICATIONAUTHORNAME = "$notification.author$"
    NOTIFICATIONCOMMENT = "$notification.comment$"
    HOSTDISPLAYNAME = "$host.display_name$"
    USEREMAIL = "$user.email$"
  }
}

object NotificationCommand "mail-service-notification" {
  import "plugin-notification-command"

  command = [ SysconfDir + "/icinga2/scripts/mail-service-notification.sh" ]

  env = {
    NOTIFICATIONTYPE = "$notification.type$"
    SERVICEDESC = "$service.name$"
    HOSTALIAS = "$host.display_name$"
    HOSTADDRESS = "$address$"
    SERVICESTATE = "$service.state$"
    LONGDATETIME = "$icinga.long_date_time$"
    SERVICEOUTPUT = "$service.output$"
    NOTIFICATIONAUTHORNAME = "$notification.author$"
    NOTIFICATIONCOMMENT = "$notification.comment$"
    HOSTDISPLAYNAME = "$host.display_name$"
    SERVICEDISPLAYNAME = "$service.display_name$"
    USEREMAIL = "$user.email$"
  }
}

