# Config Object Types <a id="object-types"></a>

This chapter provides an overview of all available config object types which can be
instantiated using the `object` keyword.

Additional details on configuration and runtime attributes and their
description are explained here too.

The attributes need to have a specific type value. Many of them are
explained in [this chapter](03-monitoring-basics.md#attribute-value-types) already.
You should note that the `Timestamp` type is a `Number`.
In addition to that `Object name` is an object reference to
an existing object name as `String` type.

Configuration objects share these runtime attributes which cannot be
modified by the user. You can access these attributes using
the [Icinga 2 API](12-icinga2-api.md#icinga2-api-config-objects).

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  version                   | Number                | Timestamp when the object was created or modified. Synced throughout cluster nodes.
  type                      | String                | Object type.
  original\_attributes      | Dictionary            | Original values of object attributes modified at runtime.
  active                    | Boolean               | Object is active (e.g. a service being checked).
  paused                    | Boolean               | Object has been paused at runtime (e.g. [IdoMysqlConnection](09-object-types.md#objecttype-idomysqlconnection). Defaults to `false`.
  templates                 | Array                 | Templates imported on object compilation.
  package                   | String                | [Configuration package name](12-icinga2-api.md#icinga2-api-config-management) this object belongs to. Local configuration is set to `_etc`, runtime created objects use `_api`.
  source\_location          | Dictionary            | Location information where the configuration files are stored.


## ApiListener <a id="objecttype-apilistener"></a>

ApiListener objects are used for distributed monitoring setups
and API usage specifying the certificate files used for ssl
authorization and additional restrictions.
This configuration object is available as [api feature](11-cli-commands.md#cli-command-feature).

The `TicketSalt` constant must be defined in [constants.conf](04-configuring-icinga-2.md#constants-conf).

Example:

```
object ApiListener "api" {
  accept_commands = true
  accept_config = true

  ticket_salt = TicketSalt
}
```

Configuration Attributes:

  Name                                  | Type                  | Description
  --------------------------------------|-----------------------|----------------------------------
  cert\_path                            | String                | **Deprecated.** Path to the public key.
  key\_path                             | String                | **Deprecated.** Path to the private key.
  ca\_path                              | String                | **Deprecated.** Path to the CA certificate file.
  ticket\_salt                          | String                | **Optional.** Private key for [CSR auto-signing](06-distributed-monitoring.md#distributed-monitoring-setup-csr-auto-signing). **Required** for a signing master instance.
  crl\_path                             | String                | **Optional.** Path to the CRL file.
  bind\_host                            | String                | **Optional.** The IP address the api listener should be bound to. If not specified, the ApiListener is bound to `::` and listens for both IPv4 and IPv6 connections.
  bind\_port                            | Number                | **Optional.** The port the api listener should be bound to. Defaults to `5665`.
  accept\_config                        | Boolean               | **Optional.** Accept zone configuration. Defaults to `false`.
  accept\_commands                      | Boolean               | **Optional.** Accept remote commands. Defaults to `false`.
  max\_anonymous\_clients               | Number                | **Optional.** Limit the number of anonymous client connections (not configured endpoints and signing requests).
  cipher\_list                          | String                | **Optional.** Cipher list that is allowed. For a list of available ciphers run `openssl ciphers`. Defaults to `ALL:!LOW:!WEAK:!MEDIUM:!EXP:!NULL`.
  tls\_protocolmin                      | String                | **Optional.** Minimum TLS protocol version. Must be one of `TLSv1`, `TLSv1.1` or `TLSv1.2`. Defaults to `TLSv1`.
  tls\_handshake\_timeout               | Number                | **Optional.** TLS Handshake timeout. Defaults to `10s`.
  access\_control\_allow\_origin        | Array                 | **Optional.** Specifies an array of origin URLs that may access the API. [(MDN docs)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS#Access-Control-Allow-Origin)
  access\_control\_allow\_credentials   | Boolean               | **Deprecated.** Indicates whether or not the actual request can be made using credentials. Defaults to `true`. [(MDN docs)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS#Access-Control-Allow-Credentials)
  access\_control\_allow\_headers       | String                | **Deprecated.** Used in response to a preflight request to indicate which HTTP headers can be used when making the actual request. Defaults to `Authorization`. [(MDN docs)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS#Access-Control-Allow-Headers)
  access\_control\_allow\_methods       | String                | **Deprecated.** Used in response to a preflight request to indicate which HTTP methods can be used when making the actual request. Defaults to `GET, POST, PUT, DELETE`. [(MDN docs)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS#Access-Control-Allow-Methods)
  environment                           | String                | **Optional.** Used as suffix in TLS SNI extension name; default from constant `ApiEnvironment`, which is empty.

The attributes `access_control_allow_credentials`, `access_control_allow_headers` and `access_control_allow_methods`
are controlled by Icinga 2 and are not changeable by config any more.


The ApiListener type expects its certificate files to be in the following locations:

  Type                 | Location
  ---------------------|-------------------------------------
  Private key          | `DataDir + "/certs/" + NodeName + ".key"`
  Certificate file     | `DataDir + "/certs/" + NodeName + ".crt"`
  CA certificate file  | `DataDir + "/certs/ca.crt"`

If the deprecated attributes `cert_path`, `key_path` and/or `ca_path` are specified Icinga 2
copies those files to the new location in `DataDir + "/certs"` unless the
file(s) there are newer.

Please check the [upgrading chapter](16-upgrading-icinga-2.md#upgrading-to-2-8-certificate-paths) for more details.

While Icinga 2 and the underlying OpenSSL library use sane and secure defaults, the attributes
`cipher_list` and `tls_protocolmin` can be used to increase communication security. A good source
for a more secure configuration is provided by the [Mozilla Wiki](https://wiki.mozilla.org/Security/Server_Side_TLS).
Ensure to use the same configuration for both attributes on **all** endpoints to avoid communication problems which
requires to use `cipher_list` compatible with the endpoint using the oldest version of the OpenSSL library. If using
other tools to connect to the API ensure also compatibility with them as this setting affects not only inter-cluster
communcation but also the REST API.

## ApiUser <a id="objecttype-apiuser"></a>

ApiUser objects are used for authentication against the [Icinga 2 API](12-icinga2-api.md#icinga2-api-authentication).

Example:

```
object ApiUser "root" {
  password = "mysecretapipassword"
  permissions = [ "*" ]
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  password                  | String                | **Optional.** Password string. Note: This attribute is hidden in API responses.
  client\_cn                | String                | **Optional.** Client Common Name (CN).
  permissions               | Array                 | **Required.** Array of permissions. Either as string or dictionary with the keys `permission` and `filter`. The latter must be specified as function.

Available permissions are explained in the [API permissions](12-icinga2-api.md#icinga2-api-permissions)
chapter.

## CheckCommand <a id="objecttype-checkcommand"></a>

A check command definition. Additional default command custom attributes can be
defined here.

Example:

```
object CheckCommand "http" {
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
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  command                   | Array                 | **Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command. When using the "arguments" attribute this must be an array. Can be specified as function for advanced implementations.
  env                       | Dictionary            | **Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout                   | Duration              | **Optional.** The command timeout in seconds. Defaults to `1m`.
  arguments                 | Dictionary            | **Optional.** A dictionary of command arguments.


### CheckCommand Arguments <a id="objecttype-checkcommand-arguments"></a>

Command arguments can be defined as key-value-pairs in the `arguments`
dictionary. If the argument requires additional configuration, for example
a `description` attribute or an optional condition, the value can be defined
as dictionary specifying additional options.

Service:

```
vars.x_val = "My command argument value."
vars.have_x = "true"
```

CheckCommand:

```
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
```

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  value                     | String/Function       | Optional argument value set by a [runtime macro string](03-monitoring-basics.md#runtime-macros) or a [function call](17-language-reference.md#functions).
  key 	                    | String                | Optional argument key overriding the key identifier.
  description               | String                | Optional argument description.
  required                  | Boolean               | Required argument. Execution error if not set. Defaults to false (optional).
  skip\_key                 | Boolean               | Use the value as argument and skip the key.
  set\_if                   | String/Function       | Argument is added if the [runtime macro string](03-monitoring-basics.md#runtime-macros) resolves to a defined numeric or boolean value. String values are not supported. [Function calls](17-language-reference.md#functions) returning a value are supported too.
  order                     | Number                | Set if multiple arguments require a defined argument order.
  repeat\_key               | Boolean               | If the argument value is an array, repeat the argument key, or not. Defaults to true (repeat).

Argument order:

```
..., -3, -2, -1, <un-ordered keys>, 1, 2, 3, ...
```

Define argument array:

```
value = "[ 'one', 'two', 'three' ]"
```

Argument array with `repeat_key = true`:

```
'key' 'value[0]' 'key' 'value[1]' 'key' 'value[2]'
```

Argument array with `repeat_key = false`:

```
'key' 'value[0]' 'value[1]' 'value[2]'
```

## CheckerComponent <a id="objecttype-checkcomponent"></a>

The checker component is responsible for scheduling active checks.
This configuration object is available as [checker feature](11-cli-commands.md#cli-command-feature).

Example:

```
object CheckerComponent "checker" { }
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  concurrent\_checks        | Number                | **Optional and deprecated.** The maximum number of concurrent checks. Was replaced by global constant `MaxConcurrentChecks` which will be set if you still use `concurrent_checks`.

## CheckResultReader <a id="objecttype-checkresultreader"></a>

Reads Icinga 1.x check result files from a directory. This functionality is provided
to help existing Icinga 1.x users and might be useful for migration scenarios.

Example:

```
object CheckResultReader "reader" {
  spool_dir = "/data/check-results"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  spool\_dir                | String                | **Optional.** The directory which contains the check result files. Defaults to DataDir + "/spool/checkresults/".

## Comment <a id="objecttype-comment"></a>

Comments created at runtime are represented as objects.
Note: This is for reference only. You can create comments
with the [add-comment](12-icinga2-api.md#icinga2-api-actions-add-comment) API action.

Example:

```
object Comment "my-comment" {
  host_name = "localhost"
  author = "icingaadmin"
  text = "This is a comment."
  entry_time = 1234567890
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host\_name                | Object name           | **Required.** The name of the host this comment belongs to.
  service\_name             | Object name           | **Optional.** The short name of the service this comment belongs to. If omitted, this comment object is treated as host comment.
  author                    | String                | **Required.** The author's name.
  text                      | String                | **Required.** The comment text.
  entry\_time               | Timestamp             | **Optional.** The UNIX timestamp when this comment was added. If omitted, the entry time is volatile!
  entry\_type               | Number                | **Optional.** The comment type (`User` = 1, `Downtime` = 2, `Flapping` = 3, `Acknowledgement` = 4).
  expire\_time              | Timestamp             | **Optional.** The comment's expire time as UNIX timestamp.
  persistent                | Boolean               | **Optional.** Only evaluated for `entry_type` Acknowledgement. `true` does not remove the comment when the acknowledgement is removed.

## CompatLogger <a id="objecttype-compatlogger"></a>

Writes log files in a format that's compatible with Icinga 1.x.
This configuration object is available as [compatlog feature](14-features.md#compat-logging).

Example:

```
object CompatLogger "compatlog" {
  log_dir = "/var/log/icinga2/compat"
  rotation_method = "DAILY"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  log\_dir                  | String                | **Optional.** Path to the compat log directory. Defaults to LogDir + "/compat".
  rotation\_method          | String                | **Optional.** Specifies when to rotate log files. Can be one of "HOURLY", "DAILY", "WEEKLY" or "MONTHLY". Defaults to "HOURLY".


## Dependency <a id="objecttype-dependency"></a>

Dependency objects are used to specify dependencies between hosts and services. Dependencies
can be defined as Host-to-Host, Service-to-Service, Service-to-Host, or Host-to-Service
relations.

> **Best Practice**
>
> Rather than creating a `Dependency` object for a specific host or service it is usually easier
> to just create a `Dependency` template and use the `apply` keyword to assign the
> dependency to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [dependencies](03-monitoring-basics.md#dependencies) chapter for detailed examples.

Service-to-Service Example:

```
object Dependency "webserver-internet" {
  parent_host_name = "internet"
  parent_service_name = "ping4"

  child_host_name = "webserver"
  child_service_name = "ping4"

  states = [ OK, Warning ]

  disable_checks = true
}
```

Host-to-Host Example:

```
object Dependency "webserver-internet" {
  parent_host_name = "internet"

  child_host_name = "webserver"

  states = [ Up ]

  disable_checks = true
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  parent\_host\_name        | Object name           | **Required.** The parent host.
  parent\_service\_name     | Object name           | **Optional.** The parent service. If omitted, this dependency object is treated as host dependency.
  child\_host\_name         | Object name           | **Required.** The child host.
  child\_service\_name      | Object name           | **Optional.** The child service. If omitted, this dependency object is treated as host dependency.
  disable\_checks           | Boolean               | **Optional.** Whether to disable checks when this dependency fails. Defaults to false.
  disable\_notifications    | Boolean               | **Optional.** Whether to disable notifications when this dependency fails. Defaults to true.
  ignore\_soft\_states      | Boolean               | **Optional.** Whether to ignore soft states for the reachability calculation. Defaults to true.
  period                    | Object name           | **Optional.** Time period object during which this dependency is enabled.
  states    	            | Array                 | **Optional.** A list of state filters when this dependency should be OK. Defaults to [ OK, Warning ] for services and [ Up ] for hosts.

Available state filters:

    OK
    Warning
    Critical
    Unknown
    Up
    Down

When using [apply rules](03-monitoring-basics.md#using-apply) for dependencies, you can leave out certain attributes which will be
automatically determined by Icinga 2.

Service-to-Host Dependency Example:

```
apply Dependency "internet" to Service {
  parent_host_name = "dsl-router"
  disable_checks = true

  assign where host.name != "dsl-router"
}
```

This example sets all service objects matching the assign condition into a dependency relation to
the parent host object `dsl-router` as implicit child services.

Service-to-Service-on-the-same-Host Dependency Example:

```
apply Dependency "disable-agent-checks" to Service {
  parent_service_name = "agent-health"

  assign where service.check_command == "ssh"
  ignore where service.name == "agent-health"
}
```

This example omits the `parent_host_name` attribute and Icinga 2 automatically sets its value to the name of the
host object matched by the apply rule condition. All services where apply matches are made implicit child services
in this dependency relation.


Dependency objects have composite names, i.e. their names are based on the `child_host_name` and `child_service_name` attributes and the
name you specified. This means you can define more than one object with the same (short) name as long as one of the `child_host_name` and
`child_service_name` attributes has a different value.

## Downtime <a id="objecttype-downtime"></a>

Downtimes created at runtime are represented as objects.
You can create downtimes with the [schedule-downtime](12-icinga2-api.md#icinga2-api-actions-schedule-downtime) API action.

Example:

```
object Downtime "my-downtime" {
  host_name = "localhost"
  author = "icingaadmin"
  comment = "This is a downtime."
  start_time = 1505312869
  end_time = 1505312924
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host\_name                | Object name           | **Required.** The name of the host this comment belongs to.
  service\_name             | Object name           | **Optional.** The short name of the service this comment belongs to. If omitted, this comment object is treated as host comment.
  author                    | String                | **Required.** The author's name.
  comment                   | String                | **Required.** The comment text.
  start\_time               | Timestamp             | **Required.** The start time as UNIX timestamp.
  end\_time                 | Timestamp             | **Required.** The end time as UNIX timestamp.
  duration                  | Number                | **Optional.** The duration as number.
  entry\_time               | Timestamp             | **Optional.** The UNIX timestamp when this downtime was added.
  fixed                     | Boolean               | **Optional.** Whether the downtime is fixed (true) or flexible (false). Defaults to flexible. Details in the [advanced topics chapter](08-advanced-topics.md#fixed-flexible-downtimes).
  triggers                  | Array of object names | **Optional.** List of downtimes which should be triggered by this downtime.

Runtime Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  trigger\_time             | Timestamp             | The UNIX timestamp when this downtime was triggered.
  triggered\_by             | Object name           | The name of the downtime this downtime was triggered by.


## ElasticsearchWriter <a id="objecttype-elasticsearchwriter"></a>

Writes check result metrics and performance data to an Elasticsearch instance.
This configuration object is available as [elasticsearch feature](14-features.md#elasticsearch-writer).

Example:

```
object ElasticsearchWriter "elasticsearch" {
  host = "127.0.0.1"
  port = 9200
  index = "icinga2"

  enable_send_perfdata = true

  flush_threshold = 1024
  flush_interval = 10
}
```

The index is rotated daily, as is recommended by Elastic, meaning the index will be renamed to `$index-$d.$M.$y`.

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Required.** Elasticsearch host address. Defaults to `127.0.0.1`.
  port                      | Number                | **Required.** Elasticsearch port. Defaults to `9200`.
  index                     | String                | **Required.** Elasticsearch index name. Defaults to `icinga2`.
  enable\_send\_perfdata    | Boolean               | **Optional.** Send parsed performance data metrics for check results. Defaults to `false`.
  flush\_interval           | Duration              | **Optional.** How long to buffer data points before transferring to Elasticsearch. Defaults to `10s`.
  flush\_threshold          | Number                | **Optional.** How many data points to buffer before forcing a transfer to Elasticsearch.  Defaults to `1024`.
  username                  | String                | **Optional.** Basic auth username if Elasticsearch is hidden behind an HTTP proxy.
  password                  | String                | **Optional.** Basic auth password if Elasticsearch is hidden behind an HTTP proxy.
  enable\_tls               | Boolean               | **Optional.** Whether to use a TLS stream. Defaults to `false`. Requires an HTTP proxy.
  ca\_path                  | String                | **Optional.** Path to CA certificate to validate the remote host. Requires `enable_tls` set to `true`.
  cert\_path                | String                | **Optional.** Path to host certificate to present to the remote host for mutual verification. Requires `enable_tls` set to `true`.
  key\_path                 | String                | **Optional.** Path to host key to accompany the cert\_path. Requires `enable_tls` set to `true`.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.

Note: If `flush_threshold` is set too low, this will force the feature to flush all data to Elasticsearch too often.
Experiment with the setting, if you are processing more than 1024 metrics per second or similar.

Basic auth is supported with the `username` and `password` attributes. This requires an
HTTP proxy (Nginx, etc.) in front of the Elasticsearch instance. Check [this blogpost](https://blog.netways.de/2017/09/14/secure-elasticsearch-and-kibana-with-an-nginx-http-proxy/)
for an example.

TLS for the HTTP proxy can be enabled with `enable_tls`. In addition to that
you can specify the certificates with the `ca_path`, `cert_path` and `cert_key` attributes.

## Endpoint <a id="objecttype-endpoint"></a>

Endpoint objects are used to specify connection information for remote
Icinga 2 instances. More details can be found in the [distributed monitoring chapter](06-distributed-monitoring.md#distributed-monitoring).

Example:

```
object Endpoint "icinga2-client1.localdomain" {
  host = "192.168.56.111"
  port = 5665
  log_duration = 1d
}
```

Example (disable replay log):

```
object Endpoint "icinga2-client1.localdomain" {
  host = "192.168.5.111"
  port = 5665
  log_duration = 0
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Optional.** The hostname/IP address of the remote Icinga 2 instance.
  port                      | Number                | **Optional.** The service name/port of the remote Icinga 2 instance. Defaults to `5665`.
  log\_duration             | Duration              | **Optional.** Duration for keeping replay logs on connection loss. Defaults to `1d` (86400 seconds). Attribute is specified in seconds. If log_duration is set to 0, replaying logs is disabled. You could also specify the value in human readable format like `10m` for 10 minutes or `1h` for one hour.

Endpoint objects cannot currently be created with the API.

## EventCommand <a id="objecttype-eventcommand"></a>

An event command definition.

Example:

```
object EventCommand "restart-httpd-event" {
  command = "/opt/bin/restart-httpd.sh"
}
```


Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  command                   | Array                 | **Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command. When using the "arguments" attribute this must be an array. Can be specified as function for advanced implementations.
  env                       | Dictionary            | **Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout                   | Duration              | **Optional.** The command timeout in seconds. Defaults to `1m`.
  arguments                 | Dictionary            | **Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for [CheckCommand objects](09-object-types.md#objecttype-checkcommand-arguments).

More advanced examples for event command usage can be found [here](03-monitoring-basics.md#event-commands).

## ExternalCommandListener <a id="objecttype-externalcommandlistener"></a>

Implements the Icinga 1.x command pipe which can be used to send commands to Icinga.
This configuration object is available as [command feature](14-features.md#external-commands).

Example:

```
object ExternalCommandListener "command" {
    command_path = "/var/run/icinga2/cmd/icinga2.cmd"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  command\_path             | String                | **Optional.** Path to the command pipe. Defaults to RunDir + "/icinga2/cmd/icinga2.cmd".



## FileLogger <a id="objecttype-filelogger"></a>

Specifies Icinga 2 logging to a file.
This configuration object is available as `mainlog` and `debuglog` [logging feature](14-features.md#logging).

Example:

```
object FileLogger "debug-file" {
  severity = "debug"
  path = "/var/log/icinga2/debug.log"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  path                      | String                | **Required.** The log path.
  severity                  | String                | **Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "warning" or "critical". Defaults to "information".


## GelfWriter <a id="objecttype-gelfwriter"></a>

Writes event log entries to a defined GELF receiver host (Graylog, Logstash).
This configuration object is available as [gelf feature](14-features.md#gelfwriter).

Example:

```
object GelfWriter "gelf" {
  host = "127.0.0.1"
  port = 12201
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Optional.** GELF receiver host address. Defaults to `127.0.0.1`.
  port                      | Number                | **Optional.** GELF receiver port. Defaults to `12201`.
  source                    | String                | **Optional.** Source name for this instance. Defaults to `icinga2`.
  enable\_send\_perfdata    | Boolean               | **Optional.** Enable performance data for 'CHECK RESULT' events.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.


## GraphiteWriter <a id="objecttype-graphitewriter"></a>

Writes check result metrics and performance data to a defined
Graphite Carbon host.
This configuration object is available as [graphite feature](14-features.md#graphite-carbon-cache-writer).

Example:

```
object GraphiteWriter "graphite" {
  host = "127.0.0.1"
  port = 2003
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Optional.** Graphite Carbon host address. Defaults to `127.0.0.1`.
  port                      | Number                | **Optional.** Graphite Carbon port. Defaults to `2003`.
  host\_name\_template      | String                | **Optional.** Metric prefix for host name. Defaults to `icinga2.$host.name$.host.$host.check_command$`.
  service\_name\_template   | String                | **Optional.** Metric prefix for service name. Defaults to `icinga2.$host.name$.services.$service.name$.$service.check_command$`.
  enable\_send\_thresholds  | Boolean               | **Optional.** Send additional threshold metrics. Defaults to `false`.
  enable\_send\_metadata    | Boolean               | **Optional.** Send additional metadata metrics. Defaults to `false`.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.

Additional usage examples can be found [here](14-features.md#graphite-carbon-cache-writer).



## Host <a id="objecttype-host"></a>

A host.

Example:

```
object Host "icinga2-client1.localdomain" {
  display_name = "Linux Client 1"
  address = "192.168.56.111"
  address6 = "2a00:1450:4001:815::2003"

  groups = [ "linux-servers" ]

  check_command = "hostalive"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the host (e.g. displayed by external interfaces instead of the name if set).
  address                   | String                | **Optional.** The host's IPv4 address. Available as command runtime macro `$address$` if set.
  address6                  | String                | **Optional.** The host's IPv6 address. Available as command runtime macro `$address6$` if set.
  groups                    | Array of object names | **Optional.** A list of host groups this host belongs to.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this host.
  check\_command            | Object name           | **Required.** The name of the check command.
  max\_check\_attempts      | Number                | **Optional.** The number of times a host is re-checked before changing into a hard state. Defaults to 3.
  check\_period             | Object name           | **Optional.** The name of a time period which determines when this host should be checked. Not set by default.
  check\_timeout            | Duration              | **Optional.** Check command timeout in seconds. Overrides the CheckCommand's `timeout` attribute.
  check\_interval           | Duration              | **Optional.** The check interval (in seconds). This interval is used for checks when the host is in a `HARD` state. Defaults to `5m`.
  retry\_interval           | Duration              | **Optional.** The retry interval (in seconds). This interval is used for checks when the host is in a `SOFT` state. Defaults to `1m`. Note: This does not affect the scheduling [after a passive check result](08-advanced-topics.md#check-result-freshness).
  enable\_notifications     | Boolean               | **Optional.** Whether notifications are enabled. Defaults to true.
  enable\_active\_checks    | Boolean               | **Optional.** Whether active checks are enabled. Defaults to true.
  enable\_passive\_checks   | Boolean               | **Optional.** Whether passive checks are enabled. Defaults to true.
  enable\_event\_handler    | Boolean               | **Optional.** Enables event handlers for this host. Defaults to true.
  enable\_flapping          | Boolean               | **Optional.** Whether flap detection is enabled. Defaults to false.
  enable\_perfdata          | Boolean               | **Optional.** Whether performance data processing is enabled. Defaults to true.
  event\_command            | Object name           | **Optional.** The name of an event command that should be executed every time the host's state changes or the host is in a `SOFT` state.
  flapping\_threshold\_high | Number                | **Optional.** Flapping upper bound in percent for a host to be considered flapping. Default `30.0`
  flapping\_threshold\_low  | Number                | **Optional.** Flapping lower bound in percent for a host to be considered  not flapping. Default `25.0`
  volatile                  | Boolean               | **Optional.** Treat all state changes as HARD changes. See [here](08-advanced-topics.md#volatile-services-hosts) for details. Defaults to `false`.
  zone                      | Object name           | **Optional.** The zone this object is a member of. Please read the [distributed monitoring](06-distributed-monitoring.md#distributed-monitoring) chapter for details.
  command\_endpoint         | Object name           | **Optional.** The endpoint where commands are executed on.
  notes                     | String                | **Optional.** Notes for the host.
  notes\_url                | String                | **Optional.** URL for notes for the host (for example, in notification commands).
  action\_url               | String                | **Optional.** URL for actions for the host (for example, an external graphing tool).
  icon\_image               | String                | **Optional.** Icon image for the host. Used by external interfaces only.
  icon\_image\_alt          | String                | **Optional.** Icon image description for the host. Used by external interface only.

The actual check interval might deviate slightly from the configured values due to the fact that Icinga tries
to evenly distribute all checks over a certain period of time, i.e. to avoid load spikes.

> **Best Practice**
>
> The `address` and `address6` attributes are required for running commands using
> the `$address$` and `$address6$` runtime macros.

Runtime Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  next\_check               | Timestamp             | When the next check occurs (as a UNIX timestamp).
  last\_check               | Timestamp             | When the last check occurred (as a UNIX timestamp).
  check\_attempt            | Number                | The current check attempt number.
  state\_type               | Number                | The current state type (0 = SOFT, 1 = HARD).
  last\_state\_type         | Number                | The previous state type (0 = SOFT, 1 = HARD).
  last\_reachable           | Boolean               | Whether the host was reachable when the last check occurred.
  last\_check\_result       | CheckResult           | The current [check result](08-advanced-topics.md#advanced-value-types-checkresult).
  last\_state\_change       | Timestamp             | When the last state change occurred (as a UNIX timestamp).
  last\_hard\_state\_change | Timestamp             | When the last hard state change occurred (as a UNIX timestamp).
  last\_in\_downtime        | Boolean               | Whether the host was in a downtime when the last check occurred.
  acknowledgement           | Number                | The acknowledgement type (0 = NONE, 1 = NORMAL, 2 = STICKY).
  acknowledgement\_expiry   | Timestamp             | When the acknowledgement expires (as a UNIX timestamp; 0 = no expiry).
  downtime\_depth           | Number                | Whether the host has one or more active downtimes.
  flapping\_last\_change    | Timestamp             | When the last flapping change occurred (as a UNIX timestamp).
  flapping                  | Boolean               | Whether the host is flapping between states.
  flapping\_current         | Number                | Current flapping value in percent (see flapping\_thresholds)
  state                     | Number                | The current state (0 = UP, 1 = DOWN).
  last\_state               | Number                | The previous state (0 = UP, 1 = DOWN).
  last\_hard\_state         | Number                | The last hard state (0 = UP, 1 = DOWN).
  last\_state\_up           | Timestamp             | When the last UP state occurred (as a UNIX timestamp).
  last\_state\_down         | Timestamp             | When the last DOWN state occurred (as a UNIX timestamp).



## HostGroup <a id="objecttype-hostgroup"></a>

A group of hosts.

> **Best Practice**
>
> Assign host group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

```
object HostGroup "linux-servers" {
  display_name = "Linux Servers"

  assign where host.vars.os == "Linux"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the host group.
  groups                    | Array of object names | **Optional.** An array of nested group names.

## IcingaApplication <a id="objecttype-icingaapplication"></a>

The IcingaApplication object is required to start Icinga 2.
The object name must be `app`. If the object configuration
is missing, Icinga 2 will automatically create an IcingaApplication
object.

Example:

```
object IcingaApplication "app" {
  enable_perfdata = false
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  enable\_notifications     | Boolean               | **Optional.** Whether notifications are globally enabled. Defaults to true.
  enable\_event\_handlers   | Boolean               | **Optional.** Whether event handlers are globally enabled. Defaults to true.
  enable\_flapping          | Boolean               | **Optional.** Whether flap detection is globally enabled. Defaults to true.
  enable\_host\_checks      | Boolean               | **Optional.** Whether active host checks are globally enabled. Defaults to true.
  enable\_service\_checks   | Boolean               | **Optional.** Whether active service checks are globally enabled. Defaults to true.
  enable\_perfdata          | Boolean               | **Optional.** Whether performance data processing is globally enabled. Defaults to true.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are available globally.
  environment               | String                | **Optional.** Specify the Icinga environment. This overrides the `Environment` constant specified in the configuration or on the CLI with `--define`. Defaults to empty.

## IdoMySqlConnection <a id="objecttype-idomysqlconnection"></a>

IDO database adapter for MySQL.
This configuration object is available as [ido-mysql feature](14-features.md#db-ido).

Example:

```
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
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Optional.** MySQL database host address. Defaults to `localhost`.
  port                      | Number                | **Optional.** MySQL database port. Defaults to `3306`.
  socket\_path              | String                | **Optional.** MySQL socket path.
  user                      | String                | **Optional.** MySQL database user with read/write permission to the icinga database. Defaults to `icinga`.
  password                  | String                | **Optional.** MySQL database user's password. Defaults to `icinga`.
  database                  | String                | **Optional.** MySQL database name. Defaults to `icinga`.
  enable\_ssl               | Boolean               | **Optional.** Use SSL. Defaults to false. Change to `true` in case you want to use any of the SSL options.
  ssl\_key                  | String                | **Optional.** MySQL SSL client key file path.
  ssl\_cert                 | String                | **Optional.** MySQL SSL certificate file path.
  ssl\_ca                   | String                | **Optional.** MySQL SSL certificate authority certificate file path.
  ssl\_capath               | String                | **Optional.** MySQL SSL trusted SSL CA certificates in PEM format directory path.
  ssl\_cipher               | String                | **Optional.** MySQL SSL list of allowed ciphers.
  table\_prefix             | String                | **Optional.** MySQL database table prefix. Defaults to `icinga_`.
  instance\_name            | String                | **Optional.** Unique identifier for the local Icinga 2 instance. Defaults to `default`.
  instance\_description     | String                | **Optional.** Description for the Icinga 2 instance.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Defaults to `true`.
  failover\_timeout         | Duration              | **Optional.** Set the failover timeout in a [HA cluster](06-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Must not be lower than 60s. Defaults to `60s`.
  cleanup                   | Dictionary            | **Optional.** Dictionary with items for historical table cleanup.
  categories                | Array                 | **Optional.** Array of information types that should be written to the database.

Cleanup Items:

  Name                            | Type                  | Description
  --------------------------------|-----------------------|----------------------------------
  acknowledgements\_age           | Duration              | **Optional.** Max age for acknowledgements table rows (entry\_time). Defaults to 0 (never).
  commenthistory\_age             | Duration              | **Optional.** Max age for commenthistory table rows (entry\_time). Defaults to 0 (never).
  contactnotifications\_age       | Duration              | **Optional.** Max age for contactnotifications table rows (start\_time). Defaults to 0 (never).
  contactnotificationmethods\_age | Duration              | **Optional.** Max age for contactnotificationmethods table rows (start\_time). Defaults to 0 (never).
  downtimehistory\_age            | Duration              | **Optional.** Max age for downtimehistory table rows (entry\_time). Defaults to 0 (never).
  eventhandlers\_age              | Duration              | **Optional.** Max age for eventhandlers table rows (start\_time). Defaults to 0 (never).
  externalcommands\_age           | Duration              | **Optional.** Max age for externalcommands table rows (entry\_time). Defaults to 0 (never).
  flappinghistory\_age            | Duration              | **Optional.** Max age for flappinghistory table rows (event\_time). Defaults to 0 (never).
  hostchecks\_age                 | Duration              | **Optional.** Max age for hostalives table rows (start\_time). Defaults to 0 (never).
  logentries\_age                 | Duration              | **Optional.** Max age for logentries table rows (logentry\_time). Defaults to 0 (never).
  notifications\_age              | Duration              | **Optional.** Max age for notifications table rows (start\_time). Defaults to 0 (never).
  processevents\_age              | Duration              | **Optional.** Max age for processevents table rows (event\_time). Defaults to 0 (never).
  statehistory\_age               | Duration              | **Optional.** Max age for statehistory table rows (state\_time). Defaults to 0 (never).
  servicechecks\_age              | Duration              | **Optional.** Max age for servicechecks table rows (start\_time). Defaults to 0 (never).
  systemcommands\_age             | Duration              | **Optional.** Max age for systemcommands table rows (start\_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web 2
  DbCatState           | Current state data     | Icinga Web 2
  DbCatAcknowledgement | Acknowledgements       | Icinga Web 2
  DbCatComment         | Comments               | Icinga Web 2
  DbCatDowntime        | Downtimes              | Icinga Web 2
  DbCatEventHandler    | Event handler data     | Icinga Web 2
  DbCatExternalCommand | External commands      | --
  DbCatFlapping        | Flap detection data    | Icinga Web 2
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | --
  DbCatNotification    | Notifications          | Icinga Web 2
  DbCatProgramStatus   | Program status data    | Icinga Web 2
  DbCatRetention       | Retention data         | Icinga Web 2
  DbCatStateHistory    | Historical state data  | Icinga Web 2

The default value for `categories` includes everything required
by Icinga Web 2 in the table above.

In addition to the category flags listed above the `DbCatEverything`
flag may be used as a shortcut for listing all flags.

## IdoPgsqlConnection <a id="objecttype-idopgsqlconnection"></a>

IDO database adapter for PostgreSQL.
This configuration object is available as [ido-pgsql feature](14-features.md#db-ido).

Example:

```
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
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Optional.** PostgreSQL database host address. Defaults to `localhost`.
  port                      | Number                | **Optional.** PostgreSQL database port. Defaults to `5432`.
  user                      | String                | **Optional.** PostgreSQL database user with read/write permission to the icinga database. Defaults to `icinga`.
  password                  | String                | **Optional.** PostgreSQL database user's password. Defaults to `icinga`.
  database                  | String                | **Optional.** PostgreSQL database name. Defaults to `icinga`.
  ssl\_mode                 | String                | **Optional.** Enable SSL connection mode. Value must be set according to the [sslmode setting](https://www.postgresql.org/docs/9.3/static/libpq-connect.html#LIBPQ-CONNSTRING): `prefer`, `require`, `verify-ca`, `verify-full`, `allow`, `disable`.
  ssl\_key                  | String                | **Optional.** PostgreSQL SSL client key file path.
  ssl\_cert                 | String                | **Optional.** PostgreSQL SSL certificate file path.
  ssl\_ca                   | String                | **Optional.** PostgreSQL SSL certificate authority certificate file path.
  table\_prefix             | String                | **Optional.** PostgreSQL database table prefix. Defaults to `icinga_`.
  instance\_name            | String                | **Optional.** Unique identifier for the local Icinga 2 instance. Defaults to `default`.
  instance\_description     | String                | **Optional.** Description for the Icinga 2 instance.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Defaults to "true".
  failover\_timeout         | Duration              | **Optional.** Set the failover timeout in a [HA cluster](06-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido). Must not be lower than 60s. Defaults to `60s`.
  cleanup                   | Dictionary            | **Optional.** Dictionary with items for historical table cleanup.
  categories                | Array                 | **Optional.** Array of information types that should be written to the database.

Cleanup Items:

  Name                            | Type                  | Description
  --------------------------------|-----------------------|----------------------------------
  acknowledgements\_age           | Duration              | **Optional.** Max age for acknowledgements table rows (entry\_time). Defaults to 0 (never).
  commenthistory\_age             | Duration              | **Optional.** Max age for commenthistory table rows (entry\_time). Defaults to 0 (never).
  contactnotifications\_age       | Duration              | **Optional.** Max age for contactnotifications table rows (start\_time). Defaults to 0 (never).
  contactnotificationmethods\_age | Duration              | **Optional.** Max age for contactnotificationmethods table rows (start\_time). Defaults to 0 (never).
  downtimehistory\_age            | Duration              | **Optional.** Max age for downtimehistory table rows (entry\_time). Defaults to 0 (never).
  eventhandlers\_age              | Duration              | **Optional.** Max age for eventhandlers table rows (start\_time). Defaults to 0 (never).
  externalcommands\_age           | Duration              | **Optional.** Max age for externalcommands table rows (entry\_time). Defaults to 0 (never).
  flappinghistory\_age            | Duration              | **Optional.** Max age for flappinghistory table rows (event\_time). Defaults to 0 (never).
  hostchecks\_age                 | Duration              | **Optional.** Max age for hostalives table rows (start\_time). Defaults to 0 (never).
  logentries\_age                 | Duration              | **Optional.** Max age for logentries table rows (logentry\_time). Defaults to 0 (never).
  notifications\_age              | Duration              | **Optional.** Max age for notifications table rows (start\_time). Defaults to 0 (never).
  processevents\_age              | Duration              | **Optional.** Max age for processevents table rows (event\_time). Defaults to 0 (never).
  statehistory\_age               | Duration              | **Optional.** Max age for statehistory table rows (state\_time). Defaults to 0 (never).
  servicechecks\_age              | Duration              | **Optional.** Max age for servicechecks table rows (start\_time). Defaults to 0 (never).
  systemcommands\_age             | Duration              | **Optional.** Max age for systemcommands table rows (start\_time). Defaults to 0 (never).

Data Categories:

  Name                 | Description            | Required by
  ---------------------|------------------------|--------------------
  DbCatConfig          | Configuration data     | Icinga Web 2
  DbCatState           | Current state data     | Icinga Web 2
  DbCatAcknowledgement | Acknowledgements       | Icinga Web 2
  DbCatComment         | Comments               | Icinga Web 2
  DbCatDowntime        | Downtimes              | Icinga Web 2
  DbCatEventHandler    | Event handler data     | Icinga Web 2
  DbCatExternalCommand | External commands      | --
  DbCatFlapping        | Flap detection data    | Icinga Web 2
  DbCatCheck           | Check results          | --
  DbCatLog             | Log messages           | --
  DbCatNotification    | Notifications          | Icinga Web 2
  DbCatProgramStatus   | Program status data    | Icinga Web 2
  DbCatRetention       | Retention data         | Icinga Web 2
  DbCatStateHistory    | Historical state data  | Icinga Web 2

The default value for `categories` includes everything required
by Icinga Web 2 in the table above.

In addition to the category flags listed above the `DbCatEverything`
flag may be used as a shortcut for listing all flags.

## InfluxdbWriter <a id="objecttype-influxdbwriter"></a>

Writes check result metrics and performance data to a defined InfluxDB host.
This configuration object is available as [influxdb feature](14-features.md#influxdb-writer).

Example:

```
object InfluxdbWriter "influxdb" {
  host = "127.0.0.1"
  port = 8086
  database = "icinga2"

  flush_threshold = 1024
  flush_interval = 10s

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
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host                      | String                | **Required.** InfluxDB host address. Defaults to `127.0.0.1`.
  port                      | Number                | **Required.** InfluxDB HTTP port. Defaults to `8086`.
  database                  | String                | **Required.** InfluxDB database name. Defaults to `icinga2`.
  username                  | String                | **Optional.** InfluxDB user name. Defaults to `none`.
  password                  | String                | **Optional.** InfluxDB user password.  Defaults to `none`.
  ssl\_enable               | Boolean               | **Optional.** Whether to use a TLS stream. Defaults to `false`.
  ssl\_ca\_cert             | String                | **Optional.** Path to CA certificate to validate the remote host.
  ssl\_cert                 | String                | **Optional.** Path to host certificate to present to the remote host for mutual verification.
  ssl\_key                  | String                | **Optional.** Path to host key to accompany the ssl\_cert.
  host\_template            | String                | **Required.** Host template to define the InfluxDB line protocol.
  service\_template         | String                | **Required.** Service template to define the influxDB line protocol.
  enable\_send\_thresholds  | Boolean               | **Optional.** Whether to send warn, crit, min & max tagged data.
  enable\_send\_metadata    | Boolean               | **Optional.** Whether to send check metadata e.g. states, execution time, latency etc.
  flush\_interval           | Duration              | **Optional.** How long to buffer data points before transferring to InfluxDB. Defaults to `10s`.
  flush\_threshold          | Number                | **Optional.** How many data points to buffer before forcing a transfer to InfluxDB.  Defaults to `1024`.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.

Note: If `flush_threshold` is set too low, this will always force the feature to flush all data
to InfluxDB. Experiment with the setting, if you are processing more than 1024 metrics per second
or similar.



## LiveStatusListener <a id="objecttype-livestatuslistener"></a>

Livestatus API interface available as TCP or UNIX socket. Historical table queries
require the [CompatLogger](09-object-types.md#objecttype-compatlogger) feature enabled
pointing to the log files using the `compat_log_path` configuration attribute.
This configuration object is available as [livestatus feature](14-features.md#setting-up-livestatus).

Examples:

```
object LivestatusListener "livestatus-tcp" {
  socket_type = "tcp"
  bind_host = "127.0.0.1"
  bind_port = "6558"
}

object LivestatusListener "livestatus-unix" {
  socket_type = "unix"
  socket_path = "/var/run/icinga2/cmd/livestatus"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  socket\_type              | String                | **Optional.** Specifies the socket type. Can be either `tcp` or `unix`. Defaults to `unix`.
  bind\_host                | String                | **Optional.** Only valid when `socket_type` is set to `tcp`. Host address to listen on for connections. Defaults to `127.0.0.1`.
  bind\_port                | Number                | **Optional.** Only valid when `socket_type` is set to `tcp`. Port to listen on for connections. Defaults to `6558`.
  socket\_path              | String                | **Optional.** Only valid when `socket_type` is set to `unix`. Specifies the path to the UNIX socket file. Defaults to RunDir + "/icinga2/cmd/livestatus".
  compat\_log\_path         | String                | **Optional.** Path to Icinga 1.x log files. Required for historical table queries. Requires `CompatLogger` feature enabled. Defaults to LogDir + "/compat"

> **Note**
>
> UNIX sockets are not supported on Windows.


## Notification <a id="objecttype-notification"></a>

Notification objects are used to specify how users should be notified in case
of host and service state changes and other events.

> **Best Practice**
>
> Rather than creating a `Notification` object for a specific host or service it is
> usually easier to just create a `Notification` template and use the `apply` keyword
> to assign the notification to a number of hosts or services. Use the `to` keyword
> to set the specific target type for `Host` or `Service`.
> Check the [notifications](03-monitoring-basics.md#alert-notifications) chapter for detailed examples.

Example:

```
object Notification "localhost-ping-notification" {
  host_name = "localhost"
  service_name = "ping4"

  command = "mail-notification"

  users = [ "user1", "user2" ] // reference to User objects

  types = [ Problem, Recovery ]
  states = [ Critical, Warning, OK ]
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host\_name                | Object name           | **Required.** The name of the host this notification belongs to.
  service\_name             | Object name           | **Optional.** The short name of the service this notification belongs to. If omitted, this notification object is treated as host notification.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this notification object.
  users                     | Array of object names | **Required.** A list of user names who should be notified. **Optional.** if the `user_groups` attribute is set.
  user\_groups              | Array of object names | **Required.** A list of user group names who should be notified. **Optional.** if the `users` attribute is set.
  times                     | Dictionary            | **Optional.** A dictionary containing `begin` and `end` attributes for the notification.
  command                   | Object name           | **Required.** The name of the notification command which should be executed when the notification is triggered.
  interval                  | Duration              | **Optional.** The notification interval (in seconds). This interval is used for active notifications. Defaults to 30 minutes. If set to 0, [re-notifications](03-monitoring-basics.md#disable-renotification) are disabled.
  period                    | Object name           | **Optional.** The name of a time period which determines when this notification should be triggered. Not set by default.
  zone		            | Object name           | **Optional.** The zone this object is a member of. Please read the [distributed monitoring](06-distributed-monitoring.md#distributed-monitoring) chapter for details.
  types                     | Array                 | **Optional.** A list of type filters when this notification should be triggered. By default everything is matched.
  states                    | Array                 | **Optional.** A list of state filters when this notification should be triggered. By default everything is matched. Note that the states filter is ignored for notifications of type Acknowledgement!

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

  Name                        | Type                  | Description
  ----------------------------|-----------------------|-----------------
  last\_notification          | Timestamp             | When the last notification was sent for this Notification object (as a UNIX timestamp).
  next\_notification          | Timestamp             | When the next notification is going to be sent for this assuming the associated host/service is still in a non-OK state (as a UNIX timestamp).
  notification\_number        | Number                | The notification number.
  last\_problem\_notification | Timestamp             | When the last notification was sent for a problem (as a UNIX timestamp).


## NotificationCommand <a id="objecttype-notificationcommand"></a>

A notification command definition.

Example:

```
object NotificationCommand "mail-service-notification" {
  command = [ ConfigDir + "/scripts/mail-service-notification.sh" ]

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
    "-f" = {
      value = "$notification_from$"
      description = "Set from address. Requires GNU mailutils (Debian/Ubuntu) or mailx (RHEL/SUSE)"
    }
    "-i" = "$notification_icingaweb2url$"
    "-l" = {
      required = true
      value = "$notification_hostname$"
    }
    "-n" = {
      required = true
      value = "$notification_hostdisplayname$"
    }
    "-o" = {
      required = true
      value = "$notification_serviceoutput$"
    }
    "-r" = {
      required = true
      value = "$notification_useremail$"
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
    "-v" = "$notification_logtosyslog$"
  }

  vars += {
    notification_address = "$address$"
    notification_address6 = "$address6$"
    notification_author = "$notification.author$"
    notification_comment = "$notification.comment$"
    notification_type = "$notification.type$"
    notification_date = "$icinga.long_date_time$"
    notification_hostname = "$host.name$"
    notification_hostdisplayname = "$host.display_name$"
    notification_servicename = "$service.name$"
    notification_serviceoutput = "$service.output$"
    notification_servicestate = "$service.state$"
    notification_useremail = "$user.email$"
    notification_servicedisplayname = "$service.display_name$"
  }
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  command                   | Array                 | **Required.** The command. This can either be an array of individual command arguments. Alternatively a string can be specified in which case the shell interpreter (usually /bin/sh) takes care of parsing the command. When using the "arguments" attribute this must be an array. Can be specified as function for advanced implementations.
  env                       | Dictionary            | **Optional.** A dictionary of macros which should be exported as environment variables prior to executing the command.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this command.
  timeout                   | Duration              | **Optional.** The command timeout in seconds. Defaults to `1m`.
  arguments                 | Dictionary            | **Optional.** A dictionary of command arguments.

Command arguments can be used the same way as for [CheckCommand objects](09-object-types.md#objecttype-checkcommand-arguments).

More details on specific attributes can be found in [this chapter](03-monitoring-basics.md#notification-commands).

## NotificationComponent <a id="objecttype-notificationcomponent"></a>

The notification component is responsible for sending notifications.
This configuration object is available as [notification feature](11-cli-commands.md#cli-command-feature).

Example:

```
object NotificationComponent "notification" { }
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-notifications). Disabling this currently only affects reminder notifications. Defaults to "true".

## OpenTsdbWriter <a id="objecttype-opentsdbwriter"></a>

Writes check result metrics and performance data to [OpenTSDB](http://opentsdb.net).
This configuration object is available as [opentsdb feature](14-features.md#opentsdb-writer).

Example:

```
object OpenTsdbWriter "opentsdb" {
  host = "127.0.0.1"
  port = 4242
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host            	    | String                | **Optional.** OpenTSDB host address. Defaults to `127.0.0.1`.
  port            	    | Number                | **Optional.** OpenTSDB port. Defaults to `4242`.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.


## PerfdataWriter <a id="objecttype-perfdatawriter"></a>

Writes check result performance data to a defined path using macro
pattern consisting of custom attributes and runtime macros.
This configuration object is available as [perfdata feature](14-features.md#writing-performance-data-files).

Example:

```
object PerfdataWriter "perfdata" {
  host_perfdata_path = "/var/spool/icinga2/perfdata/host-perfdata"

  service_perfdata_path = "/var/spool/icinga2/perfdata/service-perfdata"

  host_format_template = "DATATYPE::HOSTPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tHOSTPERFDATA::$host.perfdata$\tHOSTCHECKCOMMAND::$host.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$"
  service_format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$icinga.timet$\tHOSTNAME::$host.name$\tSERVICEDESC::$service.name$\tSERVICEPERFDATA::$service.perfdata$\tSERVICECHECKCOMMAND::$service.check_command$\tHOSTSTATE::$host.state$\tHOSTSTATETYPE::$host.state_type$\tSERVICESTATE::$service.state$\tSERVICESTATETYPE::$service.state_type$"

  rotation_interval = 15s
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host\_perfdata\_path      | String                | **Optional.** Path to the host performance data file. Defaults to SpoolDir + "/perfdata/host-perfdata".
  service\_perfdata\_path   | String                | **Optional.** Path to the service performance data file. Defaults to SpoolDir + "/perfdata/service-perfdata".
  host\_temp\_path          | String                | **Optional.** Path to the temporary host file. Defaults to SpoolDir + "/tmp/host-perfdata".
  service\_temp\_path       | String                | **Optional.** Path to the temporary service file. Defaults to SpoolDir + "/tmp/service-perfdata".
  host\_format\_template    | String                | **Optional.** Host Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  service\_format\_template | String                | **Optional.** Service Format template for the performance data file. Defaults to a template that's suitable for use with PNP4Nagios.
  rotation\_interval        | Duration              | **Optional.** Rotation interval for the files specified in `{host,service}_perfdata_path`. Defaults to `30s`.
  enable\_ha                | Boolean               | **Optional.** Enable the high availability functionality. Only valid in a [cluster setup](06-distributed-monitoring.md#distributed-monitoring-high-availability-features). Defaults to `true`.

When rotating the performance data file the current UNIX timestamp is appended to the path specified
in `host_perfdata_path` and `service_perfdata_path` to generate a unique filename.


## ScheduledDowntime <a id="objecttype-scheduleddowntime"></a>

ScheduledDowntime objects can be used to set up recurring downtimes for hosts/services.

> **Best Practice**
>
> Rather than creating a `ScheduledDowntime` object for a specific host or service it is usually easier
> to just create a `ScheduledDowntime` template and use the `apply` keyword to assign the
> scheduled downtime to a number of hosts or services. Use the `to` keyword to set the specific target
> type for `Host` or `Service`.
> Check the [recurring downtimes](08-advanced-topics.md#recurring-downtimes) example for details.

Example:

```
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
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  host\_name                | Object name           | **Required.** The name of the host this scheduled downtime belongs to.
  service\_name             | Object name           | **Optional.** The short name of the service this scheduled downtime belongs to. If omitted, this downtime object is treated as host downtime.
  author                    | String                | **Required.** The author of the downtime.
  comment                   | String                | **Required.** A comment for the downtime.
  fixed                     | Boolean               | **Optional.** Whether this is a fixed downtime. Defaults to `true`.
  duration                  | Duration              | **Optional.** How long the downtime lasts. Only has an effect for flexible (non-fixed) downtimes.
  ranges                    | Dictionary            | **Required.** A dictionary containing information which days and durations apply to this timeperiod.
  child\_options            | String                | **Optional.** Schedule child downtimes. `DowntimeNoChildren` does not do anything, `DowntimeTriggeredChildren` schedules child downtimes triggered by this downtime, `DowntimeNonTriggeredChildren` schedules non-triggered downtimes. Defaults to `DowntimeNoChildren`.

ScheduledDowntime objects have composite names, i.e. their names are based
on the `host_name` and `service_name` attributes and the
name you specified. This means you can define more than one object
with the same (short) name as long as one of the `host_name` and
`service_name` attributes has a different value.


## Service <a id="objecttype-service"></a>

Service objects describe network services and how they should be checked
by Icinga 2.

> **Best Practice**
>
> Rather than creating a `Service` object for a specific host it is usually easier
> to just create a `Service` template and use the `apply` keyword to assign the
> service to a number of hosts.
> Check the [apply](03-monitoring-basics.md#using-apply) chapter for details.

Example:

```
object Service "uptime" {
  host_name = "localhost"

  display_name = "localhost Uptime"

  check_command = "snmp"

  vars.snmp_community = "public"
  vars.snmp_oid = "DISMAN-EVENT-MIB::sysUpTimeInstance"

  check_interval = 60s
  retry_interval = 15s

  groups = [ "all-services", "snmp" ]
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the service.
  host\_name                | Object name           | **Required.** The host this service belongs to. There must be a `Host` object with that name.
  groups                    | Array of object names | **Optional.** The service groups this service belongs to.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this service.
  check\_command            | Object name           | **Required.** The name of the check command.
  max\_check\_attempts      | Number                | **Optional.** The number of times a service is re-checked before changing into a hard state. Defaults to 3.
  check\_period             | Object name           | **Optional.** The name of a time period which determines when this service should be checked. Not set by default.
  check\_timeout            | Duration              | **Optional.** Check command timeout in seconds. Overrides the CheckCommand's `timeout` attribute.
  check\_interval           | Duration              | **Optional.** The check interval (in seconds). This interval is used for checks when the service is in a `HARD` state. Defaults to `5m`.
  retry\_interval           | Duration              | **Optional.** The retry interval (in seconds). This interval is used for checks when the service is in a `SOFT` state. Defaults to `1m`. Note: This does not affect the scheduling [after a passive check result](08-advanced-topics.md#check-result-freshness).
  enable\_notifications     | Boolean               | **Optional.** Whether notifications are enabled. Defaults to `true`.
  enable\_active\_checks    | Boolean               | **Optional.** Whether active checks are enabled. Defaults to `true`.
  enable\_passive\_checks   | Boolean               | **Optional.** Whether passive checks are enabled. Defaults to `true`.
  enable\_event\_handler    | Boolean               | **Optional.** Enables event handlers for this host. Defaults to `true`.
  enable\_flapping          | Boolean               | **Optional.** Whether flap detection is enabled. Defaults to `false`.
  flapping\_threshold\_high | Number                | **Optional.** Flapping upper bound in percent for a service to be considered flapping. `30.0`
  flapping\_threshold\_low  | Number                | **Optional.** Flapping lower bound in percent for a service to be considered  not flapping. `25.0`
  enable\_perfdata          | Boolean               | **Optional.** Whether performance data processing is enabled. Defaults to `true`.
  event\_command            | Object name           | **Optional.** The name of an event command that should be executed every time the service's state changes or the service is in a `SOFT` state.
  volatile                  | Boolean               | **Optional.** Treat all state changes as HARD changes. See [here](08-advanced-topics.md#volatile-services-hosts) for details. Defaults to `false`.
  zone                      | Object name           | **Optional.** The zone this object is a member of. Please read the [distributed monitoring](06-distributed-monitoring.md#distributed-monitoring) chapter for details.
  name                      | String                | **Required.** The service name. Must be unique on a per-host basis. For advanced usage in [apply rules](03-monitoring-basics.md#using-apply) only.
  command\_endpoint         | Object name           | **Optional.** The endpoint where commands are executed on.
  notes                     | String                | **Optional.** Notes for the service.
  notes\_url                | String                | **Optional.** URL for notes for the service (for example, in notification commands).
  action\_url               | String                | **Optional.** URL for actions for the service (for example, an external graphing tool).
  icon\_image               | String                | **Optional.** Icon image for the service. Used by external interfaces only.
  icon\_image\_alt          | String                | **Optional.** Icon image description for the service. Used by external interface only.

Service objects have composite names, i.e. their names are based on the host\_name attribute and the name you specified. This means
you can define more than one object with the same (short) name as long as the `host_name` attribute has a different value.

The actual check interval might deviate slightly from the configured values due to the fact that Icinga tries
to evenly distribute all checks over a certain period of time, i.e. to avoid load spikes.

Runtime Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  next\_check               | Timestamp             | When the next check occurs (as a UNIX timestamp).
  last\_check               | Timestamp             | When the last check occurred (as a UNIX timestamp).
  check\_attempt            | Number                | The current check attempt number.
  state\_type               | Number                | The current state type (0 = SOFT, 1 = HARD).
  last\_state\_type         | Number                | The previous state type (0 = SOFT, 1 = HARD).
  last\_reachable           | Boolean               | Whether the service was reachable when the last check occurred.
  last\_check\_result       | CheckResult           | The current [check result](08-advanced-topics.md#advanced-value-types-checkresult).
  last\_state\_change       | Timestamp             | When the last state change occurred (as a UNIX timestamp).
  last\_hard\_state\_change | Timestamp             | When the last hard state change occurred (as a UNIX timestamp).
  last\_in\_downtime        | Boolean               | Whether the service was in a downtime when the last check occurred.
  acknowledgement           | Number                | The acknowledgement type (0 = NONE, 1 = NORMAL, 2 = STICKY).
  acknowledgement\_expiry   | Timestamp             | When the acknowledgement expires (as a UNIX timestamp; 0 = no expiry).
  downtime\_depth           | Number                | Whether the service has one or more active downtimes.
  flapping\_last\_change    | Timestamp             | When the last flapping change occurred (as a UNIX timestamp).
  flapping\_current         | Number                | Current flapping value in percent (see flapping\_thresholds)
  flapping                  | Boolean               | Whether the host is flapping between states.
  state                     | Number                | The current state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last\_state               | Number                | The previous state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last\_hard\_state         | Number                | The last hard state (0 = OK, 1 = WARNING, 2 = CRITICAL, 3 = UNKNOWN).
  last\_state\_ok           | Timestamp             | When the last OK state occurred (as a UNIX timestamp).
  last\_state\_warning      | Timestamp             | When the last WARNING state occurred (as a UNIX timestamp).
  last\_state\_critical     | Timestamp             | When the last CRITICAL state occurred (as a UNIX timestamp).
  last\_state\_unknown      | Timestamp             | When the last UNKNOWN state occurred (as a UNIX timestamp).


## ServiceGroup <a id="objecttype-servicegroup"></a>

A group of services.

> **Best Practice**
>
> Assign service group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

```
object ServiceGroup "snmp" {
  display_name = "SNMP services"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the service group.
  groups                    | Array of object names | **Optional.** An array of nested group names.


## StatusDataWriter <a id="objecttype-statusdatawriter"></a>

Periodically writes status and configuration data files which are used by third-party tools.
This configuration object is available as [statusdata feature](14-features.md#status-data).

Example:

```
object StatusDataWriter "status" {
    status_path = "/var/cache/icinga2/status.dat"
    objects_path = "/var/cache/icinga2/objects.cache"
    update_interval = 30s
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  status\_path              | String                | **Optional.** Path to the `status.dat` file. Defaults to CacheDir + "/status.dat".
  objects\_path             | String                | **Optional.** Path to the `objects.cache` file. Defaults to CacheDir + "/objects.cache".
  update\_interval          | Duration              | **Optional.** The interval in which the status files are updated. Defaults to `15s`.


## SyslogLogger <a id="objecttype-sysloglogger"></a>

Specifies Icinga 2 logging to syslog.
This configuration object is available as `syslog` [logging feature](14-features.md#logging).

Example:

```
object SyslogLogger "syslog" {
  severity = "warning"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  severity                  | String                | **Optional.** The minimum severity for this log. Can be "debug", "notice", "information", "warning" or "critical". Defaults to "warning".
  facility                  | String                | **Optional.** Defines the facility to use for syslog entries. This can be a facility constant like `FacilityDaemon`. Defaults to `FacilityUser`.

Facility Constants:

  Name                 | Facility      | Description
  ---------------------|---------------|----------------
  FacilityAuth         | LOG\_AUTH     | The authorization system.
  FacilityAuthPriv     | LOG\_AUTHPRIV | The same as `FacilityAuth`, but logged to a file readable only by selected individuals.
  FacilityCron         | LOG\_CRON     | The cron daemon.
  FacilityDaemon       | LOG\_DAEMON   | System daemons that are not provided for explicitly by other facilities.
  FacilityFtp          | LOG\_FTP      | The file transfer protocol daemons.
  FacilityKern         | LOG\_KERN     | Messages generated by the kernel. These cannot be generated by any user processes.
  FacilityLocal0       | LOG\_LOCAL0   | Reserved for local use.
  FacilityLocal1       | LOG\_LOCAL1   | Reserved for local use.
  FacilityLocal2       | LOG\_LOCAL2   | Reserved for local use.
  FacilityLocal3       | LOG\_LOCAL3   | Reserved for local use.
  FacilityLocal4       | LOG\_LOCAL4   | Reserved for local use.
  FacilityLocal5       | LOG\_LOCAL5   | Reserved for local use.
  FacilityLocal6       | LOG\_LOCAL6   | Reserved for local use.
  FacilityLocal7       | LOG\_LOCAL7   | Reserved for local use.
  FacilityLpr          | LOG\_LPR      | The line printer spooling system.
  FacilityMail         | LOG\_MAIL     | The mail system.
  FacilityNews         | LOG\_NEWS     | The network news system.
  FacilitySyslog       | LOG\_SYSLOG   | Messages generated internally by syslogd.
  FacilityUser         | LOG\_USER     | Messages generated by user processes. This is the default facility identifier if none is specified.
  FacilityUucp         | LOG\_UUCP     | The UUCP system.

## TimePeriod <a id="objecttype-timeperiod"></a>

Time periods can be used to specify when hosts/services should be checked or to limit
when notifications should be sent out.

Examples:

```
object TimePeriod "nonworkhours" {
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
```

Additional examples can be found [here](08-advanced-topics.md#timeperiods).

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the time period.
  ranges                    | Dictionary            | **Required.** A dictionary containing information which days and durations apply to this timeperiod.
  prefer\_includes          | Boolean               | **Optional.** Whether to prefer timeperiods `includes` or `excludes`. Default to true.
  excludes                  | Array of object names | **Optional.** An array of timeperiods, which should exclude from your timerange.
  includes                  | Array of object names | **Optional.** An array of timeperiods, which should include into your timerange


Runtime Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  is\_inside                | Boolean               | Whether we're currently inside this timeperiod.


## User <a id="objecttype-user"></a>

A user.

Example:

```
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
```

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

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the user.
  email                     | String                | **Optional.** An email string for this user. Useful for notification commands.
  pager                     | String                | **Optional.** A pager string for this user. Useful for notification commands.
  vars                      | Dictionary            | **Optional.** A dictionary containing custom attributes that are specific to this user.
  groups                    | Array of object names | **Optional.** An array of group names.
  enable\_notifications     | Boolean               | **Optional.** Whether notifications are enabled for this user. Defaults to true.
  period                    | Object name           | **Optional.** The name of a time period which determines when a notification for this user should be triggered. Not set by default.
  types                     | Array                 | **Optional.** A set of type filters when a notification for this user should be triggered. By default everything is matched.
  states                    | Array                 | **Optional.** A set of state filters when a notification for this should be triggered. By default everything is matched.

Runtime Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  last\_notification        | Timestamp             | When the last notification was sent for this user (as a UNIX timestamp).

## UserGroup <a id="objecttype-usergroup"></a>

A user group.

> **Best Practice**
>
> Assign user group members using the [group assign](17-language-reference.md#group-assign) rules.

Example:

```
object UserGroup "icingaadmins" {
    display_name = "Icinga 2 Admin Group"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  display\_name             | String                | **Optional.** A short description of the user group.
  groups                    | Array of object names | **Optional.** An array of nested group names.


## Zone <a id="objecttype-zone"></a>

Zone objects are used to specify which Icinga 2 instances are located in a zone.
Please read the [distributed monitoring chapter](06-distributed-monitoring.md#distributed-monitoring) for additional details.
Example:

```
object Zone "master" {
  endpoints = [ "icinga2-master1.localdomain", "icinga2-master2.localdomain" ]

}

object Zone "satellite" {
  endpoints = [ "icinga2-satellite1.localdomain" ]
  parent = "master"
}
```

Configuration Attributes:

  Name                      | Type                  | Description
  --------------------------|-----------------------|----------------------------------
  endpoints                 | Array of object names | **Optional.** Array of endpoint names located in this zone.
  parent                    | Object name           | **Optional.** The name of the parent zone. (Do not specify a global zone)
  global                    | Boolean               | **Optional.** Whether configuration files for this zone should be [synced](06-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync) to all endpoints. Defaults to `false`.

Zone objects cannot currently be created with the API.
