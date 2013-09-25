Configuration Format
====================

Object Definition
-----------------

Icinga 2 features an object-based configuration format. In order to
define objects the "object" keyword is used:

    object Host "host1.example.org" {
      alias = "host1",

      check_interval = 30,
      retry_interval = 15,

      macros = {
        address = "192.168.0.1"
      }
    }

> **Note**
>
> The Icinga 2 configuration format is agnostic to whitespaces and
> new-lines.

Each object is uniquely identified by its type ("Host") and name
("host1.example.org"). Objects can contain a comma-separated list of
property declarations. The following data types are available for
property values:

### Numeric Literals

A floating-point number.

Example:

    -27.3

### Duration Literal

Similar to floating-point numbers except for that fact that they support
suffixes to help with specifying time durations.

Example:

    2.5m

Supported suffixes include ms (milliseconds), s (seconds), m (minutes)
and h (hours).

### String Literals

A string. No escape characters are supported at present though this will
likely change.

Example:

    "Hello World!"

### Expression List

A list of expressions that when executed has a dictionary as a result.

Example:

    {
      address = "192.168.0.1",
      port = 443
    }

> **Note**
>
> Identifiers may not contain certain characters (e.g. space) or start
> with certain characters (e.g. digits). If you want to use a dictionary
> key that is not a valid identifier you can put the key in double
> quotes.

Operators
---------

In addition to the "=" operator shown above a number of other operators
to manipulate configuration objects are supported. Here’s a list of all
available operators:

### Operator "="

Sets a dictionary element to the specified value.

Example:

    {
      a = 5,
      a = 7
    }

In this example a has the value 7 after both instructions are executed.

### Operator "+="

Modifies a dictionary by adding new elements to it.

Example:

    {
      a = { "hello" },
      a += { "world" }
    }

In this example a contains both "hello" and "world". This currently only
works for expression lists. Support for numbers might be added later on.

### Operator "-="

Removes elements from a dictionary.

Example:

    {
      a = { "hello", "world" },
      a -= { "world" }
    }

In this example a contains "hello". Trying to remove an item that does
not exist is not an error. Not implemented yet.

### Operator "\*="

Multiplies an existing dictionary element with the specified number. If
the dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 60,
      a *= 5
    }

In this example a is 300. This only works for numbers. Not implemented
yet.

### Operator "/="

Divides an existing dictionary element by the specified number. If the
dictionary element does not already exist 0 is used as its value.

Example:

    {
      a = 300,
      a /= 5
    }

In this example a is 60. This only works for numbers. Not implemented
yet.

Attribute Shortcuts
-------------------

### Value Shortcut

Example:

    {
      "hello", "world"
    }

This is equivalent to writing:

    {
      _00000001 = "hello", _00000002 = "world"
    }

The item’s keys are monotonically increasing and the config compiler
takes care of ensuring that all keys are unique (even when adding items
to an existing attribute using +=).

### Indexer Shortcut

Example:

    {
      hello["key"] = "world"
    }

This is equivalent to writing:

    {
      hello += {
        key = "world"
      }
    }

Specifiers
----------

Objects can have specifiers that have special meaning. The following
specifiers can be used (before the "object" keyword):

### Specifier "abstract"

This specifier identifies the object as a template which can be used by
other object definitions. The object will not be instantiated on its
own.

Instead of using the "abstract" specifier you can use the "template"
keyword which is a shorthand for writing "abstract object":

    template Service "http" {
      ...
    }

### Specifier "local"

This specifier disables replication for this object. The object will not
be sent to remote Icinga instances.

Inheritance
-----------

Objects can inherit attributes from one or more other objects.

Example:

    abstract object Host "default-host" {
      check_interval = 30,

      macros = {
        color = "red"
      }
    }

    abstract object Host "test-host" inherits "default-host" {
      macros += {
        color = "blue"
      }
    }

    object Host "localhost" inherits "test-host" {
      macros += {
        address = "127.0.0.1",
        address6 = "::1"
      }
    }

> **Note**
>
> The "default-host" and "test-host" objects are marked as templates
> using the "abstract" keyword. Parent objects do not necessarily have
> to be "abstract" though in general they are.

> **Note**
>
> The += operator is used to insert additional properties into the
> macros dictionary. The final dictionary contains all 3 macros and the
> property "color" has the value "blue".

Parent objects are resolved in the order they’re specified using the
"inherits" keyword. Parent objects must already be defined by the time
they’re used in an object definition.

Comments
--------

The Icinga 2 configuration format supports C/C++-style comments.

Example:

    /*
     This is a comment.
     */
    object Host "localhost" {
      check_interval = 30, // this is also a comment.
      retry_interval = 15
    }

Includes
--------

Other configuration files can be included using the "\#include"
directive. Paths must be relative to the configuration file that
contains the "\#include" keyword:

Example:

    #include "some/other/file.conf"
    #include "conf.d/*.conf"

Icinga also supports include search paths similar to how they work in a
C/C++ compiler:

    #include <itl/itl.conf>

Note the use of angle brackets instead of double quotes. This causes the
config compiler to search the include search paths for the specified
file. By default \$PREFIX/icinga2 is included in the list of search
paths.

Wildcards are not permitted when using angle brackets.

Library directive
-----------------

The "\#library" directive can be used to manually load additional
libraries. Upon loading these libraries may provide additional classes
or methods.

Example:

    #library "snmphelper"

> **Note**
>
> The "icinga" library is automatically loaded by Icinga.

Type Definition
---------------

By default Icinga has no way of semantically verifying its configuration
objects. This is where type definitions come in. Using type definitions
you can specify which attributes are allowed in an object definition.

Example:

    type Pizza {
            %require "radius",
            %attribute number "radius",

            %attribute dictionary "ingredients" {
                    %validator "ValidateIngredients",

                    %attribute string "*",

                    %attribute dictionary "*" {
                            %attribute number "quantity",
                            %attribute string "name"
                    }
            },

            %attribute any "custom::*"
    }

The Pizza definition provides the following validation rules:

-   Pizza objects must contain an attribute "radius" which has to be a
    number.

-   Pizza objects may contain an attribute "ingredients" which has to be
    a dictionary.

-   Elements in the ingredients dictionary can be either a string or a
    dictionary.

-   If they’re a dictionary they may contain attributes "quantity" (of
    type number) and "name" (of type string).

-   The script function "ValidateIngredients" is run to perform further
    validation of the ingredients dictionary.

-   Pizza objects may contain attribute matching the pattern
    "custom::\*" of any type.

Valid types for type rules include: \* any \* number \* string \* scalar
(an alias for string) \* dictionary

Configuration Objects
=====================

Type: IcingaApplication
-----------------------

The "IcingaApplication" type is used to specify global configuration
parameters for Icinga. There must be exactly one application object in
each Icinga 2 configuration. The object must have the "local" specifier.

Example:

    local object IcingaApplication "icinga" {
      cert_path = "my-cert.pem",
      ca_path = "ca.crt",

      node = "192.168.0.1",
      service = 7777,

      pid_path = "/var/run/icinga2.pid",
      state_path = "/var/lib/icinga2/icinga2.state",

      macros = {
        plugindir = "/usr/local/icinga/libexec"
      }
    }

### Attribute: cert\_path

This is used to specify the SSL client certificate Icinga 2 will use
when connecting to other Icinga 2 instances. This property is optional
when you’re setting up a non-networked Icinga 2 instance.

### Attribute: ca\_path

This is the public CA certificate that is used to verify connections
from other Icinga 2 instances. This property is optional when you’re
setting up a non-networked Icinga 2 instance.

### Attribute: node

The externally visible IP address that is used by other Icinga 2
instances to connect to this instance. This property is optional when
you’re setting up a non-networked Icinga 2 instance.

> **Note**
>
> Icinga does not bind to this IP address.

### Attribute: service

The port this Icinga 2 instance should listen on. This property is
optional when you’re setting up a non-networked Icinga 2 instance.

### Attribute: pid\_path

Optional. The path to the PID file. Defaults to "icinga.pid" in the
current working directory.

### Attribute: state\_path

Optional. The path of the state file. This is the file Icinga 2 uses to
persist objects between program runs. Defaults to "icinga2.state" in the
current working directory.

### Attribute: macros

Optional. Global macros that are used for service checks and
notifications.

Type: Component
---------------

Icinga 2 uses a number of components to implement its feature-set. The
"Component" configuration object is used to load these components and
specify additional parameters for them. "Component" objects must have
the "local" specifier. The typical components to be loaded in the
default configuration would be "checker", "delegation" and more.

Example "compat":

    local object Component "compat" {
      status_path = "/var/cache/icinga2/status.dat",
      objects_path = "/var/cache/icinga2/objects.cache",
    }

### Attribute: status\_path

Specifies where Icinga 2 Compat component will put the status.dat file,
which can be read by Icinga 1.x Classic UI and other addons. If not set,
it defaults to the localstatedir location.

### Attribute: objects\_path

Specifies where Icinga 2 Compat component will put the objects.cache
file, which can be read by Icinga 1.x Classic UI and other addons. If
not set, it defaults to the localstatedir location.

Type: ConsoleLogger
-------------------

Specifies Icinga 2 logging to the console. Objects of this type must
have the "local" specifier.

Example:

    local object ConsoleLogger "my-debug-console" {
      severity = "debug"
    }

### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

Type: FileLogger
----------------

Specifies Icinga 2 logging to a file. Objects of this type must have the
"local" specifier.

Example:

    local object FileLogger "my-debug-file" {
      severity = "debug",
      path = "/var/log/icinga2/icinga2-debug.log"
    }

### Attribute: path

The log path.

### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

Type: SyslogLogger
------------------

Specifies Icinga 2 logging to syslog. Objects of this type must have the
"local" specifier.

Example:

    local object SyslogLogger "my-crit-syslog" {
      severity = "critical"
    }

### Attribute: severity

The minimum severity for this log. Can be "debug", "information",
"warning" or "critical". Defaults to "information".

Type: Endpoint
--------------

Endpoint objects are used to specify connection information for remote
Icinga 2 instances. Objects of this type should not be local:

    object Endpoint "icinga-c2" {
      node = "192.168.5.46",
      service = 7777,
    }

### Attribute: node

The hostname/IP address of the remote Icinga 2 instance.

### Attribute: service

The service name/port of the remote Icinga 2 instance.

Type: CheckCommand
------------------

A check command definition. Additional default command macros can be
defined here.

Example:

    object CheckCommand "check_snmp" inherits "plugin-check-command" {
      command = "$plugindir$/check_snmp -H $address$ -C $community$ -o $oid$",

      macros = {2yy
        plugindir = "/usr/lib/nagios/plugins",
        address = "127.0.0.1",
        community = "public",
      }
    }

Type: NotificationCommand
-------------------------

A notification command definition.

Example:

    object NotificationCommand "mail-service-notification" inherits "plugin-notification-command" {
      command = "/usr/bin/printf \"%b\" \"***** Icinga  *****\n\nNotification Type: $NOTIFICATIONTYPE$\n\nService: $SERVICEDESC$\nHost: $HOSTALIAS$\nAddress: $HOSTADDRESS$\nState: $SERVICESTATE$\n\nDate/Time: $LONGDATETIME$\n\nAdditional Info: $SERVICEOUTPUT$\n\nComment: [$NOTIFICATIONAUTHORNAME$] $NOTIFICATIONCOMMENT$\n\n\" | /usr/bin/mail -s \"$NOTIFICATIONTYPE$ - $HOSTNAME$ - $SERVICEDESC$ - $SERVICESTATE$\" $CONTACTEMAIL$",
    }

Type: EventCommand

    An event command definition.

    NOTE: Similar to Icinga 1.x event handlers.

    Example:

    -------------------------------------------------------------------------------
    object EventCommand "restart-httpd-event" inherits "plugin-event-command" {
      command = "/usr/local/icinga/libexec/restart-httpd.sh",
    }
    -------------------------------------------------------------------------------


    Type: Service

Service objects describe network services and how they should be checked
by Icinga 2.

> **Note**
>
> Better create a service template and use that reference on the host
> definition as shown below.

Example:

    object Service "localhost-uptime" {
      host_name = "localhost",

      alias = "localhost Uptime",

      methods = {
        check = "PluginCheck"
      },

      check_command = "check_snmp",

      macros = {
        plugindir = "/usr/lib/nagios/plugins",
        address = "127.0.0.1",
        community = "public",
        oid = "DISMAN-EVENT-MIB::sysUpTimeInstance"
      }

      check_interval = 60s,
      retry_interval = 15s,

      servicegroups = { "all-services", "snmp" },

      checkers = { "*" },
    }

### Attribute: host\_name

The host this service belongs to. There must be a "Host" object with
that name.

### Attribute: alias

Optional. A short description of the service.

### Attribute: methods - check

The check type of the service. For now only external check plugins are
supported ("PluginCheck").

### Attribute: check\_command

Optional when not using the "external plugin" check type. The check
command. May contain macros.

### Attribute: check\_interval

Optional. The check interval (in seconds).

### Attribute: retry\_interval

Optional. The retry interval (in seconds). This is used when the service
is in a soft state.

### Attribute: servicegroups

Optional. The service groups this service belongs to.

### Attribute: checkers

Optional. A list of remote endpoints that may check this service.
Wildcards can be used here.

Type: ServiceGroup
------------------

A group of services.

Example:

    object ServiceGroup "snmp" {
      alias = "SNMP services",

      custom = {
        notes_url = "http://www.example.org/",
        action_url = "http://www.example.org/",
      }
    }

### Attribute: alias

Optional. A short description of the service group.

### Attribute: notes\_url

Optional. Notes URL. Used by the CGIs.

### Attribute: action\_url

Optional. Action URL. Used by the CGIs.

Type: Host
----------

A host. Unlike in Icinga 1.x hosts are not checkable objects in Icinga
2.

Example:

    object Host "localhost" {
      alias = "The best host there is",

      hostgroups = [ "all-hosts" ],

      hostcheck = "ping",
      dependencies = [ "router-ping" ]

      services["ping"] = { templates = "ping" }
      services["http"] = {
        templates = "my-http",
        macros = {
          vhost = "test1.example.org",
          port = 81
        }
      }

      check_interval = 60m,
      retry_interval = 15m,

      servicegroups = [ "all-services" ],

      checkers = { "*" },
    }

### Attribute: alias

Optional. A short description of the host.

### Attribute: hostgroups

Optional. A list of host groups this host belongs to.

### Attribute: hostcheck

Optional. A service that is used to determine whether the host is up or
down.

### Attribute: hostdependencies

Optional. A list of hosts that are used to determine whether the host is
unreachable.

### Attribute: servicedependencies

Optional. A list of services that are used to determine whether the host
is unreachable.

### Attribute: services

Inline definition of services. Each service name is defined in square
brackets and got its own dictionary with attribute properties, such as
the template service being used. All other service-related properties
are additively copied into the new service object.

The new service’s name is "hostname-service" - where "service" is the
array key in the services array.

The priority for service properties is (from highest to lowest):

1.  Properties specified in the dictionary of the inline service
    definition

2.  Host properties

3.  Properties inherited from the new service’s parent object

### Attribute: check\_interval

Optional. Copied into inline service definitions. The host itself does
not have any checks.

### Attribute: retry\_interval

Optional. Copied into inline service definitions. The host itself does
not have any checks.

### Attribute: servicegroups

Optional. Copied into inline service definitions. The host itself does
not have any checks.

### Attribute: checkers

Optional. Copied into inline service definitions. The host itself does
not have any checks.

Type: HostGroup
---------------

A group of hosts.

Example

    object HostGroup "my-hosts" {
      alias = "My hosts",

      notes_url = "http://www.example.org/",
      action_url = "http://www.example.org/",
    }

### Attribute: alias

Optional. A short description of the host group.

### Attribute: notes\_url

Optional. Notes URL. Used by the CGIs.

### Attribute: action\_url

Optional. Action URL. Used by the CGIs.

Type: PerfdataWriter
--------------------

Write check result performance data to a defined path using macro
pattern.

Example

    local object PerfdataWriter "pnp" {
      perfdata_path = "/var/spool/icinga2/perfdata/service-perfdata",
      format_template = "DATATYPE::SERVICEPERFDATA\tTIMET::$TIMET$\tHOSTNAME::$HOSTNAME$\tSERVICEDESC::$SERVICEDESC$\tSERVICEPERFDATA::$SERVICEPERFDATA$\tSERVICECHECKCOMMAND::$SERVICECHECKCOMMAND$\tHOSTSTATE::$HOSTSTATE$\tHOSTSTATETYPE::$HOSTSTATETYPE$\tSERVICESTATE::$SERVICESTATE$\tSERVICESTATETYPE::$SERVICESTATETYPE$",
      rotation_interval = 15s,
    }

### Attribute: perfdata\_path

Path to the service perfdata file.

> **Note**
>
> Will be automatically rotated with timestamp suffix.

### Attribute: format\_template

Formatting of performance data output for graphing addons or other post
processing.

### Attribute: rotation\_interval

Rotation interval for the file defined in *perfdata\_path*.

Type: IdoMySqlConnection
------------------------

IDO DB schema compatible output into mysql database.

Example

    library "ido_mysql"
    local object IdoMysqlDbConnection "mysql-ido" {
      host = "127.0.0.1",
      port = "3306",
      user = "icinga",
      password = "icinga",
      database = "icinga",
      table_prefix = "icinga_",
      instance_name = "icinga2",
      instance_description = "icinga2 dev instance"
    }

### Attribute: host

MySQL database host address. Default is *localhost*.

### Attribute: port

MySQL database port. Default is *3306*.

### Attribute: user

MySQL database user with read/write permission to the icinga database.
Default is *icinga*.

### Attribute: password

MySQL database user’s password. Default is *icinga*.

### Attribute: database

MySQL database name. Default is *icinga*.

### Attribute: table\_prefix

MySQL database table prefix. Default is *icinga\_*.

### Attribute: instance\_name

Unique identifier for the local Icinga 2 instance.

### Attribute: instance\_description

Optional. Description for the Icinga 2 instance.

Type: LiveStatusComponent
-------------------------

Livestatus api interface available as tcp or unix socket.

Example

    library "livestatus"

    local object LivestatusComponent "livestatus-tcp" {
      socket_type = "tcp",
      host = "127.0.0.1",
      port = "6558"
    }

    local object LivestatusComponent "livestatus-unix" {
      socket_type = "unix",
      socket_path = "/var/run/icinga2/livestatus"
    }

### Attribute: socket\_type

*tcp* or *unix* socket. Default is *unix*.

> **Note**
>
> *unix* sockets are not supported on Windows.

### Attribute: host

Only valid when socket\_type="tcp". Host address to listen on for
connections.

### Attribute: port

Only valid when socket\_type="tcp". Port to listen on for connections.

### Attribute: socket\_path

Only valid when socket\_type="unix". Local unix socket file. Not
supported on Windows.

Configuration Examples
======================

Non-networked minimal example
-----------------------------

> **Note**
>
> Icinga 2 ITL provides itl/standalone.conf which loads all required
> components, as well as itl/itl.conf includes many object templates
> already for an easy start with Icinga 2.

    local object IcingaApplication "icinga" {

    }

    local object Component "checker" {

    }

    local object Component "delegation" {

    }

    object CheckCommand "ping" {
      command = "$plugindir$/check_ping -H $address$ -w $wrta$,$wpl$% -c $crta$,$cpl$%",
    }

    template Service "icinga-service" {
      methods = {
        check = "PluginCheck"
      },

      macros = {
        plugindir = "/usr/lib/nagios/plugins"
      }
    }

    template Service "ping-tmpl" inherits "icinga-service" {
      check_command = "ping",
      macros += {
        wrta = 50,
        wpl = 5,
        crta = 100,
        cpl = 10
      }
    }

    object Host "localhost" {
      services["ping"]  = { templates = "ping-tmpl" },

      macros = {
        address = "127.0.0.1"
      },

      check_interval = 10m
    }

> **Note**
>
> You may also want to load the "compat" component if you want Icinga 2
> to write status.dat and objects.cache files.

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/
