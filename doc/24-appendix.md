# Appendix <a id="appendix"></a>

## External Commands List <a id="external-commands-list-detail"></a>

Additional details can be found in the [Icinga 1.x Documentation](https://docs.icinga.com/latest/en/extcommands2.html)

  Command name                              | Parameters                        | Description
  ------------------------------------------|-----------------------------------|--------------------------
  PROCESS_HOST_CHECK_RESULT                 | ;&lt;host_name&gt;;&lt;status_code&gt;;&lt;plugin_output&gt; (3) | -
  PROCESS_SERVICE_CHECK_RESULT              | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;return_code&gt;;&lt;plugin_output&gt; (4) | -
  SCHEDULE_HOST_CHECK                       | ;&lt;host_name&gt;;&lt;check_time&gt; (2)  | -
  SCHEDULE_FORCED_HOST_CHECK                | ;&lt;host_name&gt;;&lt;check_time&gt; (2)  | -
  SCHEDULE_SVC_CHECK                        | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_time&gt; (3)  | -
  SCHEDULE_FORCED_SVC_CHECK                 | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_time&gt; (3)  | -
  ENABLE_HOST_CHECK                         | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOST_CHECK                        | ;&lt;host_name&gt; (1) | -
  ENABLE_SVC_CHECK                          | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  DISABLE_SVC_CHECK                         | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  SHUTDOWN_PROCESS                          | - | -
  RESTART_PROCESS                           | - | -
  SCHEDULE_FORCED_HOST_SVC_CHECKS           | ;&lt;host_name&gt;;&lt;check_time&gt; (2)  | -
  SCHEDULE_HOST_SVC_CHECKS                  | ;&lt;host_name&gt;;&lt;check_time&gt; (2)  | -
  ENABLE_HOST_SVC_CHECKS                    | ;&lt;host_name&gt; (1) | -
  DISABLE_HOST_SVC_CHECKS                   | ;&lt;host_name&gt; (1) | -
  ACKNOWLEDGE_SVC_PROBLEM                   | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;sticky&gt;;&lt;notify&gt;;&lt;persistent&gt;;&lt;author&gt;;&lt;comment&gt; (7) | Note: Icinga 2 treats all comments as persistent.
  ACKNOWLEDGE_SVC_PROBLEM_EXPIRE            | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;sticky&gt;;&lt;notify&gt;;&lt;persistent&gt;;&lt;timestamp&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | Note: Icinga 2 treats all comments as persistent.
  REMOVE_SVC_ACKNOWLEDGEMENT                | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  ACKNOWLEDGE_HOST_PROBLEM                  | ;&lt;host_name&gt;;&lt;sticky&gt;;&lt;notify&gt;;&lt;persistent&gt;;&lt;author&gt;;&lt;comment&gt; (6) | Note: Icinga 2 treats all comments as persistent.
  ACKNOWLEDGE_HOST_PROBLEM_EXPIRE           | ;&lt;host_name&gt;;&lt;sticky&gt;;&lt;notify&gt;;&lt;persistent&gt;;&lt;timestamp&gt;;&lt;author&gt;;&lt;comment&gt; (7) | Note: Icinga 2 treats all comments as persistent.
  REMOVE_HOST_ACKNOWLEDGEMENT               | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOST_FLAP_DETECTION               | ;&lt;host_name&gt; (1)  | -
  ENABLE_HOST_FLAP_DETECTION                | ;&lt;host_name&gt; (1)  | -
  DISABLE_SVC_FLAP_DETECTION                | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  ENABLE_SVC_FLAP_DETECTION                 | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  ENABLE_HOSTGROUP_SVC_CHECKS               | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_HOSTGROUP_SVC_CHECKS              | ;&lt;hostgroup_name&gt; (1)  | -
  ENABLE_SERVICEGROUP_SVC_CHECKS            | ;&lt;servicegroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_SVC_CHECKS           | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_PASSIVE_HOST_CHECKS                | ;&lt;host_name&gt; (1)  | -
  DISABLE_PASSIVE_HOST_CHECKS               | ;&lt;host_name&gt; (1)  | -
  ENABLE_PASSIVE_SVC_CHECKS                 | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  DISABLE_PASSIVE_SVC_CHECKS                | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS    | ;&lt;servicegroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS   | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS       | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS      | ;&lt;hostgroup_name&gt; (1)  | -
  PROCESS_FILE                              | ;&lt;file_name&gt;;&lt;delete&gt; (2)  | -
  SCHEDULE_SVC_DOWNTIME                     | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (9)  | -
  DEL_SVC_DOWNTIME                          | ;&lt;downtime_id&gt; (1)   | -
  SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME      | ;&lt;host_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME | ;&lt;host_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_HOST_DOWNTIME                    | ;&lt;host_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  DEL_HOST_DOWNTIME                         | ;&lt;downtime_id&gt; (1)  | -
  DEL_DOWNTIME_BY_HOST_NAME                 | ;&lt;host_name&gt;[;&lt;service_name;&gt;[;&lt;start_time;&gt;[;&lt;comment_text;&gt;]]] (1)  | -
  SCHEDULE_HOST_SVC_DOWNTIME                | ;&lt;host_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_HOSTGROUP_HOST_DOWNTIME          | ;&lt;hostgroup_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_HOSTGROUP_SVC_DOWNTIME           | ;&lt;hostgroup_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_SERVICEGROUP_HOST_DOWNTIME       | ;&lt;servicegroup_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  SCHEDULE_SERVICEGROUP_SVC_DOWNTIME        | ;&lt;servicegroup_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  ADD_HOST_COMMENT                          | ;&lt;host_name&gt;;&lt;persistent&gt;;&lt;author&gt;;&lt;comment&gt; (4)  | Note: Icinga 2 treats all comments as persistent.
  DEL_HOST_COMMENT                          | ;&lt;comment_id&gt; (1)  | -
  ADD_SVC_COMMENT                           | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;persistent&gt;;&lt;author&gt;;&lt;comment&gt; (5)  | Note: Icinga 2 treats all comments as persistent.
  DEL_SVC_COMMENT                           | ;&lt;comment_id&gt; (1)  | -
  DEL_ALL_HOST_COMMENTS                     | ;&lt;host_name&gt; (1)  | -
  DEL_ALL_SVC_COMMENTS                      | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  SEND_CUSTOM_HOST_NOTIFICATION             | ;&lt;host_name&gt;;&lt;options&gt;;&lt;author&gt;;&lt;comment&gt; (4)  | -
  SEND_CUSTOM_SVC_NOTIFICATION              | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;options&gt;;&lt;author&gt;;&lt;comment&gt; (5)  | -
  DELAY_HOST_NOTIFICATION                   | ;&lt;host_name&gt;;&lt;notification_time&gt; (2)  | -
  DELAY_SVC_NOTIFICATION                    | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;notification_time&gt; (3)  | -
  ENABLE_HOST_NOTIFICATIONS                 | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOST_NOTIFICATIONS                | ;&lt;host_name&gt; (1)  | -
  ENABLE_SVC_NOTIFICATIONS                  | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  DISABLE_SVC_NOTIFICATIONS                 | ;&lt;host_name&gt;;&lt;service_name&gt; (2) | -
  ENABLE_HOST_SVC_NOTIFICATIONS		    | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOST_SVC_NOTIFICATIONS	    | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOSTGROUP_HOST_CHECKS             | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_HOSTGROUP_PASSIVE_HOST_CHECKS     | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_HOST_CHECKS          | ;&lt;servicegroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS  | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_HOSTGROUP_HOST_CHECKS              | ;&lt;hostgroup_name&gt; (1)  | -
  ENABLE_HOSTGROUP_PASSIVE_HOST_CHECKS      | ;&lt;hostgroup_name&gt; (1) | -
  ENABLE_SERVICEGROUP_HOST_CHECKS           | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS   | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_NOTIFICATIONS                      | -  | -
  DISABLE_NOTIFICATIONS                     | -   | -
  ENABLE_FLAP_DETECTION                     | -  | -
  DISABLE_FLAP_DETECTION                    | -  | -
  ENABLE_EVENT_HANDLERS                     | -  | -
  DISABLE_EVENT_HANDLERS                    | -  | -
  ENABLE_PERFORMANCE_DATA                   | -  | -
  DISABLE_PERFORMANCE_DATA                  | -  | -
  START_EXECUTING_HOST_CHECKS               | -  | -
  STOP_EXECUTING_HOST_CHECKS                | -  | -
  START_EXECUTING_SVC_CHECKS                | -  | -
  STOP_EXECUTING_SVC_CHECKS                 | -  | -
  CHANGE_NORMAL_SVC_CHECK_INTERVAL          | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_interval&gt; (3)  | -
  CHANGE_NORMAL_HOST_CHECK_INTERVAL         | ;&lt;host_name&gt;;&lt;check_interval&gt; (2)  | -
  CHANGE_RETRY_SVC_CHECK_INTERVAL           | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_interval&gt; (3)  | -
  CHANGE_RETRY_HOST_CHECK_INTERVAL          | ;&lt;host_name&gt;;&lt;check_interval&gt; (2) | -
  ENABLE_HOST_EVENT_HANDLER                 | ;&lt;host_name&gt; (1)  | -
  DISABLE_HOST_EVENT_HANDLER                | ;&lt;host_name&gt; (1)  | -
  ENABLE_SVC_EVENT_HANDLER                  | ;&lt;host_name&gt;;&lt;service_name&gt; (2)  | -
  DISABLE_SVC_EVENT_HANDLER                 | ;&lt;host_name&gt;;&lt;service_name&gt; (2) | -
  CHANGE_HOST_EVENT_HANDLER                 | ;&lt;host_name&gt;;&lt;event_command_name&gt; (2)  | -
  CHANGE_SVC_EVENT_HANDLER                  | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;event_command_name&gt; (3)  | -
  CHANGE_HOST_CHECK_COMMAND                 | ;&lt;host_name&gt;;&lt;check_command_name&gt; (2)  | -
  CHANGE_SVC_CHECK_COMMAND                  | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_command_name&gt; (3)  | -
  CHANGE_MAX_HOST_CHECK_ATTEMPTS            | ;&lt;host_name&gt;;&lt;check_attempts&gt; (2)  | -
  CHANGE_MAX_SVC_CHECK_ATTEMPTS             | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;check_attempts&gt; (3)  | -
  CHANGE_HOST_CHECK_TIMEPERIOD              | ;&lt;host_name&gt;;&lt;timeperiod_name&gt; (2)   | -
  CHANGE_SVC_CHECK_TIMEPERIOD               | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;timeperiod_name&gt;  | -
  CHANGE_CUSTOM_HOST_VAR                    | ;&lt;host_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (3)  | -
  CHANGE_CUSTOM_SVC_VAR                     | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (4)  | -
  CHANGE_CUSTOM_USER_VAR                    | ;&lt;user_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (3)  | -
  CHANGE_CUSTOM_CHECKCOMMAND_VAR            | ;&lt;check_command_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (3)  | -
  CHANGE_CUSTOM_EVENTCOMMAND_VAR            | ;&lt;event_command_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (3)  | -
  CHANGE_CUSTOM_NOTIFICATIONCOMMAND_VAR     | ;&lt;notification_command_name&gt;;&lt;var_name&gt;;&lt;var_value&gt; (3)  | -
  ENABLE_HOSTGROUP_HOST_NOTIFICATIONS       | ;&lt;hostgroup_name&gt; (1) | -
  ENABLE_HOSTGROUP_SVC_NOTIFICATIONS        | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_HOSTGROUP_HOST_NOTIFICATIONS      | ;&lt;hostgroup_name&gt; (1)  | -
  DISABLE_HOSTGROUP_SVC_NOTIFICATIONS       | ;&lt;hostgroup_name&gt; (1)  | -
  ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS    | ;&lt;servicegroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS   | ;&lt;servicegroup_name&gt; (1)  | -
  ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS     | ;&lt;servicegroup_name&gt; (1)  | -
  DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS    | ;&lt;servicegroup_name&gt; (1)  | -


## Schemas <a id="schemas"></a>

By convention `CheckCommand`, `EventCommand`, and `NotificationCommand` objects
are exported using a prefix. This is mandatory for unique objects in the
command tables.

Object                  | Prefix
------------------------|------------------------
CheckCommand            | check\_
EventCommand            | event\_
NotificationCommand     | notification\_

### DB IDO Schema <a id="schema-db-ido"></a>

There is a detailed documentation for the Icinga IDOUtils 1.x
database schema available on [https://docs.icinga.com/latest/en/db_model.html]

#### DB IDO Schema Extensions <a id="schema-db-ido-extensions"></a>

Icinga 2 specific extensions are shown below:

New table: `endpointstatus`

  Table               | Column             | Type     | Default | Description
  --------------------|--------------------|----------|---------|-------------
  endpoints           | endpoint_object_id | bigint   | NULL    | FK: objects table
  endpoints           | identity           | TEXT     | NULL    | endpoint name
  endpoints           | node               | TEXT     | NULL    | local node name
  endpoints           | zone_object_id     | bigint   | NULL    | zone object where this endpoint is a member of

New table: `endpointstatus`

  Table               | Column             | Type     | Default | Description
  --------------------|--------------------|----------|---------|-------------
  endpointstatus      | endpoint_object_id | bigint   | NULL    | FK: objects table
  endpointstatus      | identity           | TEXT     | NULL    | endpoint name
  endpointstatus      | node               | TEXT     | NULL    | local node name
  endpointstatus      | is_connected       | smallint | 0       | update on endpoint connect/disconnect
  endpointstatus      | zone_object_id     | bigint   | NULL    | zone object where this endpoint is a member of

New tables: `zones` and `zonestatus`:

  Table               | Column             | Type     | Default | Description
  --------------------|--------------------|----------|---------|-------------
  zones               | zone_object_id     | bigint   | NULL    | FK: objects table
  zones               | parent_zone_object_id | bigint   | NULL    | FK: zones table
  zones               | is_global          | smallint | 0       | zone is global


New columns:

  Table               | Column                  | Type     | Default | Description
  --------------------|-------------------------|----------|---------|-------------
  all status/history  | endpoint_object_id      | bigint   | NULL    | FK: objects table
  servicestatus       | check_source            | TEXT     | NULL    | node name where check was executed
  hoststatus          | check_source            | TEXT     | NULL    | node name where check was executed
  statehistory        | check_source            | TEXT     | NULL    | node name where check was executed
  servicestatus       | is_reachable            | integer  | NULL    | object reachability
  hoststatus          | is_reachable            | integer  | NULL    | object reachability
  logentries          | object_id               | bigint   | NULL    | FK: objects table (service associated with column)
  {host,service}group | notes                   | TEXT     | NULL    | -
  {host,service}group | notes_url               | TEXT     | NULL    | -
  {host,service}group | action_url              | TEXT     | NULL    | -
  customvariable*     | is_json			| integer  | 0	     | Defines whether `varvalue` is a json encoded string from custom variables, or not
  servicestatus       | original_attributes     | TEXT     | NULL    | JSON encoded dictionary of original attributes if modified at runtime.
  hoststatus          | original_attributes     | TEXT     | NULL    | JSON encoded dictionary of original attributes if modified at runtime.

Additional command custom variables populated from 'vars' dictionary.
Additional global custom variables populated from 'Vars' constant (object_id is NULL).

### Livestatus Schema <a id="schema-livestatus"></a>

#### Livestatus Schema Extensions <a id="schema-livestatus-extensions"></a>

Icinga 2 specific extensions are shown below:

New table: `endpoints`:

  Table     | Column
  ----------|--------------
  endpoints | name
  endpoints | identity
  endpoints | node
  endpoints | is_connected
  endpoints | zone

New table: `zones`:

  Table     | Column
  ----------|--------------
  zone      | name
  zone      | endpoints
  zone      | parent
  zone      | global

New columns:

  Table     | Column
  ----------|--------------
  hosts     | is_reachable
  services  | is_reachable
  hosts	    | cv_is_json
  services  | cv_is_json
  contacts  | cv_is_json
  hosts     | check_source
  services  | check_source
  downtimes | triggers
  downtimes | trigger_time
  commands  | custom_variable_names
  commands  | custom_variable_values
  commands  | custom_variables
  commands  | modified_attributes
  commands  | modified_attributes_list
  status    | custom_variable_names
  status    | custom_variable_values
  status    | custom_variables
  hosts     | original_attributes
  services  | original_attributes

Command custom variables reflect the local 'vars' dictionary.
Status custom variables reflect the global 'Vars' constant.

#### Livestatus Hosts Table Attributes <a id="schema-livestatus-hosts-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  display_name          | string    | .
  alias                 | string    | same as display_name.
  address               | string    | .
  address6              | string    | NEW in Icinga.
  check_command         | string    | .
  check_command_expanded | string   | .
  event_handler         | string    | .
  notification_period   | string    | host with notifications: period.
  check_period          | string    | .
  notes                 | string    | .
  notes_expanded        | string    | .
  notes_url             | string    | .
  notes_url_expanded    | string    | .
  action_url            | string    | .
  action_url_expanded   | string    | .
  plugin_output         | string    | .
  perf_data             | string    | .
  icon_image            | string    | .
  icon_image_expanded   | string    | .
  icon_image_alt        | stirng    | .
  statusmap_image       | string    | .
  long_plugin_output    | string    | .
  max_check_attempts    | int       | .
  flap_detection_enabled | int      | .
  check_freshness       | int       | .
  process_performance_data | int    | .
  accept_passive_checks | int       | .
  event_handler_enabled | int       | .
  acknowledgement_type  | int       | Only 0 or 1.
  check_type            | int       | .
  last_state            | int       | .
  last_hard_state       | int       | .
  current_attempt       | int       | .
  last_notification     | int       | host with notifications: last notification.
  next_notification     | int       | host with notifications: next notification.
  next_check            | int       | .
  last_hard_state_change | int      | .
  has_been_checked      | int       | .
  current_notification_number | int | host with notifications: number.
  total_services        | int       | .
  checks_enabled        | int       | .
  notifications_enabled | int       | .
  acknowledged          | int       | .
  state                 | int       | .
  state_type            | int       | .
  no_more_notifications | int       | notification_interval == 0 && volatile == false.
  last_check            | int       | .
  last_state_change     | int       | .
  last_time_up          | int       | .
  last_time_down        | int       | .
  last_time_unreachable | int       | .
  is_flapping           | int       | .
  scheduled_downtime_depth | int    | .
  active_checks_enabled | int       | .
  modified_attributes   | array     | .
  modified_attributes_list | array  | .
  check_interval        | double    | .
  retry_interval        | double    | .
  notification_interval | double    | host with notifications: smallest interval.
  low_flap_threshold    | double    | flapping_threshold
  high_flap_threshold   | double    | flapping_threshold
  latency               | double    | .
  execution_time        | double    | .
  percent_state_change  | double    | flapping.
  in_notification_period | int      | host with notifications: matching period.
  in_check_period       | int       | .
  contacts              | array     | host with notifications, users and user groups.
  downtimes             | array     | id.
  downtimes_with_info   | array     | id+author+comment.
  comments              | array     | id.
  comments_with_info    | array     | id+author+comment.
  comments_with_extra_info | array  | id+author+comment+entry_type+entry_time.
  custom_variable_names | array     | .
  custom_variable_values | array    | .
  custom_variables      | array     | Array of custom variable array pair.
  parents               | array     | Direct host parents.
  childs                | array     | Direct host children (Note: `childs` is inherited from the origin MK_Livestatus protocol).
  num_services          | int       | .
  worst_service_state   | int       | All services and their worst state.
  num_services_ok       | int       | All services with Ok state.
  num_services_warn     | int       | All services with Warning state.
  num_services_crit     | int       | All services with Critical state.
  num_services_unknown  | int       | All services with Unknown state.
  worst_service_hard_state | int    | All services and their worst hard state.
  num_services_hard_ok  | int       | All services in a hard state with Ok state.
  num_services_hard_warn | int      | All services in a hard state with Warning state.
  num_services_hard_crit | int      | All services in a hard state with Critical state.
  num_services_hard_unknown  | int  | All services in a hard state with Unknown state.
  hard_state            | int       | Returns OK if state is OK. Returns current state if now a hard state type. Returns last hard state otherwise.
  staleness             | int       | Indicates time since last check normalized onto the check_interval.
  groups                | array     | All hostgroups this host is a member of.
  contact_groups        | array     | All usergroups associated with this host through notifications.
  services              | array     | All services associated with this host.
  services_with_state   | array     | All services associated with this host with state and hasbeenchecked.
  services_with_info    | array     | All services associated with this host with state, hasbeenchecked and output.

Not supported: `initial_state`, `pending_flex_downtime`, `check_flapping_recovery_notification`,
`is_executing`, `check_options`, `obsess_over_host`, `first_notification_delay`, `x_3d`, `y_3d`, `z_3d`,
`x_2d`, `y_2d`, `filename`, `pnpgraph_present`.

#### Livestatus Hostgroups Table Attributes <a id="schema-livestatus-hostgroups-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  alias                 | string    | `display_name` attribute.
  notes                 | string    | .
  notes_url             | string    | .
  action_url            | string    | .
  members               | array     | .
  members_with_state    | array     | Host name and state.
  worst_host_state      | int       | Of all group members.
  num_hosts             | int       | In this group.
  num_hosts_pending     | int       | .
  num_hosts_up          | int       | .
  num_hosts_down        | int       | .
  num_hosts_unreach     | int       | .
  num_services          | int       | Number of services associated with hosts in this hostgroup.
  worst_services_state  | int       | .
  num_services_pending  | int       | .
  num_services_ok       | int       | .
  num_services_warn     | int       | .
  num_services_crit     | int       | .
  num_services_unknown  | int       | .
  worst_service_hard_state | int    | .
  num_services_hard_ok | int        | .
  num_services_hard_warn | int      | .
  num_services_hard_crit | int      | .
  num_services_hard_unknown | int   | .

#### Livestatus Services Table Attributes <a id="schema-livestatus-services-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  description           | string    | .
  display_name          | string    | .
  alias                 | string    | same as display_name.
  check_command         | string    | .
  check_command_expanded | string   | .
  event_handler         | string    | .
  notification_period   | string    | host with notifications: period.
  check_period          | string    | .
  notes                 | string    | .
  notes_expanded        | string    | .
  notes_url             | string    | .
  notes_url_expanded    | string    | .
  action_url            | string    | .
  action_url_expanded   | string    | .
  plugin_output         | string    | .
  perf_data             | string    | .
  icon_image            | string    | .
  icon_image_expanded   | string    | .
  icon_image_alt        | stirng    | .
  statusmap_image       | string    | .
  long_plugin_output    | string    | .
  max_check_attempts    | int       | .
  flap_detection_enabled | int      | .
  check_freshness       | int       | .
  process_performance_data | int    | .
  accept_passive_checks | int       | .
  event_handler_enabled | int       | .
  acknowledgement_type  | int       | Only 0 or 1.
  check_type            | int       | .
  last_state            | int       | .
  last_hard_state       | int       | .
  current_attempt       | int       | .
  last_notification     | int       | service with notifications: last notification.
  next_notification     | int       | service with notifications: next notification.
  next_check            | int       | .
  last_hard_state_change | int      | .
  has_been_checked      | int       | .
  current_notification_number | int | service with notifications: number.
  checks_enabled        | int       | .
  notifications_enabled | int       | .
  acknowledged          | int       | .
  state                 | int       | .
  state_type            | int       | .
  no_more_notifications | int       | notification_interval == 0 && volatile == false.
  last_check            | int       | .
  last_state_change     | int       | .
  last_time_ok          | int       | .
  last_time_warning     | int       | .
  last_time_critical    | int       | .
  last_time_unknown     | int       | .
  is_flapping           | int       | .
  scheduled_downtime_depth | int    | .
  active_checks_enabled | int       | .
  modified_attributes   | array     | .
  modified_attributes_list | array  | .
  check_interval        | double    | .
  retry_interval        | double    | .
  notification_interval | double    | service with notifications: smallest interval.
  low_flap_threshold    | double    | flapping_threshold
  high_flap_threshold   | double    | flapping_threshold
  latency               | double    | .
  execution_time        | double    | .
  percent_state_change  | double    | flapping.
  in_notification_period | int      | service with notifications: matching period.
  in_check_period       | int       | .
  contacts              | array     | service with notifications, users and user groups.
  downtimes             | array     | id.
  downtimes_with_info   | array     | id+author+comment.
  comments              | array     | id.
  comments_with_info    | array     | id+author+comment.
  comments_with_extra_info | array  | id+author+comment+entry_type+entry_time.
  custom_variable_names | array     | .
  custom_variable_values | array    | .
  custom_variables      | array     | Array of custom variable array pair.
  hard_state            | int       | Returns OK if state is OK. Returns current state if now a hard state type. Returns last hard state otherwise.
  staleness             | int       | Indicates time since last check normalized onto the check_interval.
  groups                | array     | All hostgroups this host is a member of.
  contact_groups        | array     | All usergroups associated with this host through notifications.
  host_                 | join      | Prefix for attributes from implicit join with hosts table.

Not supported: `initial_state`, `is_executing`, `check_options`, `obsess_over_service`, `first_notification_delay`,
`pnpgraph_present`.

#### Livestatus Servicegroups Table Attributes <a id="schema-livestatus-servicegroups-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  alias                 | string    | `display_name` attribute.
  notes                 | string    | .
  notes_url             | string    | .
  action_url            | string    | .
  members               | array     | CSV format uses `host|service` syntax.
  members_with_state    | array     | Host, service, hoststate, servicestate.
  worst_service_state   | int       | .
  num_services          | int       | .
  num_services_pending  | int       | .
  num_services_ok       | int       | .
  num_services_warn     | int       | .
  num_services_crit     | int       | .
  num_services_unknown  | int       | .
  num_services_hard_ok | int        | .
  num_services_hard_warn | int      | .
  num_services_hard_crit | int      | .
  num_services_hard_unknown | int   | .

#### Livestatus Contacts Table Attributes <a id="schema-livestatus-contacts-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  alias                 | string    | `display_name` attribute.
  email                 | string    | .
  pager                 | string    | .
  host_notification_period | string | .
  service_notification_period | string | .
  host_notifications_enabled | int | .
  service_notifications_enabled | int | .
  in_host_notification_period | int | .
  in_service_notification_period | int | .
  custom_variable_names | array     | .
  custom_variable_values | array    | .
  custom_variables      | array     | Array of customvariable array pairs.
  modified_attributes   | array     | .
  modified_attributes_list | array  | .


Not supported: `can_submit_commands`.

#### Livestatus Contactgroups Table Attributes <a id="schema-livestatus-contactgroups-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  alias                 | string    | `display_name` attribute.
  members               | array     | .


#### Livestatus Commands Table Attributes <a id="schema-livestatus-commands-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | 3 types of commands in Icinga 2.
  line                  | string    | .


#### Livestatus Status Table Attributes <a id="schema-livestatus-status-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  connections           | int       | Since application start.
  connections_rate      | double    | .
  service_checks        | int       | Since application start.
  service_checks_rate   | double    | .
  host_checks           | int       | Since application start.
  host_checks_rate      | double    | .
  external_commands     | int       | Since application start.
  external_commands_rate | double   | .
  nagios_pid            | string    | Application PID.
  enable_notifications  | int       | .
  execute_service_checks | int      | .
  accept_passive_service_checks | int | .
  execute_host_checks   | int       | .
  accept_passive_host_checks | int  | .
  enable_event_handlers | int       | .
  check_service_freshness | int     | .
  check_host_freshness  | int       | .
  enable_flap_detection | int       | .
  process_performance_data | int    | .
  check_external_commands | int     | Always enabled.
  program_start         | int       | In seconds.
  last_command_check    | int       | Always.
  interval_length       | int       | Compatibility mode: 60.
  num_hosts             | int       | .
  num_services          | int       | .
  program_version       | string    | 2.0.
  livestatus_active_connections | string | .

Not supported: `neb_callbacks`, `neb_callbacks_rate`, `requests`, `requests_rate`, `forks`, `forks_rate`,
`log_messages`, `log_messages_rate`, `livechecks`, `livechecks_rate`, `livecheck_overflows`,
`livecheck_overflows_rate`, `obsess_over_services`, `obsess_over_hosts`, `last_log_rotation`,
`external_command_buffer_slots`, `external_command_buffer_usage`, `external_command_buffer_max`,
`cached_log_messages`, `livestatus_queued_connections`, `livestatus_threads`.


#### Livestatus Comments Table Attributes <a id="schema-livestatus-comments-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  author                | string    | .
  comment               | string    | .
  id                    | int       | legacy_id.
  entry_time            | string    | Seconds.
  type                  | int       | 1=host, 2=service.
  is_service            | int       | .
  persistent            | int       | Always.
  source                | string    | Always external (1).
  entry_type            | int       | .
  expires               | int       | .
  expire_time           | string    | Seconds.
  service_              | join      | Prefix for attributes from implicit join with services table.
  host_                 | join      | Prefix for attributes from implicit join with hosts table.


#### Livestatus Downtimes Table Attributes <a id="schema-livestatus-downtimes-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  author                | string    | .
  comment               | string    | .
  id                    | int       | legacy_id.
  entry_time            | string    | Seconds.
  type                  | int       | 1=active, 0=pending.
  is_service            | int       | .
  start_time            | string    | Seconds.
  end_time              | string    | Seconds.
  fixed                 | int       | 0=flexible, 1=fixed.
  duration              | int       | .
  triggered_by          | int       | legacy_id.
  triggers              | int       | NEW in Icinga 2.
  trigger_time          | string    | NEW in Icinga 2.
  service_              | join      | Prefix for attributes from implicit join with services table.
  host_                 | join      | Prefix for attributes from implicit join with hosts table.


#### Livestatus Timeperiods Table Attributes <a id="schema-livestatus-timeperiods-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  name                  | string    | .
  alias                 | string    | `display_name` attribute.
  in                    | int       | Current time is in timeperiod or not.

#### Livestatus Log Table Attributes <a id="schema-livestatus-log-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  time                  | int       | Time of log event (unix timestamp).
  lineno                | int       | Line number in `CompatLogger` log file.
  class                 | int       | Log message class: 0=info, 1=state, 2=program, 3=notification, 4=passive, 5=command.
  message               | string    | Complete message line.
  type                  | string    | Text before the colon `:`.
  options               | string    | Text after the colon `:`.
  comment               | string    | Comment if available.
  plugin_output         | string    | Check output if available.
  state                 | int       | Host or service state.
  state_type            | int       | State type if available.
  attempt               | int       | Current check attempt.
  service_description   | string    | .
  host_name             | string    | .
  contact_name          | string    | .
  command_name          | string    | .
  current_service_      | join      | Prefix for attributes from implicit join with services table.
  current_host_         | join      | Prefix for attributes from implicit join with hosts table.
  current_contact_      | join      | Prefix for attributes from implicit join with contacts table.
  current_command_      | join      | Prefix for attributes from implicit join with commands table.

#### Livestatus Statehist Table Attributes <a id="schema-livestatus-statehist-table-attributes"></a>

  Key                   | Type      | Note
  ----------------------|-----------|-------------------------
  time                  | int       | Time of log event (unix timestamp).
  lineno                | int       | Line number in `CompatLogger` log file.
  from                  | int       | Start timestamp (unix timestamp).
  until                 | int       | End timestamp (unix timestamp).
  duration              | int       | until-from.
  duration_part         | double    | duration / query_part.
  state                 | int       | State: 0=ok, 1=warn, 2=crit, 3=unknown, -1=notmonitored.
  host_down             | int       | Host associated with the service is down or not.
  in_downtime           | int       | Host/service is in downtime.
  in_host_downtime      | int       | Host associated with the service is in a downtime or not.
  is_flapping           | int       | Host/service is flapping.
  in_notification_period | int      | Host/service notification periods match or not.
  notification_period   | string    | Host/service notification period.
  host_name             | string    | .
  service_description   | string    | .
  log_output            | string    | Log file output for this state.
  duration_ok           | int       | until-from for OK state.
  duration_part_ok      | double    | .
  duration_warning      | int       | until-from for Warning state.
  duration_part_warning | double    | .
  duration_critical     | int       | until-from for Critical state.
  duration_part_critical | double    | .
  duration_unknown      | int       | until-from for Unknown state.
  duration_part_unknown | double    | .
  duration_unmonitored  | int       | until-from for Not-Monitored state.
  duration_part_unmonitored | double    | .
  current_service_      | join      | Prefix for attributes from implicit join with services table.
  current_host_         | join      | Prefix for attributes from implicit join with hosts table.

Not supported: `debug_info`.

#### Livestatus Hostsbygroup Table Attributes <a id="schema-livestatus-hostsbygroup-table-attributes"></a>

All [hosts](24-appendix.md#schema-livestatus-hosts-table-attributes) table attributes grouped with
the [hostgroups](24-appendix.md#schema-livestatus-hostgroups-table-attributes) table prefixed with `hostgroup_`.

#### Livestatus Servicesbygroup Table Attributes <a id="schema-livestatus-servicesbygroup-table-attributes"></a>

All [services](24-appendix.md#schema-livestatus-services-table-attributes) table attributes grouped with
the [servicegroups](24-appendix.md#schema-livestatus-servicegroups-table-attributes) table prefixed with `servicegroup_`.

#### Livestatus Servicesbyhostgroup Table Attributes <a id="schema-livestatus-servicesbyhostgroup-table-attributes"></a>

All [services](24-appendix.md#schema-livestatus-services-table-attributes) table attributes grouped with
the [hostgroups](24-appendix.md#schema-livestatus-hostgroups-table-attributes) table prefixed with `hostgroup_`.
