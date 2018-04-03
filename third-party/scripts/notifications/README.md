# Notifications
This folder contains additional 3rd party notifications scripts for Icinga2.

## Matrix
* https://matrix.org/
> An open network for secure, decentralized communication

To send notifications from Icinga2 into a room, the following conditions are needed:
* A matrix compatible server, e.g [matrix-synapse](https://github.com/matrix-org/synapse)
* An access token to get access to the matrix server
* A matrix room ID
* Access to the room (invite)

There exists several ways, but an easy way is to create a new user (for example "monitoring" via login in Riot web); get the access token and invite the new user into room, which may was create for the monitoring. A good approach is, to have several rooms, for example devops, sysops, web, dba .... and use the apply rule to assign the correct room.

### host notifications

Example notification command:

```
object NotificationCommand "Host Alarm by Matrix" {
    import "plugin-notification-command"
    command = [ "/etc/icinga2/scripts/matrix-host-notification.sh" ]
    arguments += {
        "-4" = "$notification_address$"
        "-6" = "$notification_address6$"
        "-b" = "$notification_author$"
        "-c" = "$notification_comment$"
        "-d" = {
            required = true
            value = "$notification_date$"
        }
        "-i" = "$notification_icingaweb2url$"
        "-l" = {
            required = true
            value = "$notification_hostname$"
        }
        "-m" = {
            required = true
            value = "$notification_matrix_room_id$"
        }
        "-n" = {
            required = true
            value = "$notification_hostdisplayname$"
        }
        "-o" = {
            required = true
            value = "$notification_hostoutput$"
        }
        "-s" = {
            required = true
            value = "$notification_hoststate$"
        }
        "-t" = {
            required = true
            value = "$notification_type$"
        }
        "-x" = {
            required = true
            value = "$notification_matrix_server$"
        }
        "-y" = {
            required = true
            value = "$notification_matrix_token$"
        }
    }
    vars.notification_address = "$address$"
    vars.notification_address6 = "$address6$"
    vars.notification_author = "$notification.author$"
    vars.notification_comment = "$notification.comment$"
    vars.notification_date = "$icinga.long_date_time$"
    vars.notification_hostdisplayname = "$host.display_name$"
    vars.notification_hostname = "$host.name$"
    vars.notification_hostoutput = "$host.output$"
    vars.notification_hoststate = "$host.state$"
    vars.notification_logtosyslog = false
    vars.notification_type = "$notification.type$"
    vars.notification_useremail = "$user.email$"
}
```
### service notifications

Example notification command:

```
object NotificationCommand "Service Alarm by Matrix" {
    import "plugin-notification-command"
    command = [ "/etc/icinga2/scripts/matrix-service-notification.sh" ]
    arguments += {
        "-4" = {
            required = true
            value = "$notification_address$"
        }
        "-6" = "$notification_address6$"
        "-b" = "$notification_author$"
        "-c" = "$notification_comment$"
        "-d" = {
            required = true
            value = "$notification_date$"
        }
        "-e" = {
            required = true
            value = "$notification_servicename$"
        }
        "-i" = {
            required = true
            value = "$notification_icingaweb2url$"
        }
        "-l" = {
            required = true
            value = "$notification_hostname$"
        }
        "-m" = {
            required = true
            value = "$notification_matrix_room_id$"
        }
        "-n" = {
            required = true
            value = "$notification_hostdisplayname$"
        }
        "-o" = {
            required = true
            value = "$notification_serviceoutput$"
        }
        "-s" = {
            required = true
            value = "$notification_servicestate$"
        }
        "-t" = {
            required = true
            value = "$notification_type$"
        }
        "-u" = {
            required = true
            value = "$notification_servicedisplayname$"
        }
        "-x" = {
            required = true
            value = "$notification_matrix_server$"
        }
        "-y" = {
            required = true
            value = "$notification_matrix_token$"
        }
    }
    vars.notification_address = "$address$"
    vars.notification_address6 = "$address6$"
    vars.notification_author = "$notification.author$"
    vars.notification_comment = "$notification.comment$"
    vars.notification_date = "$icinga.long_date_time$"
    vars.notification_hostdisplayname = "$host.display_name$"
    vars.notification_hostname = "$host.name$"
    vars.notification_icingaweb2url = "https://icinga2.example.com/icingaweb2"
    vars.notification_logtosyslog = false
    vars.notification_servicedisplayname = "$service.display_name$"
    vars.notification_serviceoutput = "$service.output$"
    vars.notification_servicestate = "$service.state$"
    vars.notification_type = "$notification.type$"
```

## Apply rule
An example apply rule:

```
apply Notification "Sysops service problems - matrix" to Service {
    import "Generic Service Alarm by Matrix"

    interval = 0s
    period = "24x7"
    assign where "sysops" in service.vars.sections
    user_groups = [ "Sysops" ]
    vars.notification_matrix_room_id = "!<id>:matrix.example.com"
}
```
