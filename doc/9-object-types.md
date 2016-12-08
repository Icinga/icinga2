# <a id="object-types"></a> Config Object Types

This chapter provides an overview of all available config object types which can be
instantiated using the `object` keyword.

Additional details on configuration and runtime attributes and their
description are explained as well.

Config objects share these runtime attributes which cannot be
modified by the user. You can access these attributes using
the [Icinga 2 API](12-icinga2-api.md#icinga2-api-config-objects).

  Name                      |Description
  --------------------------|--------------------------
  version                   | Timestamp when the object was created or modified. Synced throughout cluster nodes.
  type                      | Object type.
  original_attributes       | Original values of object attributes modified at runtime.
  active                    | Object is active (e.g. a service being checked).
  paused                    | Object has been paused at runtime (e.g. [IdoMysqlConnection](9-object-types.md#objecttype-idomysqlconnection). Defaults to `false`.
  templates                 | Templates imported on object compilation.
  package                   | [Configuration package name](12-icinga2-api.md#icinga2-api-config-management) this object belongs to. Local configuration is set to `_etc`, runtime created objects use `_api`.


## <a id="objecttype-apilistener"></a> ApiListener

ApiListener objects are used for distributed monitoring setups
and API usage specifying the certificate files used for ssl
authorization and additional restrictions.

The `NodeName` constant must be defined in [constants.conf](4-configuring-icinga-2.md#constants-conf).

Example:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
    }


Configuration Attributes:

  Name                      |Description
  --------------------------|--------------------------
  cert\_path                |**Required.** Path to the public key.
  key\_path                 |**Required.** Path to the private key.
  ca\_path                  |**Required.** Path to the CA certificate file.
  crl\_path                 |**Optional.** Path to the CRL file.
  bind\_host                |**Optional.** The IP address the api listener should be bound to. Defaults to `0.0.0.0`.
  bind\_port                |**Optional.** The port the api listener should be bound to. Defaults to `5665`.
  accept\_config            |**Optional.** Accept zone configuration. Defaults to `false`.
  accept\_commands          |**Optional.** Accept remote commands. Defaults to `false`.
  cipher\_list		    |**Optional.** Cipher list that is allowed.
  tls\_protocolmin          |**Optional.** Minimum TLS protocol version. Must be one of `TLSv1`, `TLSv1.1` or `TLSv1.2`. Defaults to `TLSv1`.

## <a id="objecttype-apiuser"></a> ApiUser

ApiUser objects are used for authentication against the Icinga 2 API.

Example:

    object ApiUser "root" {
      password = "mysecretapipassword"
      permissions = [ "*" ]
    }


Configuration Attributes:

  Name                      |Description
  --------------------------|--------------------------
  password                  |**Optional.** Password string.
  client\_cn                |**Optional.** Client Common Name (CN).
  permissions		    |**Required.** Array of permissions. Either as string or dictionary with the keys `permission` and `filter`. The latter must be specified as function.

Available permissions are described in the [API permissions](12-icinga2-api.md#icinga2-api-permissions)
chapter.

## <a id="objecttype-checkcommand"></a> CheckCommand

A check command definition. Additional default command custom attributes can be
defined here.

> **Note**
>
> Icinga 2 versions < 2.6.0 require the import of the [plugin-check-command](10-icinga-template-library.md#itl-plugin-check-command) template.

Example:

    object CheckCommand "check_http" {
      command = [ PluginDir + "/check_http" ]

      arguments = {
        "-H" = "$http_vhost$"
        "-I" = "$http_address$"
        "-u" = "$http_uri$"
        "-p" = "$http_port$"
        "-S" = {
          set_if = "$http_ssl$"
        }
        "--sni" = {
          set_if = "$http_sni$"
        }
        "-a" = {
          value = "$http_auth_pair$"
          description = "Username:password on sites with basic authentication"
        }
        "--no-body" = {
          set_if = "$http_ignore_body$"
        }
        "-r" = "$http_expect_body_regex$"
        "-w" = "$http_warn_time$"
        "-c" = "$http_critical_time$"
        "-e" = "$http_expect$"
      }

      vars.http_address = "$address$"
      vars.http_ssl = false
      vars.http_sni = false
    }


Configuration Attributes:

  Name            |Description
  ----------------|----------------
  execute         |**Required.** The "execute" script method takes care of executing the check. The default template "plugin-check-command" which is imported into all CheckCommand objects takes care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command. When using the "arguments" attribute this must be an array. Can be specified as function for advanced implementations.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 60 seconds.
  arguments       |**Optional.** A dictionary of command arguments.


### <a id="objecttype-checkcommand-arguments"></a> CheckCommand Arguments

Command arguments can be defined as key-value-pairs in the `arguments`
dictionary. If the argument requires additional configuration, for example
a `description` attribute or an optional condition, the value can be defined
as dictionary specifying additional options.

Service:

    vars.x_val = "My command argument value."
    vars.have_x = "true"

CheckCommand:

    arguments = {
      "-X" = {
        value = "$x_val$"
        key = "-Xnew"	    /* optional, set a new key identifier */
        description = "My plugin requires this argument for doing X."
        required = false    /* optional, no error if not set */
        skip_key = false    /* always use "-X <value>" */
        set_if = "$have_x$" /* only set if variable defined and resolves to a numeric value. String values are not supported */
        order = -1          /* first position */
        repeat_key = true   /* if `value` is an array, repeat the key as parameter: ... 'key' 'value[0]' 'key' 'value[1]' 'key' 'value[2]' ... */
      }
      "-Y" = {
        value = "$y_val$"
        description = "My plugin requires this argument for doing Y."
        required = false    /* optional, no error if not set */
        skip_key = true     /* don't prefix "-Y" only use "<value>" */
        set_if = "$have_y$" /* only set if variable defined and resolves to a numeric value. String values are not supported */
        order = 0           /* second position */
        repeat_key = false  /* if `value` is an array, do not repeat the key as parameter: ... 'key' 'value[0]' 'value[1]' 'value[2]' ... */
      }
    }

  Option      | Description
  ------------|--------------
  value       | Optional argument value set by a macro string or a function call.
  key 	      | Optional argument key overriding the key identifier.
  description | Optional argument description.
  required    | Required argument. Execution error if not set. Defaults to false (optional).
  skip_key    | Use the value as argument and skip the key.
  set_if      | Argument is added if the macro resolves to a defined numeric or boolean value. String values are not supported. Function calls returning a value are supported too.
  order       | Set if multiple arguments require a defined argument order.
  repeat_key  | If the argument value is an array, repeat the argument key, or not. Defaults to true (repeat).

Argument order:

    `..., -3, -2, -1, <un-ordered keys>, 1, 2, 3, ...`

Argument array `repeat_key = true`:

    `'key' 'value[0]' 'key' 'value[1]' 'key' 'value[2]'`

Argument array `repeat_key = false`:

    `'key' 'value[0]' 'value[1]' 'value[2]'`

## <a id="objecttype-checkcomponent"></a> CheckerComponent

The checker component is responsible for scheduling active checks.

Example:

    library "checker"

    object CheckerComponent "checker" {
      concurrent_checks = 512
    }

Configuration Attributes:

  Name                |Description
  --------------------|----------------
  concurrent\_checks  |**Optional.** The maximum number of concurrent checks. Defaults to 512.

## <a id="objecttype-checkresultreader"></a> CheckResultReader

Reads Icinga 1.x check results from a directory. This functionality is provided
to help existing Icinga 1.x users and might be useful for certain cluster
scenarios.

Example:

    library "compat"

    object CheckResultReader "reader" {
      spool_dir = "/data/check-results"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  spool\_dir      |**Optional.** The directory which contains the check result files. Defaults to LocalStateDir + "/lib/icinga2/spool/checkresults/".

## <a id="objecttype-comment"></a> Comment

Comments created at runtime are represented as objects.

Example:

    object Comment "localhost!my-comment" {
      host_name = "localhost"
      author = "icingaadmin"
      text = "This is a comment."
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host_name       | **Required.** The name of the host this comment belongs to.
  service_name    | **Optional.** The short name of the service this comment belongs to. If omitted, this comment object is treated as host comment.
  author          | **Required.** The author's name.
  text            | **Required.** The comment text.
  entry_time      | **Optional.** The unix timestamp when this comment was added.
  entry_type      | **Optional.** The comment type (`User` = 1, `Downtime` = 2, `Flapping` = 3, `Acknowledgement` = 4).
  expire_time     | **Optional.** The comment's expire time as unix timestamp.

## <a id="objecttype-compatlogger"></a> CompatLogger

Writes log files in a format that's compatible with Icinga 1.x.

Example:

    library "compat"

    object CompatLogger "my-log" {
      log_dir = "/var/log/icinga2/compat"
      rotation_method = "HOURLY"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  log\_dir        |**Optional.** Path to the compat log directory. Defaults to LocalStateDir + "/log/icinga2/compat".
  rotation\_method|**Optional.** Specifies when to rotate log files. Can be one of "HOURLY", "DAILY", "WEEKLY" or "MONTHLY". Defaults to "HOURLY".



## <a id="objecttype-dependency"></a> Dependency

Dependency objects are used to specify dependencies between hosts and services. Dependencies
can be defined as Host-to-Host, Service-to-Service, Service-to-Host, or Host-to-Service
relations.

> **Best Practice**
>
> Rather than creating a `Dependency` object for a specific host or service it is usually easier
> to just create a `Dependency` template and use the `apply` keyword to assign the
> dependency to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [dependencies](3-monitoring-basics.md#dependencies) chapter for detailed examples.

Service-to-Service Example:

    object Dependency "webserver-internet" {
      parent_host_name = "internet"
      parent_service_name = "ping4"

      child_host_name = "webserver"
      child_service_name = "ping4"

      states = [ OK, Warning ]

      disable_checks = true
    }

Host-to-Host Example:

    object Dependency "webserver-internet" {
      parent_host_name = "internet"

      child_host_name = "webserver"

      states = [ Up ]

      disable_checks = true
    }

Configuration Attributes:

  Name                  |Description
  ----------------------|----------------
  parent_host_name      |**Required.** The parent host.
  parent_service_name   |**Optional.** The parent service. If omitted, this dependency object is treated as host dependency.
  child_host_name       |**Required.** The child host.
  child_service_name    |**Optional.** The child service. If omitted, this dependency object is treated as host dependency.
  disable_checks        |**Optional.** Whether to disable checks when this dependency fails. Defaults to false.
  disable_notifications |**Optional.** Whether to disable notifications when this dependency fails. Defaults to true.
  ignore_soft_states    |**Optional.** Whether to ignore soft states for the reachability calculation. Defaults to true.
  period                |**Optional.** Time period during which this dependency is enabled.
  states    	        |**Optional.** A list of state filters when this dependency should be OK. Defaults to [ OK, Warning ] for services and [ Up ] for hosts.

Available state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

When using [apply rules](3-monitoring-basics.md#using-apply) for dependencies, you can leave out certain attributes which will be
automatically determined by Icinga 2.

Service-to-Host Dependency Example:

    apply Dependency "internet" to Service {
      parent_host_name = "dsl-router"
      disable_checks = true

      assign where host.name != "dsl-router"
    }

This example sets all service objects matching the assign condition into a dependency relation to
the parent host object `dsl-router` as implicit child services.

Service-to-Service-on-the-same-Host Dependency Example:

    apply Dependency "disable-nrpe-checks" to Service {
      parent_service_name = "nrpe-health"

      assign where service.check_command == "nrpe"
      ignore where service.name == "nrpe-health"
    }

This example omits the `parent_host_name` attribute and Icinga 2 automatically sets its value to the name of the
host object matched by the apply rule condition. All services where apply matches are made implicit child services
in this dependency relation.


Dependency objects have composite names, i.e. their names are based on the `child_host_name` and `child_service_name` attributes and the
name you specified. This means you can define more than one object with the same (short) name as long as one of the `child_host_name` and
`child_service_name` attributes has a different value.

## <a id="objecttype-downtime"></a> Downtime

Downtimes created at runtime are represented as objects.

Example:

    object Downtime "localhost!my-downtime" {
      host_name = "localhost"
      author = "icingaadmin"
      text = "This is a comment."
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host_name       | **Required.** The name of the host this comment belongs to.
  service_name    | **Optional.** The short name of the service this comment belongs to. If omitted, this comment object is treated as host comment.
  author          | **Required.** The author's name.
  comment         | **Required.** The comment text.
  start_time      | **Required.** The start time as unix timestamp.
  end_time        | **Required.** The end time as unix timestamp.
  duration        | **Required.** The duration as number.
  entry_time      | **Optional.** The unix timestamp when this downtime was added.
  fixed           | **Optional.** Whether the downtime is fixed (true) or flexible (false). Defaults to flexible. Details in the [advanced topics chapter](8-advanced-topics.md#fixed-flexible-downtimes).
  triggers        | **Optional.** List of downtimes which should be triggered by this downtime.

Runtime Attributes:

  Name            |Description
  ----------------|----------------
  trigger_time    | The unix timestamp when this downtime was triggered.
  triggered_by    | The name of the downtime this downtime was triggered by.



## <a id="objecttype-endpoint"></a> Endpoint

Endpoint objects are used to specify connection information for remote
Icinga 2 instances.

Example:

    object Endpoint "icinga2b" {
      host = "192.168.5.46"
      port = 5665
      log_duration = 1d
    }

Example (disable replay log):

    object Endpoint "icinga2b" {
      host = "192.168.5.46"
      port = 5665
      log_duration = 0
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** The hostname/IP address of the remote Icinga 2 instance.
  port            |**Optional.** The service name/port of the remote Icinga 2 instance. Defaults to `5665`.
  log_duration    |**Optional.** Duration for keeping replay logs on connection loss. Defaults to `1d` (86400 seconds). Attribute is specified in seconds. If log_duration is set to 0, replaying logs is disabled. You could also specify the value in human readable format like `10m` for 10 minutes or `1h` for one hour.

Endpoint objects cannot currently be created with the API.

## <a id="objecttype-eventcommand"></a> EventCommand

An event command definition.

> **Note**
>
> Icinga 2 versions < 2.6.0 require the import of the [plugin-event-command](10-icinga-template-library.md#itl-plugin-event-command) template.

Example:

    object EventCommand "restart-httpd-event" {
      command = "/opt/bin/restart-httpd.sh"
    }


Configuration Attributes:

  Name            |Description
  ----------------|----------------
  execute         |**Required.** The "execute" script method takes care of executing the event handler. The default template "plugin-event-command" which is imported into all CheckCommand objects takes care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 60 seconds.
  arguments       |**Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for [CheckCommand objects](9-object-types.md#objecttype-checkcommand-arguments).


## <a id="objecttype-externalcommandlistener"></a> ExternalCommandListener

Implements the Icinga 1.x command pipe which can be used to send commands to Icinga.

Example:

    library "compat"

    object ExternalCommandListener "external" {
        command_path = "/var/run/icinga2/cmd/icinga2.cmd"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  command\_path   |**Optional.** Path to the command pipe. Defaults to RunDir + "/icinga2/cmd/icinga2.cmd".



## <a id="objecttype-filelogger"></a> FileLogger

Specifies Icinga 2 logging to a file.

Example:

    object FileLogger "debug-file" {
      severity = "debug"
      path = "/var/log/icinga2/debug.log"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  path            |**Required.** The log path.
  severity        |**Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "warning" or "critical". Defaults to "information".


## <a id="objecttype-gelfwriter"></a> GelfWriter

Writes event log entries to a defined GELF receiver host (Graylog2, Logstash).

Example:

    library "perfdata"

    object GelfWriter "gelf" {
      host = "127.0.0.1"
      port = 12201
    }

Configuration Attributes:

  Name            	|Description
  ----------------------|----------------------
  host            	|**Optional.** GELF receiver host address. Defaults to '127.0.0.1'.
  port            	|**Optional.** GELF receiver port. Defaults to `12201`.
  source		|**Optional.** Source name for this instance. Defaults to `icinga2`.
  enable_send_perfdata  |**Optional.** Enable performance data for 'CHECK RESULT' events.


## <a id="objecttype-graphitewriter"></a> GraphiteWriter

Writes check result metrics and performance data to a defined
Graphite Carbon host.

Example:

    library "perfdata"

    object GraphiteWriter "graphite" {
      host = "127.0.0.1"
      port = 2003
    }

Configuration Attributes:

  Name            	|Description
  ----------------------|----------------------
  host            	|**Optional.** Graphite Carbon host address. Defaults to '127.0.0.1'.
  port            	|**Optional.** Graphite Carbon port. Defaults to 2003.
  host_name_template 	|**Optional.** Metric prefix for host name. Defaults to "icinga2.$host.name$.host.$host.check_command$".
  service_name_template |**Optional.** Metric prefix for service name. Defaults to "icinga2.$host.name$.services.$service.name$.$service.check_command$".
  enable_send_thresholds | **Optional.** Send additional threshold metrics. Defaults to `false`.
  enable_send_metadata 	| **Optional.** Send additional metadata metrics. Defaults to `false`.
  enable_legacy_mode	| **Optional.** Enable legacy mode for schema < 2.4. **Note**: This will be removed in future versions.

Additional usage examples can be found [here](14-features.md#graphite-carbon-cache-writer).



## <a id="objecttype-host"></a> Host

A host.

Example:

    object Host NodeName {
      display_name = "Local host on this node"
      address = "127.0.0.1"
      address6 = "::1"

      groups = [ "all-hosts" ]

      check_command = "hostalive"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the host (e.g. displayed by external interfaces instead of the name if set).
  address         |**Optional.** The host's address. Available as command runtime macro `$address$` if set.
  address6        |**Optional.** The host's address. Available as command runtime macro `$address6$` if set.
  groups          |**Optional.** A list of host groups this host belongs to.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this host.
  check\_command  |**Required.** The name of the check command.
  max\_check\_attempts|**Optional.** The number of times a host is re-checked before changing into a hard state. Defaults to 3.
  check\_period   |**Optional.** The name of a time period which determines when this host should be checked. Not set by default.
  check\_timeout  |**Optional.** Check command timeout in seconds. Overrides the CheckCommand's `timeout` attribute.
  check\_interval |**Optional.** The check interval (in seconds). This interval is used for checks when the host is in a `HARD` state. Defaults to 5 minutes.
  retry\_interval |**Optional.** The retry interval (in seconds). This interval is used for checks when the host is in a `SOFT` state. Defaults to 1 minute.
  enable\_notifications|**Optional.** Whether notifications are enabled. Defaults to true.
  enable\_active\_checks|**Optional.** Whether active checks are enabled. Defaults to true.
  enable\_passive\_checks|**Optional.** Whether passive checks are enabled. Defaults to true.
  enable\_event\_handler|**Optional.** Enables event handlers for this host. Defaults to true.
  enable\_flapping|**Optional.** Whether flap detection is enabled. Defaults to false.
  enable\_perfdata|**Optional.** Whether performance data processing is enabled. Defaults to true.
  event\_command  |**Optional.** The name of an event command that should be executed every time the host's state changes or the host is in a `SOFT` state.
  flapping\_threshold|**Optional.** The flapping threshold in percent when a host is considered to be flapping.
  volatile        |**Optional.** The volatile setting enables always `HARD` state types if `NOT-OK` state changes occur.
  zone		  |**Optional.** The zone this object is a member of.
  command\_endpoint|**Optional.** The endpoint where commands are executed on.
  notes           |**Optional.** Notes for the host.
  notes\_url      |**Optional.** Url for notes for the host (for example, in notification commands).
  action\_url     |**Optional.** Url for actions for the host (for example, an external graphing tool).
  icon\_image     |**Optional.** Icon image for the host. Used by external interfaces only.
  icon\_image\_alt|**Optional.** Icon image description for the host. Used by external interface only.

The actual check interval might deviate slightly from the configured values due to the fact that Icinga tries
to evenly distribute all checks over a certain period of time, i.e. to avoid load spikes.

> **Best Practice**
>
> The `address` and `address6` attributes are required for running commands using
> the `$address$` and `$address6$` runtime macros.

Runtime Attributes:

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  next\_check               | Number        | When the next check occurs (as a UNIX timestamp).
  last\_check               | Number        | When the last check occured (as a UNIX timestamp).
  check\_attempt            | Number        | The current check attempt number.
  state\_type               | Number        | The current state type (0 = SOFT, 1 = HARD).
  last\_state\_type         | Number        | The previous state type (0 = SOFT, 1 = HARD).
  last\_reachable           | Boolean       | Whether the host was reachable when the last check occurred.
  last\_check\_result       | CheckResult   | The current check result.
  last\_state\_change       | Number        | When the last state change occurred (as a UNIX timestamp).
  last\_hard\_state\_change | Number        | When the last hard state change occurred (as a UNIX timestamp).
  last\_in\_downtime        | Boolean       | Whether the host was in a downtime when the last check occurred.
  acknowledgement           | Number        | The acknowledgement type (0 = NONE, 1 = NORMAL, 2 = STICKY).
  acknowledgement_expiry    | Number        | When the acknowledgement expires (as a UNIX timestamp; 0 = no expiry).
  downtime\_depth           | Number        | Whether the host has one or more active downtimes.
  flapping_last_change      | Number        | When the last flapping change occurred (as a UNIX timestamp).
  flapping                  | Boolean       | Whether the host is flapping between states.
  state                     | Number        | The current state (0 = UP, 1 = DOWN).
  last\_state               | Number        | The previous state (0 = UP, 1 = DOWN).
  last\_hard\_state         | Number        | The last hard state (0 = UP, 1 = DOWN).
  last_state_up             | Number        | When the last UP state occurred (as a UNIX timestamp).
  last_state_down           | Number        | When the last DOWN state occurred (as a UNIX timestamp).



## <a id="objecttype-hostgroup"></a> HostGroup

A group of hosts.

> **Best Practice**
>
> Assign host group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

    object HostGroup "my-hosts" {
      display_name = "My hosts"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the host group.
  groups          |**Optional.** An array of nested group names.

## <a id="objecttype-icingaapplication"></a> IcingaApplication

The IcingaApplication object is required to start Icinga 2.
The object name must be `app`. If the object configuration
is missing, Icinga 2 will automatically create an IcingaApplication
object.

Example:

    object IcingaApplication "app" {
      enable_perfdata = false
    }

Configuration Attributes:

  Name                  |Description
  ----------------------|--------------------------
  enable_notifications  |**Optional.** Whether notifications are globally enabled. Defaults to true.
  enable_event_handlers |**Optional.** Whether event handlers are globally enabled. Defaults to true.
  enable_flapping       |**Optional.** Whether flap detection is globally enabled. Defaults to true.
  enable_host_checks    |**Optional.** Whether active host checks are globally enabled. Defaults to true.
  enable_service_checks |**Optional.** Whether active service checks are globally enabled. Defaults to true.
  enable_perfdata       |**Optional.** Whether performance data processing is globally enabled. Defaults to true.
  vars                  |**Optional.** A dictionary containing custom attributes that are available globally.

## <a id="objecttype-idomysqlconnection"></a> IdoMySqlConnection

IDO database adapter for MySQL.

Example:

    library "db_ido_mysql"

    object IdoMysqlConnection "mysql-ido" {
      host = "127.0.0.1"
      port = 3306
      user = "icinga"
      password = "icinga"
      database = "icinga"

      cleanup = {
        downtimehistory_age = 48h
        contactnotifications_age = 31d
      }
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** MySQL database host address. Defaults to "localhost".
  port            |**Optional.** MySQL database port. Defaults to 3306.
  socket_path     |**Optional.** MySQL socket path.
  user            |**Optional.** MySQL database user with read/write permission to the icinga database. Defaults to "icinga".
  password        |**Optional.** MySQL database user's password. Defaults to "icinga".
  database        |**Optional.** MySQL database name. Defaults to "icinga".
  enable\_ssl     |**Optional.** Use SSL. Defaults to false. Change to `true` in case you want to use any of the SSL options.
  ssl\_key        |**Optional.** MySQL SSL client key file path.
  ssl\_cert       |**Optional.** MySQL SSL certificate file path.
  ssl\_ca         |**Optional.** MySQL SSL certificate authority certificate file path.
  ssl\_capath     |**Optional.** MySQL SSL trusted SSL CA certificates in PEM format directory path.
  ssl\_cipher     |**Optional.** MySQL SSL list of allowed ciphers.
  table\_prefix   |**Optional.** MySQL database table prefix. Defaults to "icinga\_".
  instance\_name  |**Optional.** Unique identifier for the local Icinga 2 instance. Defaults to "default".
  instance\_description|**Optional.** Description for the Icinga 2 instance.
  enable_ha       |**Optional.** Enable the high availability functionality. Only valid in a [cluster setup](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Defaults to "true".
  failover_timeout | **Optional.** Set the failover timeout in a [HA cluster](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Must not be lower than 60s. Defaults to "60s".
  cleanup         |**Optional.** Dictionary with items for historical table cleanup.
  categories      |**Optional.** Array of information types that should be written to the database.

Cleanup Items:

  Name            | Description
  ----------------|----------------
  acknowledgements_age |**Optional.** Max age for acknowledgements table rows (entry_time). Defaults to 0 (never).
  commenthistory_age |**Optional.** Max age for commenthistory table rows (entry_time). Defaults to 0 (never).
  contactnotifications_age |**Optional.** Max age for contactnotifications table rows (start_time). Defaults to 0 (never).
  contactnotificationmethods_age |**Optional.** Max age for contactnotificationmethods table rows (start_time). Defaults to 0 (never).
  downtimehistory_age |**Optional.** Max age for downtimehistory table rows (entry_time). Defaults to 0 (never).
  eventhandlers_age |**Optional.** Max age for eventhandlers table rows (start_time). Defaults to 0 (never).
  externalcommands_age |**Optional.** Max age for externalcommands table rows (entry_time). Defaults to 0 (never).
  flappinghistory_age |**Optional.** Max age for flappinghistory table rows (event_time). Defaults to 0 (never).
  hostchecks_age |**Optional.** Max age for hostalives table rows (start_time). Defaults to 0 (never).
  logentries_age |**Optional.** Max age for logentries table rows (logentry_time). Defaults to 0 (never).
  notifications_age |**Optional.** Max age for notifications table rows (start_time). Defaults to 0 (never).
  processevents_age |**Optional.** Max age for processevents table rows (event_time). Defaults to 0 (never).
  statehistory_age |**Optional.** Max age for statehistory table rows (state_time). Defaults to 0 (never).
  servicechecks_age |**Optional.** Max age for servicechecks table rows (start_time). Defaults to 0 (never).
  systemcommands_age |**Optional.** Max age for systemcommands table rows (start_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web 2
  DbCatState           | Current state data     | Icinga Web 2
  DbCatAcknowledgement | Acknowledgements       | Icinga Web 2
  DbCatComment         | Comments               | Icinga Web 2
  DbCatDowntime        | Downtimes              | Icinga Web 2
  DbCatEventHandler    | Event handler data     | Icinga Web 2
  DbCatExternalCommand | External commands      | Icinga Web 2
  DbCatFlapping        | Flap detection data    | Icinga Web 2
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | --
  DbCatNotification    | Notifications          | Icinga Web 2
  DbCatProgramStatus   | Program status data    | Icinga Web 2
  DbCatRetention       | Retention data         | Icinga Web 2
  DbCatStateHistory    | Historical state data  | Icinga Web 2

In addition to the category flags listed above the `DbCatEverything`
flag may be used as a shortcut for listing all flags.

> **Note**
>
> The previous way of defining the `categories` attribute e.g.
> `DbCatProgramStatus | DbCatState` was deprecated in 2.5 and will
> be removed in future versions.

External interfaces like Icinga Web 2 require everything except `DbCatCheck`
and `DbCatLog` which is the default value if `categories` is not set.

## <a id="objecttype-idopgsqlconnection"></a> IdoPgSqlConnection

IDO database adapter for PostgreSQL.

Example:

    library "db_ido_pgsql"

    object IdoPgsqlConnection "pgsql-ido" {
      host = "127.0.0.1"
      port = 5432
      user = "icinga"
      password = "icinga"
      database = "icinga"

      cleanup = {
        downtimehistory_age = 48h
        contactnotifications_age = 31d
      }
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host            |**Optional.** PostgreSQL database host address. Defaults to "localhost".
  port            |**Optional.** PostgreSQL database port. Defaults to "5432".
  user            |**Optional.** PostgreSQL database user with read/write permission to the icinga database. Defaults to "icinga".
  password        |**Optional.** PostgreSQL database user's password. Defaults to "icinga".
  database        |**Optional.** PostgreSQL database name. Defaults to "icinga".
  table\_prefix   |**Optional.** PostgreSQL database table prefix. Defaults to "icinga\_".
  instance\_name  |**Optional.** Unique identifier for the local Icinga 2 instance. Defaults to "default".
  instance\_description|**Optional.** Description for the Icinga 2 instance.
  enable_ha       |**Optional.** Enable the high availability functionality. Only valid in a [cluster setup](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Defaults to "true".
  failover_timeout | **Optional.** Set the failover timeout in a [HA cluster](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Must not be lower than 60s. Defaults to "60s".
  cleanup         |**Optional.** Dictionary with items for historical table cleanup.
  categories      |**Optional.** Array of information types that should be written to the database.

Cleanup Items:

  Name            | Description
  ----------------|----------------
  acknowledgements_age |**Optional.** Max age for acknowledgements table rows (entry_time). Defaults to 0 (never).
  commenthistory_age |**Optional.** Max age for commenthistory table rows (entry_time). Defaults to 0 (never).
  contactnotifications_age |**Optional.** Max age for contactnotifications table rows (start_time). Defaults to 0 (never).
  contactnotificationmethods_age |**Optional.** Max age for contactnotificationmethods table rows (start_time). Defaults to 0 (never).
  downtimehistory_age |**Optional.** Max age for downtimehistory table rows (entry_time). Defaults to 0 (never).
  eventhandlers_age |**Optional.** Max age for eventhandlers table rows (start_time). Defaults to 0 (never).
  externalcommands_age |**Optional.** Max age for externalcommands table rows (entry_time). Defaults to 0 (never).
  flappinghistory_age |**Optional.** Max age for flappinghistory table rows (event_time). Defaults to 0 (never).
  hostchecks_age |**Optional.** Max age for hostalives table rows (start_time). Defaults to 0 (never).
  logentries_age |**Optional.** Max age for logentries table rows (logentry_time). Defaults to 0 (never).
  notifications_age |**Optional.** Max age for notifications table rows (start_time). Defaults to 0 (never).
  processevents_age |**Optional.** Max age for processevents table rows (event_time). Defaults to 0 (never).
  statehistory_age |**Optional.** Max age for statehistory table rows (state_time). Defaults to 0 (never).
  servicechecks_age |**Optional.** Max age for servicechecks table rows (start_time). Defaults to 0 (never).
  systemcommands_age |**Optional.** Max age for systemcommands table rows (start_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web 2
  DbCatState           | Current state data     | Icinga Web 2
  DbCatAcknowledgement | Acknowledgements       | Icinga Web 2
  DbCatComment         | Comments               | Icinga Web 2
  DbCatDowntime        | Downtimes              | Icinga Web 2
  DbCatEventHandler    | Event handler data     | Icinga Web 2
  DbCatExternalCommand | External commands      | Icinga Web 2
  DbCatFlapping        | Flap detection data    | Icinga Web 2
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | --
  DbCatNotification    | Notifications          | Icinga Web 2
  DbCatProgramStatus   | Program status data    | Icinga Web 2
  DbCatRetention       | Retention data         | Icinga Web 2
  DbCatStateHistory    | Historical state data  | Icinga Web 2

In addition to the category flags listed above the `DbCatEverything`
flag may be used as a shortcut for listing all flags.

> **Note**
>
> The previous way of defining the `categories` attribute e.g.
> `DbCatProgramStatus | DbCatState` was deprecated in 2.5 and will
> be removed in future versions.

External interfaces like Icinga Web 2 require everything except `DbCatCheck`
and `DbCatLog` which is the default value if `categories` is not set.


## <a id="objecttype-influxdbwriter"></a> InfluxdbWriter

Writes check result metrics and performance data to a defined InfluxDB host.

Example:

    library "perfdata"

    object InfluxdbWriter "influxdb" {
      host = "127.0.0.1"
      port = 8086
      database = "icinga2"
      host_template = {
        measurement = "$host.check_command$"
        tags = {
          hostname = "$host.name$"
        }
      }
      service_template = {
        measurement = "$service.check_command$"
        tags = {
          hostname = "$host.name$"
          service = "$service.name$"
        }
      }
    }

Measurement names and tags are fully configurable by the end user. The InfluxdbWriter
object will automatically add a `metric` tag to each data point. This correlates to the
perfdata label. Fields (value, warn, crit, min, max) are created from data if available
and the configuration allows it.  If a value associated with a tag is not able to be
resolved, it will be dropped and not sent to the target host.

Backslashes are allowed in tag keys, tag values and field keys, however they are also
escape characters when followed by a space or comma, but cannot be escaped themselves.
As a result all trailling slashes in these fields are replaced with an underscore.  This
predominantly affects Windows paths e.g. `C:\` becomes `C:_`.

The database is assumed to exist so this object will make no attempt to create it currently.

Configuration Attributes:

  Name                   |Description
  -----------------------|---------------------------------------------------------------------------------------------------------
  host                   | **Required.** InfluxDB host address. Defaults to `127.0.0.1`.
  port                   | **Required.** InfluxDB HTTP port. Defaults to `8086`.
  database               | **Required.** InfluxDB database name. Defaults to `icinga2`.
  username               | **Optional.** InfluxDB user name. Defaults to `none`.
  password               | **Optional.** InfluxDB user password.  Defaults to `none`.
  ssl_enable             | **Optional.** Whether to use a TLS stream.  Defaults to `false`.
  ssl_ca_cert            | **Optional.** CA certificate to validate the remote host.
  ssl_cert               | **Optional.** Host certificate to present to the remote host for mutual verification.
  ssl_key                | **Optional.** Host key to accompany the ssl_cert
  host_template          | **Required.** Host template to define the InfluxDB line protocol.
  service_template       | **Required.** Service template to define the influxDB line protocol.
  enable_send_thresholds | **Optional.** Whether to send warn, crit, min & max tagged data.
  enable_send_metadata   | **Optional.** Whether to send check metadata e.g. states, execution time, latency etc.
  flush_interval         | **Optional.** How long to buffer data points before transfering to InfluxDB. Defaults to `10s`.
  flush_threshold        | **Optional.** How many data points to buffer before forcing a transfer to InfluxDB.  Defaults to `1024`.

### <a id="objecttype-influxdbwriter-instance-tags"></a> Instance Tagging

Consider the following service check:

    apply Service "disk" for (disk => attributes in host.vars.disks) {
      import "generic-service"
      check_command = "disk"
      display_name = "Disk " + disk
      vars.disk_partitions = disk
      assign where host.vars.disks
    }

This is a typical pattern for checking individual disks, NICs, SSL certificates etc associated
with a host.  What would be useful is to have the data points tagged with the specific instance
for that check.  This would allow you to query time series data for a check on a host and for a
specific instance e.g. /dev/sda.  To do this quite simply add the instance to the service variables:

    apply Service "disk" for (disk => attributes in host.vars.disks) {
      ...
      vars.instance = disk
      ...
    }

Then modify your writer configuration to add this tag to your data points if the instance variable
is associated with the service:

    object InfluxdbWriter "influxdb" {
      ...
      service_template = {
        measurement = "$service.check_command$"
        tags = {
          hostname = "$host.name$"
          service = "$service.name$"
          instance = "$service.vars.instance$"
        }
      }
      ...
    }

## <a id="objecttype-livestatuslistener"></a> LiveStatusListener

Livestatus API interface available as TCP or UNIX socket. Historical table queries
require the [CompatLogger](9-object-types.md#objecttype-compatlogger) feature enabled
pointing to the log files using the `compat_log_path` configuration attribute.

Example:

    library "livestatus"

    object LivestatusListener "livestatus-tcp" {
      socket_type = "tcp"
      bind_host = "127.0.0.1"
      bind_port = "6558"
    }

    object LivestatusListener "livestatus-unix" {
      socket_type = "unix"
      socket_path = "/var/run/icinga2/cmd/livestatus"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  socket\_type      |**Optional.** Specifies the socket type. Can be either "tcp" or "unix". Defaults to "unix".
  bind\_host        |**Optional.** Only valid when socket\_type is "tcp". Host address to listen on for connections. Defaults to "127.0.0.1".
  bind\_port        |**Optional.** Only valid when `socket_type` is "tcp". Port to listen on for connections. Defaults to 6558.
  socket\_path      |**Optional.** Only valid when `socket_type` is "unix". Specifies the path to the UNIX socket file. Defaults to RunDir + "/icinga2/cmd/livestatus".
  compat\_log\_path |**Optional.** Required for historical table queries. Requires `CompatLogger` feature enabled. Defaults to LocalStateDir + "/log/icinga2/compat"

> **Note**
>
> UNIX sockets are not supported on Windows.


## <a id="objecttype-notification"></a> Notification

Notification objects are used to specify how users should be notified in case
of host and service state changes and other events.

> **Best Practice**
>
> Rather than creating a `Notification` object for a specific host or service it is
> usually easier to just create a `Notification` template and use the `apply` keyword
> to assign the notification to a number of hosts or services. Use the `to` keyword
> to set the specific target type for `Host` or `Service`.
> Check the [notifications](3-monitoring-basics.md#notifications) chapter for detailed examples.

Example:

    object Notification "localhost-ping-notification" {
      host_name = "localhost"
      service_name = "ping4"

      command = "mail-notification"

      users = [ "user1", "user2" ]

      types = [ Problem, Recovery ]
    }

Configuration Attributes:

  Name                      | Description
  --------------------------|----------------
  host_name                 | **Required.** The name of the host this notification belongs to.
  service_name              | **Optional.** The short name of the service this notification belongs to. If omitted, this notification object is treated as host notification.
  vars                      | **Optional.** A dictionary containing custom attributes that are specific to this notification object.
  users                     | **Optional.** A list of user names who should be notified.
  user_groups               | **Optional.** A list of user group names who should be notified.
  times                     | **Optional.** A dictionary containing `begin` and `end` attributes for the notification.
  command                   | **Required.** The name of the notification command which should be executed when the notification is triggered.
  interval                  | **Optional.** The notification interval (in seconds). This interval is used for active notifications. Defaults to 30 minutes. If set to 0, [re-notifications](3-monitoring-basics.md#disable-renotification) are disabled.
  period                    | **Optional.** The name of a time period which determines when this notification should be triggered. Not set by default.
  zone		            |**Optional.** The zone this object is a member of.
  types                     | **Optional.** A list of type filters when this notification should be triggered. By default everything is matched.
  states                    | **Optional.** A list of state filters when this notification should be triggered. By default everything is matched.

Available notification state filters for Service:

    OK
    Warning
    Critical
    Unknown

Available notification state filters for Host:

    Up
    Down

Available notification type filters:

    DowntimeStart
    DowntimeEnd
    DowntimeRemoved
    Custom
    Acknowledgement
    Problem
    Recovery
    FlappingStart
    FlappingEnd

Runtime Attributes:

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  last\_notification        | Number        | When the last notification was sent for this Notification object (as a UNIX timestamp).
  next\_notification         | Number        | When the next notification is going to be sent for this assuming the associated host/service is still in a non-OK state (as a UNIX timestamp).
  notification\_number      | Number        | The notification number
  last\_problem\_notification | Number      | When the last notification was sent for a problem (as a UNIX timestamp).


## <a id="objecttype-notificationcommand"></a> NotificationCommand

A notification command definition.

> **Note**
>
> Icinga 2 versions < 2.6.0 require the import of the [plugin-notification-command](10-icinga-template-library.md#itl-plugin-notification-command) template.

Example:

    object NotificationCommand "mail-service-notification" {
      command = [
        SysconfDir + "/icinga2/scripts/mail-notification.sh"
      ]

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

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  execute         |**Required.** The "execute" script method takes care of executing the notification. The default template "plugin-notification-command" which is imported into all CheckCommand objects takes care of this setting.
  command         |**Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command.
  env             |**Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout         |**Optional.** The command timeout in seconds. Defaults to 60 seconds.
  arguments       |**Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for [CheckCommand objects](9-object-types.md#objecttype-checkcommand-arguments).


## <a id="objecttype-notificationcomponent"></a> NotificationComponent

The notification component is responsible for sending notifications. There are no configurable options.

Example:

    library "notification"

    object NotificationComponent "notification" { }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  enable\_ha      |**Optional.** Enable the high availability functionality. Only valid in a [cluster setup](6-distributed-monitoring.md#distributed-monitoring-high-availability-notifications). Disabling this currently only affects reminder notifications. Defaults to "true".

## <a id="objecttype-opentsdbwriter"></a> OpenTsdbWriter

Writes check result metrics and performance data to [OpenTSDB](http://opentsdb.net).

Example:

    library "perfdata"

    object OpenTsdbWriter "opentsdb" {
      host = "127.0.0.1"
      port = 4242
    }

Configuration Attributes:

  Name            	|Description
  ----------------------|----------------------
  host            	|**Optional.** OpenTSDB host address. Defaults to '127.0.0.1'.
  port            	|**Optional.** OpenTSDB port. Defaults to 4242.


## <a id="objecttype-perfdatawriter"></a> PerfdataWriter

Writes check result performance data to a defined path using macro
pattern consisting of custom attributes and runtime macros.

Example:

    library "perfdata"

    object PerfdataWriter "pnp" {
      host_perfdata_path = "/var/spool/icinga2/perfdata/host-perfdata"

      service_perfdata_path = "/var/spool/icinga2/perfdata/service-perfdata"

      host_format_template = "DATATYPE::HOSTPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tHOSTPERFDATA::$host.perfdata$\tHOSTCHECKCOMMAND::$host.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$"
      service_format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tSERVICEDESC::$service.name$\tSERVICEPERFDATA::$service.perfdata$\tSERVICECHECKCOMMAND::$service.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$\tSERVICESTATE::$service.state$\tSERVICESTATETYPE::$service.state_type$"

      rotation_interval = 15s
    }

Configuration Attributes:

  Name                    |Description
  ------------------------|----------------
  host_perfdata\_path     |**Optional.** Path to the host performance data file. Defaults to LocalStateDir + "/spool/icinga2/perfdata/host-perfdata".
  service_perfdata\_path  |**Optional.** Path to the service performance data file. Defaults to LocalStateDir + "/spool/icinga2/perfdata/service-perfdata".
  host_temp\_path         |**Optional.** Path to the temporary host file. Defaults to LocalStateDir + "/spool/icinga2/tmp/host-perfdata".
  service_temp\_path      |**Optional.** Path to the temporary service file. Defaults to LocalStateDir + "/spool/icinga2/tmp/service-perfdata".
  host_format\_template   |**Optional.** Host Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  service_format\_template|**Optional.** Service Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  rotation\_interval      |**Optional.** Rotation interval for the files specified in `{host,service}_perfdata_path`. Defaults to 30 seconds.

When rotating the performance data file the current UNIX timestamp is appended to the path specified
in `host_perfdata_path` and `service_perfdata_path` to generate a unique filename.


## <a id="objecttype-scheduleddowntime"></a> ScheduledDowntime

ScheduledDowntime objects can be used to set up recurring downtimes for hosts/services.

> **Best Practice**
>
> Rather than creating a `ScheduledDowntime` object for a specific host or service it is usually easier
> to just create a `ScheduledDowntime` template and use the `apply` keyword to assign the
> scheduled downtime to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [recurring downtimes](8-advanced-topics.md#recurring-downtimes) example for details.

Example:

    object ScheduledDowntime "some-downtime" {
      host_name = "localhost"
      service_name = "ping4"

      author = "icingaadmin"
      comment = "Some comment"

      fixed = false
      duration = 30m

      ranges = {
        "sunday" = "02:00-03:00"
      }
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  host_name       |**Required.** The name of the host this scheduled downtime belongs to.
  service_name    |**Optional.** The short name of the service this scheduled downtime belongs to. If omitted, this downtime object is treated as host downtime.
  author          |**Required.** The author of the downtime.
  comment         |**Required.** A comment for the downtime.
  fixed           |**Optional.** Whether this is a fixed downtime. Defaults to true.
  duration        |**Optional.** How long the downtime lasts. Only has an effect for flexible (non-fixed) downtimes.
  ranges          |**Required.** A dictionary containing information which days and durations apply to this timeperiod.

ScheduledDowntime objects have composite names, i.e. their names are based
on the `host_name` and `service_name` attributes and the
name you specified. This means you can define more than one object
with the same (short) name as long as one of the `host_name` and
`service_name` attributes has a different value.


## <a id="objecttype-service"></a> Service

Service objects describe network services and how they should be checked
by Icinga 2.

> **Best Practice**
>
> Rather than creating a `Service` object for a specific host it is usually easier
> to just create a `Service` template and use the `apply` keyword to assign the
> service to a number of hosts.
> Check the [apply](3-monitoring-basics.md#using-apply) chapter for details.

Example:

    object Service "uptime" {
      host_name = "localhost"

      display_name = "localhost Uptime"

      check_command = "check_snmp"

      vars.community = "public"
      vars.oid = "DISMAN-EVENT-MIB::sysUpTimeInstance"

      check_interval = 60s
      retry_interval = 15s

      groups = [ "all-services", "snmp" ]
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the service.
  host_name       |**Required.** The host this service belongs to. There must be a `Host` object with that name.
  name            |**Required.** The service name. Must be unique on a per-host basis (Similar to the service_description attribute in Icinga 1.x).
  groups          |**Optional.** The service groups this service belongs to.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this service.
  check\_command  |**Required.** The name of the check command.
  max\_check\_attempts|**Optional.** The number of times a service is re-checked before changing into a hard state. Defaults to 3.
  check\_period   |**Optional.** The name of a time period which determines when this service should be checked. Not set by default.
  check\_timeout  |**Optional.** Check command timeout in seconds. Overrides the CheckCommand's `timeout` attribute.
  check\_interval |**Optional.** The check interval (in seconds). This interval is used for checks when the service is in a `HARD` state. Defaults to 5 minutes.
  retry\_interval |**Optional.** The retry interval (in seconds). This interval is used for checks when the service is in a `SOFT` state. Defaults to 1 minute.
  enable\_notifications|**Optional.** Whether notifications are enabled. Defaults to true.
  enable\_active\_checks|**Optional.** Whether active checks are enabled. Defaults to true.
  enable\_passive\_checks|**Optional.** Whether passive checks are enabled. Defaults to true.
  enable\_event\_handler|**Optional.** Enables event handlers for this host. Defaults to true.
  enable\_flapping|**Optional.** Whether flap detection is enabled. Defaults to false.
  enable\_perfdata|**Optional.** Whether performance data processing is enabled. Defaults to true.
  event\_command  |**Optional.** The name of an event command that should be executed every time the service's state changes or the service is in a `SOFT` state.
  flapping\_threshold|**Optional.** The flapping threshold in percent when a service is considered to be flapping.
  volatile        |**Optional.** The volatile setting enables always `HARD` state types if `NOT-OK` state changes occur.
  zone		  |**Optional.** The zone this object is a member of.
  command\_endpoint|**Optional.** The endpoint where commands are executed on.
  notes           |**Optional.** Notes for the service.
  notes\_url      |**Optional.** Url for notes for the service (for example, in notification commands).
  action_url      |**Optional.** Url for actions for the service (for example, an external graphing tool).
  icon\_image     |**Optional.** Icon image for the service. Used by external interfaces only.
  icon\_image\_alt|**Optional.** Icon image description for the service. Used by external interface only.

Service objects have composite names, i.e. their names are based on the host_name attribute and the name you specified. This means
you can define more than one object with the same (short) name as long as the `host_name` attribute has a different value.

The actual check interval might deviate slightly from the configured values due to the fact that Icinga tries
to evenly distribute all checks over a certain period of time, i.e. to avoid load spikes.

Runtime Attributes:

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  next\_check               | Number        | When the next check occurs (as a UNIX timestamp).
  last\_check               | Number        | When the last check occured (as a UNIX timestamp).
  check\_attempt            | Number        | The current check attempt number.
  state\_type               | Number        | The current state type (0 = SOFT, 1 = HARD).
  last\_state\_type         | Number        | The previous state type (0 = SOFT, 1 = HARD).
  last\_reachable           | Boolean       | Whether the service was reachable when the last check occurred.
  last\_check\_result       | CheckResult   | The current check result.
  last\_state\_change       | Number        | When the last state change occurred (as a UNIX timestamp).
  last\_hard\_state\_change | Number        | When the last hard state change occurred (as a UNIX timestamp).
  last\_in\_downtime        | Boolean       | Whether the service was in a downtime when the last check occurred.
  acknowledgement           | Number        | The acknowledgement type (0 = NONE, 1 = NORMAL, 2 = STICKY).
  acknowledgement_expiry    | Number        | When the acknowledgement expires (as a UNIX timestamp; 0 = no expiry).
  downtime\_depth           | Number        | Whether the service has one or more active downtimes.
  flapping_last_change      | Number        | When the last flapping change occurred (as a UNIX timestamp).
  flapping                  | Boolean       | Whether the host is flapping between states.
  state                     | Number        | The current state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last\_state               | Number        | The previous state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last\_hard\_state         | Number        | The last hard state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last_state_ok             | Number        | When the last OK state occurred (as a UNIX timestamp).
  last_state_warning        | Number        | When the last WARNING state occurred (as a UNIX timestamp).
  last_state_critical       | Number        | When the last CRITICAL state occurred (as a UNIX timestamp).
  last_state_unknown        | Number        | When the last UNKNOWN state occurred (as a UNIX timestamp).


## <a id="objecttype-servicegroup"></a> ServiceGroup

A group of services.

> **Best Practice**
>
> Assign service group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

    object ServiceGroup "snmp" {
      display_name = "SNMP services"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the service group.
  groups          |**Optional.** An array of nested group names.


## <a id="objecttype-statusdatawriter"></a> StatusDataWriter

Periodically writes status data files which are used by the Classic UI and other third-party tools.

Example:

    library "compat"

    object StatusDataWriter "status" {
        status_path = "/var/cache/icinga2/status.dat"
        objects_path = "/var/cache/icinga2/objects.cache"
        update_interval = 30s
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  status\_path    |**Optional.** Path to the status.dat file. Defaults to LocalStateDir + "/cache/icinga2/status.dat".
  objects\_path   |**Optional.** Path to the objects.cache file. Defaults to LocalStateDir + "/cache/icinga2/objects.cache".
  update\_interval|**Optional.** The interval in which the status files are updated. Defaults to 15 seconds.


## <a id="objecttype-sysloglogger"></a> SyslogLogger

Specifies Icinga 2 logging to syslog.

Example:

    object SyslogLogger "crit-syslog" {
      severity = "critical"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  severity        |**Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "warning" or "critical". Defaults to "warning".


## <a id="objecttype-timeperiod"></a> TimePeriod

Time periods can be used to specify when hosts/services should be checked or to limit
when notifications should be sent out.

Examples:

    object TimePeriod "nonworkhours" {
      import "legacy-timeperiod"

      display_name = "Icinga 2 TimePeriod for non working hours"

      ranges = {
        monday = "00:00-8:00,17:00-24:00"
        tuesday = "00:00-8:00,17:00-24:00"
        wednesday = "00:00-8:00,17:00-24:00"
        thursday = "00:00-8:00,17:00-24:00"
        friday = "00:00-8:00,16:00-24:00"
        saturday = "00:00-24:00"
        sunday = "00:00-24:00"
      }
    }

    object TimePeriod "exampledays" {
        import "legacy-timeperiod"

        display_name = "Icinga 2 TimePeriod for random example days"

        ranges = {
            //We still believe in Santa, no peeking!
            //Applies every 25th of December every year
            "december 25" = "00:00-24:00"

            //Any point in time can be specified,
            //but you still have to use a range
            "2038-01-19" = "03:13-03:15"

            //Evey 3rd day from the second monday of February
            //to 8th of November
            "monday 2 february - november 8 / 3" = "00:00-24:00"
        }
    }


Additional examples can be found [here](8-advanced-topics.md#timeperiods).

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the time period.
  update          |**Required.** The "update" script method takes care of updating the internal representation of the time period. In virtually all cases you should import the "legacy-timeperiod" template to take care of this setting.
  ranges          |**Required.** A dictionary containing information which days and durations apply to this timeperiod.
  prefer_includes |**Optional.** Boolean whether to prefer timeperiods `includes` or `excludes`. Default to true.
  excludes        |**Optional.** An array of timeperiods, which should exclude from your timerange.
  includes        |**Optional.** An array of timeperiods, which should include into your timerange

The `/etc/icinga2/conf.d/timeperiods.conf` file is usually used to define
timeperiods including this one.

Runtime Attributes:

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  is\_inside                | Boolean       | Whether we're currently inside this timeperiod.


## <a id="objecttype-user"></a> User

A user.

Example:

    object User "icingaadmin" {
      display_name = "Icinga 2 Admin"
      groups = [ "icingaadmins" ]
      email = "icinga@localhost"
      pager = "icingaadmin@localhost.localdomain"

      period = "24x7"

      states = [ OK, Warning, Critical, Unknown ]
      types = [ Problem, Recovery ]

      vars.additional_notes = "This is the Icinga 2 Admin account."
    }

Available notification state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

Available notification type filters:

    DowntimeStart
    DowntimeEnd
    DowntimeRemoved
    Custom
    Acknowledgement
    Problem
    Recovery
    FlappingStart
    FlappingEnd

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the user.
  email           |**Optional.** An email string for this user. Useful for notification commands.
  pager           |**Optional.** A pager string for this user. Useful for notification commands.
  vars            |**Optional.** A dictionary containing custom attributes that are specific to this user.
  groups          |**Optional.** An array of group names.
  enable_notifications|**Optional.** Whether notifications are enabled for this user.
  period          |**Optional.** The name of a time period which determines when a notification for this user should be triggered. Not set by default.
  types           |**Optional.** A set of type filters when this notification should be triggered. By default everything is matched.
  states          |**Optional.** A set of state filters when this notification should be triggered. By default everything is matched.

Runtime Attributes:

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  last\_notification        | Number        | When the last notification was sent for this user (as a UNIX timestamp).

## <a id="objecttype-usergroup"></a> UserGroup

A user group.

> **Best Practice**
>
> Assign user group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

    object UserGroup "icingaadmins" {
        display_name = "Icinga 2 Admin Group"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  display_name    |**Optional.** A short description of the user group.
  groups          |**Optional.** An array of nested group names.


## <a id="objecttype-zone"></a> Zone

Zone objects are used to specify which Icinga 2 instances are located in a zone.

Example:

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]

    }

    object Zone "check-satellite" {
      endpoints = [ "icinga2c" ]
      parent = "config-ha-master"
    }

Configuration Attributes:

  Name            |Description
  ----------------|----------------
  endpoints       |**Optional.** Array of endpoint names located in this zone.
  parent          |**Optional.** The name of the parent zone.
  global          |**Optional.** Whether configuration files for this zone should be synced to all endpoints. Defaults to false.

Zone objects cannot currently be created with the API.

# <a id="value-types"></a> Value Types

In addition to [configuration objects](9-object-types.md#object-types) Icinga 2 also uses a few other types to represent its internal state. The following types are exposed via the [API](12-icinga2-api.md#icinga2-api).

## <a id="value-types-checkresult"></a> CheckResult

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  exit_status               | Number        | The exit status returned by the check execution.
  output                    | String        | The check output.
  performance_data          | Array         | Array of [performance data values](9-object-types.md#value-types-perfdatavalue).
  check_source              | String        | Name of the node executing the check.
  state                     | Number        | The current state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  command                   | Value         | Array of command with shell-escaped arguments or command line string.
  execution_start           | Number        | Check execution start time (as a UNIX timestamp).
  execution_end             | Number        | Check execution end time (as a UNIX timestamp).
  schedule_start            | Number        | Scheduled check execution start time (as a UNIX timestamp).
  schedule_end              | Number        | Scheduled check execution end time (as a UNIX timestamp).
  active                    | Boolean       | Whether the result is from an active or passive check.
  vars_before               | Dictionary    | Internal attribute used for calculations.
  vars_after                | Dictionary    | Internal attribute used for calculations.

## <a id="value-types-perfdatavalue"></a> PerfdataValue

Icinga 2 parses performance data strings returned by check plugins and makes the information available to external interfaces (e.g. [GraphiteWriter](9-object-types.md#objecttype-graphitewriter) or the [Icinga 2 API](12-icinga2-api.md#icinga2-api)).

  Name                      | Type          | Description
  --------------------------|---------------|-----------------
  label                     | String        | Performance data label.
  value                     | Number        | Normalized performance data value without unit.
  counter                   | Boolean       | Enabled if the original value contains `c` as unit. Defaults to `false`.
  unit                      | String        | Unit of measurement (`seconds`, `bytes`. `percent`) according to the [plugin API](5-service-monitoring.md#service-monitoring-plugin-api).
  crit                      | Value         | Critical threshold value.
  warn                      | Value         | Warning threshold value.
  min                       | Value         | Minimum value returned by the check.
  max                       | Value         | Maximum value returned by the check.


