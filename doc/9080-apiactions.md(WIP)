###API ACTIONS


Action name                            | Parameters                        | Target types             | Notes
---------------------------------------|-----------------------------------|--------------------------|-----------------------
process-check-result                   | exit_status; plugin_output; check_source; performance_data[]; check_command[]; execution_end; execution_start; schedule_end; schedule_start | Service; Host | -
reschedule-check                       | {next_check}; {(force_check)} | Service; Host | -
acknowledge-problem                    | author; comment; {timestamp}; {(sticky)}; {(notify)} | Service; Host | -
remove-acknowledgement                 | - | Service; Host | -
add-comment                            | author; comment | Service; Host | -
remove-comment                         | comment_id | - | -
remove-all-comments                    | - | Service; Host | -
delay-notifications                    | timestamp | Service;Host | -
add-downtime                           | start_time; end_time; duration; author; comment; {trigger_id}; {(fixed)} | Service; Host; ServiceGroup; HostGroup | Downtime for all services on host x?
remove-downtime                        | downtime_id | - | remove by name?
send-custom-notification               | options[]; author; comment | Service; Host | -

enable-passive-checks                  | - | Service; Host; ServiceGroup; HostGroup | "System" as target?
disable-passive-checks                 | - | Service; Host; ServiceGroup; HostGroup | diable all passive checks for services of hosts y in hostgroup x?
enable-active-checks                   | - | Host; HostGroup | -
disable-active-checks                  | - | Host; HostGroup | -
enable-notifications                   | - | Service; Host; ServiceGroup; HostGroup | Enable all notifications for services of host x?
disable-notifications                  | - | Service; Host; ServiceGroup; HostGroup | -
enable-flap-detection                  | - | Service; Host; ServiceGroup; HostGroup | -
disable-flap-detection                 | - | Service; Host; ServiceGroup; HostGroup | -
enable-event-handler                   | - | Service; Host | -
disable-event-handler                  | - | Service; Host | -

change-custom-vor                      | var_name; var_value | Service; Host; User; Checkcommand; Eventcommand; Notificationcommand
change-mod-attribute                   | mod_value | Service; Host; User; Checkcommand; Eventcommand; Notificationcommand
change-event-handler                   | {event_command_name} | Service; Host | -
change-check-command                   | check_command_name | Service; Host | -
change-max-check-attempts              | max_check_attempts | Service; Host | -
change-check-interval                  | check_interval | Service; Host | -
change-retry-interval                  | retry_interval | Service; Host | -
change-check-period                    | time_period_name | Service; Host | -

enable-all-notifications               | - | - | -
disable-all-notifications              | - | - | -
enable-all-flap-detection              | - | - | -
disable-all-flap-detection             | - | - | -
enable-all-event-handlers              | - | - | -
disable-all-event-handlers             | - | - | -
enable-all-performance-data            | - | - | -
disable-all-performance-data           | - | - | -
start-all-executing-svc-checks         | - | - | -
stop-all-executing-svc-checks          | - | - | -
start-all-executing-host-checks        | - | - | -
stop-all-executing-host-checks         | - | - | -
shutdown-process                       | - | - | -
restart-process                        | - | - | -
process-file                           | - | - | -
