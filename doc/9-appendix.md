# <a id="appendix"></a> Appendix

## <a id="icinga-template-library"></a> Icinga Template Library

### <a id="itl-overview"></a> Overview

The Icinga Template Library (ITL) implements standard templates and object
definitions for commonly used services.

You can include the ITL by using the `include` directive in your configuration
file:

    include <itl>

### <a id="itl-generic-templates"></a> Generic Templates

These templates are imported by the provided example configuration.

#### <a id="itl-plugin-check-command"></a> plugin-check-command

Command template for check plugins executed by Icinga 2.

The `plugin-check-command` command does not support any vars.

#### <a id="itl-plugin-notification-command"></a> plugin-notification-command

Command template for notification scripts executed by Icinga 2.

The `plugin-notification-command` command does not support any vars.

#### <a id="itl-plugin-event-command"></a> plugin-event-command

Command template for event handler scripts executed by Icinga 2.

The `plugin-event-command` command does not support any vars.

### <a id="itl-check-commands"></a> Check Commands

These check commands are embedded into Icinga 2 and do not require any external
plugin scripts.

#### <a id="itl-icinga"></a> icinga

Check command for the built-in `icinga` check. This check returns performance
data for the current Icinga instance.

The `icinga` check command does not support any vars.

#### <a id="itl-icinga"></a> cluster

Check command for the built-in `cluster` check. This check returns performance
data for the current Icinga instance and connected endpoints.

The `cluster` check command does not support any vars.



## <a id="plugin-check-commands"></a> Plugin Check Commands

### <a id="plugin-check-command-overview"></a> Overview

The Plugin Check Commands provides example configuration for plugin check commands
provided by the the Monitoring Plugins project.

You can include the plugin check command definitions by using the `include`
directive in your configuration file:

    include <plugins>

The plugin check commands assume that there's a global constant named `PluginDir`
which contains the path of the plugins from the Monitoring Plugins project.


#### <a id="plugin-check-command-ping4"></a> ping4

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-ping6"></a> ping6

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv6 address. Defaults to "$address6$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-hostalive"></a> hostalive

Check command object for the `check_ping` plugin with host check default values.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### <a id="plugin-check-command-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
dummy_text      | **Optional.** Plugin output. Defaults to "Check was successful.".

#### <a id="plugin-check-command-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
dummy_text      | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".

#### <a id="plugin-check-command-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
tcp_address     | **Optional.** The host's address. Defaults to "$address$".
tcp_port        | **Required.** The port that should be checked.

#### <a id="plugin-check-command-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
udp_address     | **Optional.** The host's address. Defaults to "$address$".
udp_port        | **Required.** The port that should be checked.

#### <a id="plugin-check-command-http-ip"></a> http

Check command object for the `check_http` plugin.

Custom Attributes:

Name               | Description
-------------------|--------------
http_address       | **Optional.** The host's address. Defaults to "$address".
http_vhost         | **Optional.** The virtual host that should be sent in the "Host" header.
http_uri           | **Optional.** The request URI.
http_port          | **Optional.** The TCP port. Defaults to 80 when not using SSL, 443 otherwise.
http_ssl           | **Optional.** Whether to use SSL. Defaults to false.
http_warn_time     | **Optional.** The warning threshold.
http_critical_time | **Optional.** The critical threshold.

#### <a id="plugin-check-command-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
smtp_address         | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssmtp_address   | **Required.** The host's address. Defaults to "$address$".
ssmtp_port      | **Optional.** The port that should be checked. Defaults to 465.

#### <a id="plugin-check-command-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$".

#### <a id="plugin-check-command-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssh_address     | **Optional.** The host's address. Defaults to "$address$".
ssh_port        | **Optional.** The port that should be checked. Defaults to 22.

#### <a id="plugin-check-command-disk"></a> disk

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
disk_wfree      | **Optional.** The free space warning threshold in %. Defaults to 20.
disk_cfree      | **Optional.** The free space critical threshold in %. Defaults to 10.

#### <a id="plugin-check-command-users"></a> users

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
users_wgreater  | **Optional.** The user count warning threshold. Defaults to 20.
users_cgreater  | **Optional.** The user count critical threshold. Defaults to 50.

#### <a id="plugin-check-command-processes"></a> procs

Check command object for the `check_procs` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
procs_wgreater  | **Optional.** The process count warning threshold. Defaults to 250.
procs_cgreater  | **Optional.** The process count critical threshold. Defaults to 400.

#### <a id="plugin-check-command-swap"></a> swap

Check command object for the `check_swap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
swap_wfree      | **Optional.** The free swap space warning threshold in %. Defaults to 50.
swap_cfree      | **Optional.** The free swap space critical threshold in %. Defaults to 25.

#### <a id="plugin-check-command-load"></a> load

Check command object for the `check_load` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
load_wload1     | **Optional.** The 1-minute warning threshold. Defaults to 5.
load_wload5     | **Optional.** The 5-minute warning threshold. Defaults to 4.
load_wload15    | **Optional.** The 15-minute warning threshold. Defaults to 3.
load_cload1     | **Optional.** The 1-minute critical threshold. Defaults to 10.
load_cload5     | **Optional.** The 5-minute critical threshold. Defaults to 6.
load_cload15    | **Optional.** The 15-minute critical threshold. Defaults to 4.

#### <a id="plugin-check-command-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Required.** The SNMP OID.
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

#### <a id="plugin-check-command-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

#### <a id="plugin-check-command-nscp"></a> nscp

Check command object for the `check_nt` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
nscp_address    | **Optional.** The host's address. Defaults to "$address$".
nscp_port       | **Optional.** The NSClient++ port. Defaults to 12489.
nscp_password   | **Optional.** The NSClient++ password.
nscp_variable   | **Required.** The variable that should be checked.
nscp_params     | **Optional.** Parameters for the query.
nscp_warn       | **Optional.** The warning threshold.
nscp_crit       | **Optional.** The critical threshold.
nscp_timeout    | **Optional.** The query timeout in seconds.

#### <a id="plugin-check-command-apt"></a> apt

Check command for the `check_apt` plugin.

The `apt` check command does not support any vars.


## <a id="schemas"></a> Schemas

By convention `CheckCommand`, `EventCommand` and `NotificationCommand` objects
are exported using a prefix. This is mandatory for unique objects in the
command tables.

Object                  | Prefix
------------------------|------------------------
CheckCommand            | check_
EventCommand            | event_
NotificationCommand     | notification_

### <a id="schema-status-files"></a> Status Files

Status files used by Icinga 1.x Classic UI: `status.dat`, `objects.cache`.

Icinga 2 specific extensions:

* host and service objects support 'check_source' (added in Classic UI 1.10.0)
* command objects support custom variables (added in Classic UI 1.11.2)
* host and service objects support 'is_reachable' (added in Classic UI 1.11.3)

### <a id="schema-db-ido"></a> DB IDO

There is a detailed documentation for the Icinga IDOUtils 1.x
database schema available on [http://docs.icinga.org/latest/en/db_model.html]

#### <a id="schema-db-ido-extensions"></a> DB IDO Schema Extensions

Icinga 2 specific extensions are shown below:

New tables: `endpoints`, `endpointstatus`

  Table               | Column             | Type     | Default | Description
  --------------------|--------------------|----------|---------|-------------
  endpoints           | endpoint_object_id | bigint   | NULL    | FK: objects table
  endpoints           | identity           | TEXT     | NULL    | endpoint name
  endpoints           | node               | TEXT     | NULL    | local node name

  Table               | Column             | Type     | Default | Description
  --------------------|--------------------|----------|---------|-------------
  endpointstatus      | endpoint_object_id | bigint   | NULL    | FK: objects table
  endpointstatus      | identity           | TEXT     | NULL    | endpoint name
  endpointstatus      | node               | TEXT     | NULL    | local node name
  endpointstatus      | is_connected       | smallint | 0       | update on endpoint connect/disconnect

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

Additional command custom variables populated from 'vars' dictionary.
Additional global custom variables populated from 'Vars' constant (object_id is NULL).


### <a id="schema-livestatus"></a> Livestatus


#### <a id="schema-livestatus-tables"></a> Livestatus Tables

  Table         | Join      |Description
  --------------|-----------|----------------------------
  hosts         | &nbsp;    | host config and status attributes, services counter
  hostgroups    | &nbsp;    | hostgroup config, status attributes and host/service counters
  services      | hosts     | service config and status attributes
  servicegroups | &nbsp;    | servicegroup config, status attributes and service counters
  contacts      | &nbsp;    | contact config and status attributes
  contactgroups | &nbsp;    | contact config, members
  commands      | &nbsp;    | command name and line
  status        | &nbsp;    | programstatus, config and stats
  comments      | services  | status attributes
  downtimes     | services  | status attributes
  timeperiods   | &nbsp;    | name and is inside flag
  endpoints     | &nbsp;    | config and status attributes
  log           | services, hosts, contacts, commands | parses [compatlog](#objecttype-compatlogger) and shows log attributes
  statehist     | hosts, services | parses [compatlog](#objecttype-compatlogger) and aggregates state change attributes

The `commands` table is populated with `CheckCommand`, `EventCommand` and `NotificationCommand` objects.


#### <a id="schema-livestatus-table-attributes"></a> Livestatus Table Attributes

A detailed list which table attributes are supported can be found here: [https://wiki.icinga.org/display/icinga2/Livestatus#Livestatus-Attributes]


#### <a id="schema-livestatus-get-queries"></a> Livestatus GET Queries

    $ echo -e 'GET services' | netcat 127.0.0.1 6558

    $ cat servicegroups <<EOF
    GET servicegroups

    EOF

    (cat servicegroups; sleep 1) | netcat 127.0.0.1 6558

#### <a id="schema-livestatus-command-queries"></a> Livestatus COMMAND Queries

A list of available external commands and their parameters can be found [here](#external-commands-list-detail)

    $ echo -e 'COMMAND <externalcommandstring>' | netcat 127.0.0.1 6558


#### <a id="schema-livestatus-filters"></a> Livestatus Filters

and, or, negate

  Operator  | Negate   | Description
  ----------|------------------------
   =        | !=       | Euqality
   ~        | !~       | Regex match
   =~       | !=~      | Euqality ignoring case
   ~~       | !~~      | Regex ignoring case
   >        |          | Less than
   <        |          | Greater than
   >=       |          | Less than or equal
   <=       |          | Greater than or equal


#### <a id="schema-livestatus-stats"></a> Livestatus Stats

Schema: "Stats: aggregatefunction aggregateattribute"

  Aggregate Function | Description
  -------------------|--------------
  sum                | &nbsp;
  min                | &nbsp;
  max                | &nbsp;
  avg                | sum / count
  std                | standard deviation
  suminv             | sum (1 / value)
  avginv             | suminv / count
  count              | ordinary default for any stats query if not aggregate function defined

Example:

    GET hosts
    Filter: has_been_checked = 1
    Filter: check_type = 0
    Stats: sum execution_time
    Stats: sum latency
    Stats: sum percent_state_change
    Stats: min execution_time
    Stats: min latency
    Stats: min percent_state_change
    Stats: max execution_time
    Stats: max latency
    Stats: max percent_state_change
    OutputFormat: json
    ResponseHeader: fixed16

#### <a id="schema-livestatus-output"></a> Livestatus Output

* CSV

CSV Output uses two levels of array seperators: The members array separator
is a comma (1st level) while extra info and host|service relation separator
is a pipe (2nd level).

Seperators can be set using ASCII codes like:

    Separators: 10 59 44 124

* JSON

Default separators.

#### <a id="schema-livestatus-error-codes"></a> Livestatus Error Codes

  Code      | Description
  ----------|--------------
  200       | OK
  404       | Table does not exist
  452       | Exception on query

#### <a id="schema-livestatus-extensions"></a> Livestatus Schema Extensions

Icinga 2 specific extensions are shown below:

New table: `endpoints`

  Table     | Column
  ----------|--------------
  endpoints | name
  endpoints | identity
  endpoints | node
  endpoints | is_connected

New columns:

  Table     | Column
  ----------|--------------
  hosts     | is_reachable
  services  | is_reachable
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

Command custom variables reflect the local 'vars' dictionary.
Status custom variables reflect the global 'Vars' constant.


## <a id="external-commands-list-detail"></a> External Commands List

Additional details can be found in the [Icinga 1.x Documentation](http://docs.icinga.org/latest/en/extcommands2.html)

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
  SCHEDULE_HOST_DOWNTIME                    | ;&lt;host_name&gt;;&lt;start_time&gt;;&lt;end_time&gt;;&lt;fixed&gt;;&lt;trigger_id&gt;;&lt;duration&gt;;&lt;author&gt;;&lt;comment&gt; (8)  | -
  DEL_HOST_DOWNTIME                         | ;&lt;downtime_id&gt; (1)  | -
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
  CHANGE_SVC_MODATTR                        | ;&lt;host_name&gt;;&lt;service_name&gt;;&lt;value&gt; (3)  | -
  CHANGE_HOST_MODATTR                       | ;&lt;host_name&gt;;&lt;value&gt; (2)  | -
  CHANGE_USER_MODATTR                       | ;&lt;user_name&gt;;&lt;value&gt; (2)  | -
  CHANGE_CHECKCOMMAND_MODATTR               | ;&lt;checkcommand_name&gt;;&lt;value&gt; (2)  | -
  CHANGE_EVENTCOMMAND_MODATTR               | ;&lt;eventcommand_name&gt;;&lt;value&gt; (2)  | -
  CHANGE_NOTIFICATIONCOMMAND_MODATTR        | ;&lt;notificationcommand_name&gt;;&lt;value&gt; (2)  | -
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
