# Migration from Icinga 1.x <a id="migration"></a>

## Configuration Migration <a id="configuration-migration"></a>

The Icinga 2 configuration format introduces plenty of behavioural changes. In
order to ease migration from Icinga 1.x, this section provides hints and tips
on your migration requirements.

### Manual Config Migration <a id="manual-config-migration"></a>

For a long-term migration of your configuration you should consider re-creating
your configuration based on the proposed Icinga 2 configuration paradigm.

Please read the [next chapter](23-migrating-from-icinga-1x.md#differences-1x-2) to find out more about the differences
between 1.x and 2.

### Manual Config Migration Hints <a id="manual-config-migration-hints"></a>

These hints should provide you with enough details for manually migrating your configuration,
or to adapt your configuration export tool to dump Icinga 2 configuration instead of
Icinga 1.x configuration.

The examples are taken from Icinga 1.x test and production environments and converted
straight into a possible Icinga 2 format. If you found a different strategy, please
let us know!

If you require in-depth explanations, please check the [next chapter](23-migrating-from-icinga-1x.md#differences-1x-2).

#### Manual Config Migration Hints for Intervals <a id="manual-config-migration-hints-Intervals"></a>

By default all intervals without any duration literal are interpreted as seconds. Therefore
all existing Icinga 1.x `*_interval` attributes require an additional `m` duration literal.

Icinga 1.x:

```
define service {
  service_description             service1
  host_name                       localhost1
  check_command                   test_customvar
  use                             generic-service
  check_interval                  5
  retry_interval                  1
}
```

Icinga 2:

```
object Service "service1" {
  import "generic-service"
  host_name = "localhost1"
  check_command = "test_customvar"
  check_interval = 5m
  retry_interval = 1m
}
```

#### Manual Config Migration Hints for Services <a id="manual-config-migration-hints-services"></a>

If you have used the `host_name` attribute in Icinga 1.x with one or more host names this service
belongs to, you can migrate this to the [apply rules](03-monitoring-basics.md#using-apply) syntax.

Icinga 1.x:

```
define service {
  service_description             service1
  host_name                       localhost1,localhost2
  check_command                   test_check
  use                             generic-service
}
```

Icinga 2:

```
apply Service "service1" {
  import "generic-service"
  check_command = "test_check"

  assign where host.name in [ "localhost1", "localhost2" ]
}
```

In Icinga 1.x you would have organized your services with hostgroups using the `hostgroup_name` attribute
like the following example:

```
define service {
  service_description             servicewithhostgroups
  hostgroup_name                  hostgroup1,hostgroup3
  check_command                   test_check
  use                             generic-service
}
```

Using Icinga 2 you can migrate this to the [apply rules](03-monitoring-basics.md#using-apply) syntax:

```
apply Service "servicewithhostgroups" {
  import "generic-service"
  check_command = "test_check"

  assign where "hostgroup1" in host.groups
  assign where "hostgroup3" in host.groups
}
```

#### Manual Config Migration Hints for Group Members <a id="manual-config-migration-hints-group-members"></a>

The Icinga 1.x hostgroup `hg1` has two members `host1` and `host2`. The hostgroup `hg2` has `host3` as
a member and includes all members of the `hg1` hostgroup.

```
define hostgroup {
  hostgroup_name                  hg1
  members                         host1,host2
}

define hostgroup {
  hostgroup_name                  hg2
  members                         host3
  hostgroup_members               hg1
}
```

This can be migrated to Icinga 2 and [using group assign](17-language-reference.md#group-assign). The additional nested hostgroup
`hg1` is included into `hg2` with the `groups` attribute.

```
object HostGroup "hg1" {
  groups = [ "hg2" ]
  assign where host.name in [ "host1", "host2" ]
}

object HostGroup "hg2" {
  assign where host.name == "host3"
}
```

These assign rules can be applied for all groups: `HostGroup`, `ServiceGroup` and `UserGroup`
(requires renaming from `contactgroup`).

> **Tip**
>
> Define custom attributes and assign/ignore members based on these attribute pattern matches.



#### Manual Config Migration Hints for Check Command Arguments <a id="manual-config-migration-hints-check-command-arguments"></a>

Host and service check command arguments are separated by a `!` in Icinga 1.x. Their order is important and they
are referenced as `$ARGn$` where `n` is the argument counter.

```
define command {
  command_name                      my-ping
  command_line                      $USER1$/check_ping -H $HOSTADDRESS$ -w $ARG1$ -c $ARG2$ -p 5
}

define service {
  use                               generic-service
  host_name                         my-server
  service_description               my-ping
  check_command                     my-ping-check!100.0,20%!500.0,60%
}
```

While you could manually migrate this like (please note the new generic command arguments and default argument values!):

```
object CheckCommand "my-ping-check" {
  command = [
    PluginDir + "/check_ping", "-4"
  ]

  arguments = {
    "-H" = "$ping_address$"
    "-w" = "$ping_wrta$,$ping_wpl$%"
    "-c" = "$ping_crta$,$ping_cpl$%"
    "-p" = "$ping_packets$"
    "-t" = "$ping_timeout$"
  }

  vars.ping_address = "$address$"
  vars.ping_wrta = 100
  vars.ping_wpl = 5
  vars.ping_crta = 200
  vars.ping_cpl = 15
}

object Service "my-ping" {
  import "generic-service"
  host_name = "my-server"
  check_command = "my-ping-check"

  vars.ping_wrta = 100
  vars.ping_wpl = 20
  vars.ping_crta = 500
  vars.ping_cpl = 60
}
```

#### Manual Config Migration Hints for Runtime Macros <a id="manual-config-migration-hints-runtime-macros"></a>

Runtime macros have been renamed. A detailed comparison table can be found [here](23-migrating-from-icinga-1x.md#differences-1x-2-runtime-macros).

For example, accessing the service check output looks like the following in Icinga 1.x:

```
$SERVICEOUTPUT$
```

In Icinga 2 you will need to write:

```
$service.output$
```

Another example referencing the host's address attribute in Icinga 1.x:

```
$HOSTADDRESS$
```

In Icinga 2 you'd just use the following macro to access all `address` attributes (even overridden from the service objects):

```
$address$
```

#### Manual Config Migration Hints for Runtime Custom Attributes <a id="manual-config-migration-hints-runtime-custom-attributes"></a>

Custom variables from Icinga 1.x are available as Icinga 2 custom attributes.

```
define command {
  command_name                    test_customvar
  command_line                    echo "Host CV: $_HOSTCVTEST$ Service CV: $_SERVICECVTEST$\n"
}

define host {
  host_name                       localhost1
  check_command                   test_customvar
  use                             generic-host
  _CVTEST                         host cv value
}

define service {
  service_description             service1
  host_name                       localhost1
  check_command                   test_customvar
  use                             generic-service
  _CVTEST                         service cv value
}
```

Can be written as the following in Icinga 2:

```
object CheckCommand "test_customvar" {
  command = "echo "Host CV: $host.vars.CVTEST$ Service CV: $service.vars.CVTEST$\n""
}

object Host "localhost1" {
  import "generic-host"
  check_command = "test_customvar"
  vars.CVTEST = "host cv value"
}

object Service "service1" {
  host_name = "localhost1"
  check_command = "test_customvar"
  vars.CVTEST = "service cv value"
}
```

If you are just defining `$CVTEST$` in your command definition, its value depends on the
execution scope -- the host check command will fetch the host attribute value of `vars.CVTEST`
while the service check command resolves its value to the service attribute attribute `vars.CVTEST`.

> **Note**
>
> Custom attributes in Icinga 2 are case-sensitive. `vars.CVTEST` is not the same as `vars.CvTest`.

#### Manual Config Migration Hints for Contacts (Users) <a id="manual-config-migration-hints-contacts-users"></a>

Contacts in Icinga 1.x act as users in Icinga 2, but do not have any notification commands specified.
This migration part is explained in the [next chapter](23-migrating-from-icinga-1x.md#manual-config-migration-hints-notifications).

```
define contact{
  contact_name                    testconfig-user
  use                             generic-user
  alias                           Icinga Test User
  service_notification_options    c,f,s,u
  email                           icinga@localhost
}
```

The `service_notification_options` can be [mapped](23-migrating-from-icinga-1x.md#manual-config-migration-hints-notification-filters)
into generic `state` and `type` filters, if additional notification filtering is required. `alias` gets
renamed to `display_name`.

```
object User "testconfig-user" {
  import "generic-user"
  display_name = "Icinga Test User"
  email = "icinga@localhost"
}
```

This user can be put into usergroups (former contactgroups) or referenced in newly migration notification
objects.

#### Manual Config Migration Hints for Notifications <a id="manual-config-migration-hints-notifications"></a>

If you are migrating a host or service notification, you'll need to extract the following information from
your existing Icinga 1.x configuration objects

* host/service attribute `contacts` and `contact_groups`
* host/service attribute `notification_options`
* host/service attribute `notification_period`
* host/service attribute `notification_interval`

The clean approach is to refactor your current contacts and their notification command methods into a
generic strategy

* host or service has a notification type (for example mail)
* which contacts (users) are notified by mail?
* do the notification filters, periods, intervals still apply for them? (do a cleanup during migration)
* assign users and groups to these notifications
* Redesign the notifications into generic [apply rules](03-monitoring-basics.md#using-apply-notifications)


The ugly workaround solution could look like this:

Extract all contacts from the remaining groups, and create a unique list. This is required for determining
the host and service notification commands involved.

* contact attributes `host_notification_commands` and `service_notification_commands` (can be a comma separated list)
* get the command line for each notification command and store them for later
* create a new notification name and command name

Generate a new notification object based on these values. Import the generic template based on the type (`host` or `service`).
Assign it to the host or service and set the newly generated notification command name as `command` attribute.

```
object Notification "<notificationname>" {
  import "mail-host-notification"
  host_name = "<thishostname>"
  command = "<notificationcommandname>"
```

Convert the `notification_options` attribute from Icinga 1.x to Icinga 2 `states` and `types`. Details
[here](23-migrating-from-icinga-1x.md#manual-config-migration-hints-notification-filters). Add the notification period.

```
  states = [ OK, Warning, Critical ]
  types = [ Recovery, Problem, Custom ]
  period = "24x7"
```

The current contact acts as `users` attribute.

```
  users = [ "<contactwithnotificationcommand>" ]
}
```

Do this in a loop for all notification commands (depending if host or service contact). Once done, dump the
collected notification commands.

The result of this migration are lots of unnecessary notification objects and commands but it will unroll
the Icinga 1.x logic into the revamped Icinga 2 notification object schema. If you are looking for code
examples, try [LConf](https://www.netways.org).



#### Manual Config Migration Hints for Notification Filters <a id="manual-config-migration-hints-notification-filters"></a>

Icinga 1.x defines all notification filters in an attribute called `notification_options`. Using Icinga 2 you will
have to split these values into the `states` and `types` attributes.

> **Note**
>
> `Recovery` type requires the `Ok` state.
> `Custom` and `Problem` should always be set as `type` filter.

  Icinga 1.x option     | Icinga 2 state        | Icinga 2 type
  ----------------------|-----------------------|-------------------
  o                     | OK (Up for hosts)     |
  w                     | Warning               | Problem
  c                     | Critical              | Problem
  u                     | Unknown               | Problem
  d                     | Down                  | Problem
  s                     | .                     | DowntimeStart / DowntimeEnd / DowntimeRemoved
  r                     | Ok                    | Recovery
  f                     | .                     | FlappingStart / FlappingEnd
  n                     | 0  (none)             | 0 (none)
  .                     | .                     | Custom



#### Manual Config Migration Hints for Escalations <a id="manual-config-migration-hints-escalations"></a>

Escalations in Icinga 1.x are a bit tricky. By default service escalations can be applied to hosts and
hostgroups and require a defined service object.

The following example applies a service escalation to the service `dep_svc01` and all hosts in the `hg_svcdep2`
hostgroup. The default `notification_interval` is set to `10` minutes notifying the `cg_admin` contact.
After 20 minutes (`10*2`, notification_interval * first_notification) the notification is escalated to the
`cg_ops` contactgroup until 60 minutes (`10*6`) have passed.

```
define service {
  service_description             dep_svc01
  host_name                       dep_hostsvc01,dep_hostsvc03
  check_command                   test2
  use                             generic-service
  notification_interval           10
  contact_groups                  cg_admin
}

define hostgroup {
  hostgroup_name                  hg_svcdep2
  members                         dep_hostsvc03
}

# with hostgroup_name and service_description
define serviceescalation {
  hostgroup_name                  hg_svcdep2
  service_description             dep_svc01
  first_notification              2
  last_notification               6
  contact_groups                  cg_ops
}
```

In Icinga 2 the service and hostgroup definition will look quite the same. Save the `notification_interval`
and `contact_groups` attribute for an additional notification.

```
apply Service "dep_svc01" {
  import "generic-service"

  check_command = "test2"

  assign where host.name == "dep_hostsvc01"
  assign where host.name == "dep_hostsvc03"
}

object HostGroup "hg_svcdep2" {
  assign where host.name == "dep_hostsvc03"
}

apply Notification "email" to Service {
  import "service-mail-notification"

  interval = 10m
  user_groups = [ "cg_admin" ]

  assign where service.name == "dep_svc01" && (host.name == "dep_hostsvc01" || host.name == "dep_hostsvc03")
}
```

Calculate the begin and end time for the newly created escalation notification:

* begin = first_notification * notification_interval = 2 * 10m = 20m
* end = last_notification * notification_interval = 6 * 10m = 60m = 1h

Assign the notification escalation to the service `dep_svc01` on all hosts in the hostgroup `hg_svcdep2`.

```
apply Notification "email-escalation" to Service {
  import "service-mail-notification"

  interval = 10m
  user_groups = [ "cg_ops" ]

  times = {
    begin = 20m
    end = 1h
  }

  assign where service.name == "dep_svc01" && "hg_svcdep2" in host.groups
}
```

The assign rule could be made more generic and the notification be applied to more than
just this service belonging to hosts in the matched hostgroup.


> **Note**
>
> When the notification is escalated, Icinga 1.x suppresses notifications to the default contacts.
> In Icinga 2 an escalation is an additional notification with a defined begin and end time. The
> `email` notification will continue as normal.



#### Manual Config Migration Hints for Dependencies <a id="manual-config-migration-hints-dependencies"></a>

There are some dependency examples already in the [basics chapter](03-monitoring-basics.md#dependencies). Dependencies in
Icinga 1.x can be confusing in terms of which host/service is the parent and which host/service acts
as the child.

While Icinga 1.x defines `notification_failure_criteria` and `execution_failure_criteria` as dependency
filters, this behaviour has changed in Icinga 2. There is no 1:1 migration but generally speaking
the state filter defined in the `execution_failure_criteria` defines the Icinga 2 `state` attribute.
If the state filter matches, you can define whether to disable checks and notifications or not.

The following example describes service dependencies. If you migrate from Icinga 1.x, you will only
want to use the classic `Host-to-Host` and `Service-to-Service` dependency relationships.

```
define service {
  service_description             dep_svc01
  hostgroup_name                  hg_svcdep1
  check_command                   test2
  use                             generic-service
}

define service {
  service_description             dep_svc02
  hostgroup_name                  hg_svcdep2
  check_command                   test2
  use                             generic-service
}

define hostgroup {
  hostgroup_name                  hg_svcdep2
  members                         host2
}

define host{
  use                             linux-server-template
  host_name                       host1
  address                         192.168.1.10
}

# with hostgroup_name and service_description
define servicedependency {
  host_name                       host1
  dependent_hostgroup_name        hg_svcdep2
  service_description             dep_svc01
  dependent_service_description   *
  execution_failure_criteria      u,c
  notification_failure_criteria   w,u,c
  inherits_parent                 1
}
```

Map the dependency attributes accordingly.

  Icinga 1.x            | Icinga 2
  ----------------------|---------------------
  host_name             | parent_host_name
  dependent_host_name   | child_host_name (used in assign/ignore)
  dependent_hostgroup_name | all child hosts in group (used in assign/ignore)
  service_description   | parent_service_name
  dependent_service_description | child_service_name (used in assign/ignore)

And migrate the host and services.

```
object Host "host1" {
  import "linux-server-template"
  address = "192.168.1.10"
}

object HostGroup "hg_svcdep2" {
  assign where host.name == "host2"
}

apply Service "dep_svc01" {
  import "generic-service"
  check_command = "test2"

  assign where "hp_svcdep1" in host.groups
}

apply Service "dep_svc02" {
  import "generic-service"
  check_command = "test2"

  assign where "hp_svcdep2" in host.groups
}
```

When it comes to the `execution_failure_criteria` and `notification_failure_criteria` attribute migration,
you will need to map the most common values, in this example `u,c` (`Unknown` and `Critical` will cause the
dependency to fail). Therefore the `Dependency` should be ok on Ok and Warning. `inherits_parents` is always
enabled.

```
apply Dependency "all-svc-for-hg-hg_svcdep2-on-host1-dep_svc01" to Service {
  parent_host_name = "host1"
  parent_service_name = "dep_svc01"

  states = [ Ok, Warning ]
  disable_checks = true
  disable_notifications = true

  assign where "hg_svcdep2" in host.groups
}
```

Host dependencies are explained in the [next chapter](23-migrating-from-icinga-1x.md#manual-config-migration-hints-host-parents).



#### Manual Config Migration Hints for Host Parents <a id="manual-config-migration-hints-host-parents"></a>

Host parents from Icinga 1.x are migrated into `Host-to-Host` dependencies in Icinga 2.

The following example defines the `vmware-master` host as parent host for the guest
virtual machines `vmware-vm1` and `vmware-vm2`.

By default all hosts in the hostgroup `vmware` should get the parent assigned. This isn't really
solvable with Icinga 1.x parents, but only with host dependencies.

```
define host{
  use                             linux-server-template
  host_name                       vmware-master
  hostgroups                      vmware
  address                         192.168.1.10
}

define host{
  use                             linux-server-template
  host_name                       vmware-vm1
  hostgroups                      vmware
  address                         192.168.27.1
  parents                         vmware-master
}

define host{
  use                             linux-server-template
  host_name                       vmware-vm2
  hostgroups                      vmware
  address                         192.168.28.1
  parents                         vmware-master
}
```

By default all hosts in the hostgroup `vmware` should get the parent assigned (but not the `vmware-master`
host itself). This isn't really solvable with Icinga 1.x parents, but only with host dependencies as shown
below:

```
define hostdependency {
  dependent_hostgroup_name        vmware
  dependent_host_name             !vmware-master
  host_name                       vmware-master
  inherits_parent                 1
  notification_failure_criteria   d,u
  execution_failure_criteria      d,u
  dependency_period               testconfig-24x7
}
```

When migrating to Icinga 2, the parents must be changed to a newly created host dependency.


Map the following attributes

  Icinga 1.x            | Icinga 2
  ----------------------|---------------------
  host_name             | parent_host_name
  dependent_host_name   | child_host_name (used in assign/ignore)
  dependent_hostgroup_name | all child hosts in group (used in assign/ignore)

The Icinga 2 configuration looks like this:

```
object Host "vmware-master" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.1.10"
  vars.is_vmware_master = true
}

object Host "vmware-vm1" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.27.1"
}

object Host "vmware-vm2" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.28.1"
}

apply Dependency "vmware-master" to Host {
  parent_host_name = "vmware-master"

  assign where "vmware" in host.groups
  ignore where host.vars.is_vmware_master
  ignore where host.name == "vmware-master"
}
```

For easier identification you could add the `vars.is_vmware_master` attribute to the `vmware-master`
host and let the dependency ignore that instead of the hardcoded host name. That's different
to the Icinga 1.x example and a best practice hint only.


Another way to express the same configuration would be something like:

```
object Host "vmware-master" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.1.10"
}

object Host "vmware-vm1" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.27.1"
  vars.parents = [ "vmware-master" ]
}

object Host "vmware-vm2" {
  import "linux-server-template"
  groups += [ "vmware" ]
  address = "192.168.28.1"
  vars.parents = [ "vmware-master" ]
}

apply Dependency "host-to-parent-" for (parent in host.vars.parents) to Host {
  parent_host_name = parent
}
```

This example allows finer grained host-to-host dependency, as well as multiple dependency support.

#### Manual Config Migration Hints for Distributed Setups <a id="manual-config-migration-hints-distributed-setup"></a>

* Icinga 2 does not use active/passive instances calling OSCP commands and requiring the NSCA
daemon for passing check results between instances.
* Icinga 2 does not support any 1.x NEB addons for check load distribution

* If your current setup consists of instances distributing the check load, you should consider
building a [load distribution](06-distributed-monitoring.md#distributed-monitoring-scenarios) setup with Icinga 2.
* If your current setup includes active/passive clustering with external tools like Pacemaker/DRBD,
consider the [High Availability](06-distributed-monitoring.md#distributed-monitoring-scenarios) setup.
* If you have build your own custom configuration deployment and check result collecting mechanism,
you should re-design your setup and re-evaluate your requirements, and how they may be fulfilled
using the Icinga 2 cluster capabilities.


## Differences between Icinga 1.x and 2 <a id="differences-1x-2"></a>

### Configuration Format <a id="differences-1x-2-configuration-format"></a>

Icinga 1.x supports two configuration formats: key-value-based settings in the
`icinga.cfg` configuration file and object-based in included files (`cfg_dir`,
`cfg_file`). The path to the `icinga.cfg` configuration file must be passed to
the Icinga daemon at startup.

icinga.cfg:

```
enable_notifications=1
```

objects.cfg:

```
define service {
   notifications_enabled    0
}
```

Icinga 2 supports objects and (global) variables, but does not make a difference 
between the main configuration file or any other included file.

icinga2.conf:

```
const EnableNotifications = true

object Service "test" {
    enable_notifications = false
}
```

#### Sample Configuration and ITL <a id="differences-1x-2-sample-configuration-itl"></a>

While Icinga 1.x ships sample configuration and templates spread in various
object files, Icinga 2 moves all templates into the Icinga Template Library (ITL)
and includes them in the sample configuration.

Additional plugin check commands are shipped with Icinga 2 as well.

The ITL will be updated on every release and must not be edited by the user.

There are still generic templates available for your convenience which may or may
not be re-used in your configuration. For instance, `generic-service` includes
all required attributes except `check_command` for a service.

Sample configuration files are located in the `conf.d/` directory which is
included in `icinga2.conf` by default.

> **Note**
>
> Add your own custom templates in the `conf.d/` directory as well, e.g. inside
> the [templates.conf](04-configuring-icinga-2.md#templates-conf) file.

### Main Config File <a id="differences-1x-2-main-config"></a>

In Icinga 1.x there are many global configuration settings available in `icinga.cfg`.
Icinga 2 only uses a small set of [global constants](17-language-reference.md#constants) allowing
you to specify certain different setting such as the `NodeName` in a cluster scenario.

Aside from that, the [icinga2.conf](04-configuring-icinga-2.md#icinga2-conf) should take care of including
global constants, enabled [features](11-cli-commands.md#enable-features) and the object configuration.

### Include Files and Directories <a id="differences-1x-2-include-files-dirs"></a>

In Icinga 1.x the `icinga.cfg` file contains `cfg_file` and `cfg_dir`
directives. The `cfg_dir` directive recursively includes all files with a `.cfg`
suffix in the given directory. Only absolute paths may be used. The `cfg_file`
and `cfg_dir` directives can include the same file twice which leads to
configuration errors in Icinga 1.x.

```
cfg_file=/etc/icinga/objects/commands.cfg
cfg_dir=/etc/icinga/objects
```

Icinga 2 supports wildcard includes and relative paths, e.g. for including
`conf.d/*.conf` in the same directory.

```
include "conf.d/*.conf"
```

If you want to include files and directories recursively, you need to define
a separate option and add the directory and an optional pattern.

```
include_recursive "conf.d"
```

A global search path for includes is available for advanced features like
the Icinga Template Library (ITL) or additional monitoring plugins check
command configuration.

```
include <itl>
include <plugins>
```

By convention the `.conf` suffix is used for Icinga 2 configuration files.

### Resource File and Global Macros <a id="differences-1x-2-resource-file-global-macros"></a>

Global macros such as for the plugin directory, usernames and passwords can be
set in the `resource.cfg` configuration file in Icinga 1.x. By convention the
`USER1` macro is used to define the directory for the plugins.

Icinga 2 uses global constants instead. In the default config these are
set in the `constants.conf` configuration file:

```
/**
 * This file defines global constants which can be used in
 * the other configuration files. At a minimum the
 * PluginDir constant should be defined.
 */

const PluginDir = "/usr/lib/nagios/plugins"
```

[Global macros](17-language-reference.md#constants) can only be defined once. Trying to modify a
global constant will result in an error.

### Configuration Comments <a id="differences-1x-2-configuration-comments"></a>

In Icinga 1.x comments are made using a leading hash (`#`) or a semi-colon (`;`)
for inline comments.

In Icinga 2 comments can either be encapsulated by `/*` and `*/` (allowing for
multi-line comments) or starting with two slashes (`//`). A leading hash (`#`)
could also be used.

### Object Names <a id="differences-1x-2-object-names"></a>

Object names must not contain an exclamation mark (`!`). Use the `display_name` attribute
to specify user-friendly names which should be shown in UIs (supported by
Icinga Web 2 for example).

Object names are not specified using attributes (e.g. `service_description` for
services) like in Icinga 1.x but directly after their type definition.

```
define service {
    host_name  localhost
    service_description  ping4
}

object Service "ping4" {
  host_name = "localhost"
}
```

### Templates <a id="differences-1x-2-templates"></a>

In Icinga 1.x templates are identified using the `register 0` setting. Icinga 2
uses the `template` identifier:

```
template Service "ping4-template" { }
```

Icinga 1.x objects inherit from templates using the `use` attribute.
Icinga 2 uses the keyword `import` with template names in double quotes.

```
define service {
    service_description testservice
    use                 tmpl1,tmpl2,tmpl3
}

object Service "testservice" {
  import "tmpl1"
  import "tmpl2"
  import "tmpl3"
}
```

The last template overrides previously set values.

### Object attributes <a id="differences-1x-2-object-attributes"></a>

Icinga 1.x separates attribute and value pairs with whitespaces/tabs. Icinga 2
requires an equal sign (=) between them.

```
define service {
    check_interval  5
}

object Service "test" {
    check_interval = 5m
}
```

Please note that the default time value is seconds if no duration literal
is given. `check_interval = 5` behaves the same as `check_interval = 5s`.

All strings require double quotes in Icinga 2. Therefore a double quote
must be escaped by a backslash (e.g. in command line).
If an attribute identifier starts with a number, it must be enclosed
in double quotes as well.

#### Alias vs. Display Name <a id="differences-1x-2-alias-display-name"></a>

In Icinga 1.x a host can have an `alias` and a `display_name` attribute used
for a more descriptive name. A service only can have a `display_name` attribute.
The `alias` is used for group, timeperiod, etc. objects too.
Icinga 2 only supports the `display_name` attribute which is also taken into
account by Icinga web interfaces.

### Custom Attributes <a id="differences-1x-2-custom-attributes"></a>

Icinga 2 allows you to define custom attributes in the `vars` dictionary.
The `notes`, `notes_url`, `action_url`, `icon_image`, `icon_image_alt`
attributes for host and service objects are still available in Icinga 2.

`2d_coords` and `statusmap_image` are not supported in Icinga 2.

#### Custom Variables <a id="differences-1x-2-custom-variables"></a>

Icinga 1.x custom variable attributes must be prefixed using an underscore (`_`).
In Icinga 2 these attributes must be added to the `vars` dictionary as custom attributes.

```
vars.dn = "cn=icinga2-dev-host,ou=icinga,ou=main,ou=IcingaConfig,ou=LConf,dc=icinga,dc=org"
vars.cv = "my custom cmdb description"
```

These custom attributes are also used as [command parameters](03-monitoring-basics.md#command-passing-parameters).

While Icinga 1.x only supports numbers and strings as custom attribute values,
Icinga 2 extends that to arrays and (nested) dictionaries. For more details
look [here](03-monitoring-basics.md#custom-attributes).

### Host Service Relation <a id="differences-1x-2-host-service-relation"></a>

In Icinga 1.x a service object is associated with a host by defining the
`host_name` attribute in the service definition. Alternate methods refer
to `hostgroup_name` or behaviour changing regular expression.

The preferred way of associating hosts with services in Icinga 2 is by
using the [apply](03-monitoring-basics.md#using-apply) keyword.

Direct object relations between a service and a host still allow you to use
the `host_name` [Service](09-object-types.md#objecttype-service) object attribute.

### Users <a id="differences-1x-2-users"></a>

Contacts have been renamed to users (same for groups). A contact does not
only provide (custom) attributes and notification commands used for notifications,
but is also used for authorization checks in Icinga 1.x.

Icinga 2 changes that behavior and makes the user an attribute provider only.
These attributes can be accessed using [runtime macros](03-monitoring-basics.md#runtime-macros)
inside notification command definitions.

In Icinga 2 notification commands are not directly associated with users.
Instead the notification command is specified inside `Notification` objects next to
user and user group relations.

The `StatusDataWriter`, `IdoMySqlConnection` and `LivestatusListener` types will
provide the contact and contactgroups attributes for services for compatibility
reasons. These values are calculated from all services, their notifications,
and their users.

### Macros <a id="differences-1x-2-macros"></a>

Various object attributes and runtime variables can be accessed as macros in
commands in Icinga 1.x -- Icinga 2 supports all required [custom attributes](03-monitoring-basics.md#custom-attributes).

#### Command Arguments <a id="differences-1x-2-command-arguments"></a>

If you have previously used Icinga 1.x, you may already be familiar with
user and argument definitions (e.g., `USER1` or `ARG1`). Unlike in Icinga 1.x
the Icinga 2 custom attributes may have arbitrary names and arguments are no
longer specified in the `check_command` setting.

In Icinga 1.x arguments are specified in the `check_command` attribute and
are separated from the command name using an exclamation mark (`!`).

Please check the migration hints for a detailed
[migration example](23-migrating-from-icinga-1x.md#manual-config-migration-hints-check-command-arguments).

> **Note**
>
> The Icinga 1.x feature named `Command Expander` does not work with Icinga 2.

#### Environment Macros <a id="differences-1x-2-environment-macros"></a>

The global configuration setting `enable_environment_macros` does not exist in
Icinga 2.

Macros exported into the [environment](03-monitoring-basics.md#command-environment-variables)
can be set using the `env` attribute in command objects.

#### Runtime Macros <a id="differences-1x-2-runtime-macros"></a>

Icinga 2 requires an object specific namespace when accessing configuration
and stateful runtime macros. Custom attributes can be accessed directly.

If a runtime macro from Icinga 1.x is not listed here, it is not supported
by Icinga 2.

Changes to user (contact) runtime macros

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  CONTACTNAME            | user.name
  CONTACTALIAS           | user.display_name
  CONTACTEMAIL           | user.email
  CONTACTPAGER           | user.pager

`CONTACTADDRESS*` is not supported but can be accessed as `$user.vars.address1$`
if set.

Changes to service runtime macros

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  SERVICEDESC            | service.name
  SERVICEDISPLAYNAME     | service.display_name
  SERVICECHECKCOMMAND    | service.check_command
  SERVICESTATE           | service.state
  SERVICESTATEID         | service.state_id
  SERVICESTATETYPE       | service.state_type
  SERVICEATTEMPT         | service.check_attempt
  MAXSERVICEATTEMPT      | service.max_check_attempts
  LASTSERVICESTATE       | service.last_state
  LASTSERVICESTATEID     | service.last_state_id
  LASTSERVICESTATETYPE   | service.last_state_type
  LASTSERVICESTATECHANGE | service.last_state_change
  SERVICEDOWNTIME 	 | service.downtime_depth
  SERVICEDURATIONSEC     | service.duration_sec
  SERVICELATENCY         | service.latency
  SERVICEEXECUTIONTIME   | service.execution_time
  SERVICEOUTPUT          | service.output
  SERVICEPERFDATA        | service.perfdata
  LASTSERVICECHECK       | service.last_check
  SERVICENOTES           | service.notes
  SERVICENOTESURL        | service.notes_url
  SERVICEACTIONURL       | service.action_url


Changes to host runtime macros

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  HOSTNAME               | host.name
  HOSTADDRESS            | host.address
  HOSTADDRESS6           | host.address6
  HOSTDISPLAYNAME        | host.display_name
  HOSTALIAS              | (use `host.display_name` instead)
  HOSTCHECKCOMMAND       | host.check_command
  HOSTSTATE              | host.state
  HOSTSTATEID            | host.state_id
  HOSTSTATETYPE          | host.state_type
  HOSTATTEMPT            | host.check_attempt
  MAXHOSTATTEMPT         | host.max_check_attempts
  LASTHOSTSTATE          | host.last_state
  LASTHOSTSTATEID        | host.last_state_id
  LASTHOSTSTATETYPE      | host.last_state_type
  LASTHOSTSTATECHANGE    | host.last_state_change
  HOSTDOWNTIME  	 | host.downtime_depth
  HOSTDURATIONSEC        | host.duration_sec
  HOSTLATENCY            | host.latency
  HOSTEXECUTIONTIME      | host.execution_time
  HOSTOUTPUT             | host.output
  HOSTPERFDATA           | host.perfdata
  LASTHOSTCHECK          | host.last_check
  HOSTNOTES              | host.notes
  HOSTNOTESURL           | host.notes_url
  HOSTACTIONURL          | host.action_url
  TOTALSERVICES          | host.num_services
  TOTALSERVICESOK        | host.num_services_ok
  TOTALSERVICESWARNING   | host.num_services_warning
  TOTALSERVICESUNKNOWN   | host.num_services_unknown
  TOTALSERVICESCRITICAL  | host.num_services_critical

Changes to command runtime macros

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  COMMANDNAME            | command.name

Changes to notification runtime macros

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  NOTIFICATIONTYPE       | notification.type
  NOTIFICATIONAUTHOR     | notification.author
  NOTIFICATIONCOMMENT    | notification.comment
  NOTIFICATIONAUTHORNAME | (use `notification.author`)
  NOTIFICATIONAUTHORALIAS   | (use `notification.author`)


Changes to global runtime macros:

  Icinga 1.x             | Icinga 2
  -----------------------|----------------------
  TIMET                  | icinga.timet
  LONGDATETIME           | icinga.long_date_time
  SHORTDATETIME          | icinga.short_date_time
  DATE                   | icinga.date
  TIME                   | icinga.time
  PROCESSSTARTTIME       | icinga.uptime

Changes to global statistic macros:

  Icinga 1.x                        | Icinga 2
  ----------------------------------|----------------------
  TOTALHOSTSUP                      | icinga.num_hosts_up
  TOTALHOSTSDOWN                    | icinga.num_hosts_down
  TOTALHOSTSUNREACHABLE             | icinga.num_hosts_unreachable
  TOTALHOSTSDOWNUNHANDLED           | --
  TOTALHOSTSUNREACHABLEUNHANDLED    | --
  TOTALHOSTPROBLEMS                 | down
  TOTALHOSTPROBLEMSUNHANDLED        | down-(downtime+acknowledged)
  TOTALSERVICESOK                   | icinga.num_services_ok
  TOTALSERVICESWARNING              | icinga.num_services_warning
  TOTALSERVICESCRITICAL             | icinga.num_services_critical
  TOTALSERVICESUNKNOWN              | icinga.num_services_unknown
  TOTALSERVICESWARNINGUNHANDLED     | --
  TOTALSERVICESCRITICALUNHANDLED    | --
  TOTALSERVICESUNKNOWNUNHANDLED     | --
  TOTALSERVICEPROBLEMS              | ok+warning+critical+unknown
  TOTALSERVICEPROBLEMSUNHANDLED     | warning+critical+unknown-(downtime+acknowledged)




### External Commands <a id="differences-1x-2-external-commands"></a>

`CHANGE_CUSTOM_CONTACT_VAR` was renamed to `CHANGE_CUSTOM_USER_VAR`.

The following external commands are not supported:

```
CHANGE_*MODATTR
CHANGE_CONTACT_HOST_NOTIFICATION_TIMEPERIOD
CHANGE_HOST_NOTIFICATION_TIMEPERIOD
CHANGE_SVC_NOTIFICATION_TIMEPERIOD
DEL_DOWNTIME_BY_HOSTGROUP_NAME
DEL_DOWNTIME_BY_START_TIME_COMMENT
DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST
DISABLE_CONTACT_HOST_NOTIFICATIONS
DISABLE_CONTACT_SVC_NOTIFICATIONS
DISABLE_CONTACTGROUP_HOST_NOTIFICATIONS
DISABLE_CONTACTGROUP_SVC_NOTIFICATIONS
DISABLE_FAILURE_PREDICTION
DISABLE_HOST_AND_CHILD_NOTIFICATIONS
DISABLE_HOST_FRESHNESS_CHECKS
DISABLE_NOTIFICATIONS_EXPIRE_TIME
DISABLE_SERVICE_FRESHNESS_CHECKS
ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST
ENABLE_CONTACT_HOST_NOTIFICATIONS
ENABLE_CONTACT_SVC_NOTIFICATIONS
ENABLE_CONTACTGROUP_HOST_NOTIFICATIONS
ENABLE_CONTACTGROUP_SVC_NOTIFICATIONS
ENABLE_FAILURE_PREDICTION
ENABLE_HOST_AND_CHILD_NOTIFICATIONS
ENABLE_HOST_FRESHNESS_CHECKS
ENABLE_SERVICE_FRESHNESS_CHECKS
READ_STATE_INFORMATION
SAVE_STATE_INFORMATION
SET_HOST_NOTIFICATION_NUMBER
SET_SVC_NOTIFICATION_NUMBER
START_ACCEPTING_PASSIVE_HOST_CHECKS
START_ACCEPTING_PASSIVE_SVC_CHECKS
START_OBSESSING_OVER_HOST
START_OBSESSING_OVER_HOST_CHECKS
START_OBSESSING_OVER_SVC
START_OBSESSING_OVER_SVC_CHECKS
STOP_ACCEPTING_PASSIVE_HOST_CHECKS
STOP_ACCEPTING_PASSIVE_SVC_CHECKS
STOP_OBSESSING_OVER_HOST
STOP_OBSESSING_OVER_HOST_CHECKS
STOP_OBSESSING_OVER_SVC
STOP_OBSESSING_OVER_SVC_CHECKS
```

### Asynchronous Event Execution <a id="differences-1x-2-async-event-execution"></a>

Unlike Icinga 1.x, Icinga 2 does not block when it's waiting for a command
being executed -- whether if it's a check, a notification, an event
handler, a performance data writing update, etc. That way you'll
recognize low to zero (check) latencies with Icinga 2.

### Checks <a id="differences-1x-2-checks"></a>

#### Check Output <a id="differences-1x-2-check-output"></a>

Icinga 2 does not make a difference between `output` (first line) and
`long_output` (remaining lines) like in Icinga 1.x. Performance Data is
provided separately.

There is no output length restriction as known from Icinga 1.x using an
[8KB static buffer](https://docs.icinga.com/latest/en/pluginapi.html#outputlengthrestrictions).

The `StatusDataWriter`, `IdoMysqlConnection` and `LivestatusListener` types
split the raw output into `output` (first line) and `long_output` (remaining
lines) for compatibility reasons.

#### Initial State <a id="differences-1x-2-initial-state"></a>

Icinga 1.x uses the `max_service_check_spread` setting to specify a timerange
where the initial state checks must have happened. Icinga 2 will use the
`retry_interval` setting instead and `check_interval` divided by 5 if
`retry_interval` is not defined.

### Comments <a id="differences-1x-2-comments"></a>

Icinga 2 doesn't support non-persistent comments.

### Commands <a id="differences-1x-2-commands"></a>

Unlike in Icinga 1.x there are three different command types in Icinga 2:
`CheckCommand`, `NotificationCommand`, and `EventCommand`.

For example in Icinga 1.x it is possible to accidentally use a notification
command as an event handler which might cause problems depending on which
runtime macros are used in the notification command.

In Icinga 2 these command types are separated and will generate an error on
configuration validation if used in the wrong context.

While Icinga 2 still supports the complete command line in command objects, it's
recommended to use [command arguments](03-monitoring-basics.md#command-arguments)
with optional and conditional command line parameters instead.

It's also possible to define default argument values for the command itself
which can be overridden by the host or service then.

#### Command Timeouts <a id="differences-1x-2-commands-timeouts"></a>

In Icinga 1.x there were two global options defining a host and service check
timeout. This was essentially bad when there only was a couple of check plugins
requiring some command timeouts to be extended.

Icinga 2 allows you to specify the command timeout directly on the command. So,
if your VMVware check plugin takes 15 minutes, [increase the timeout](09-object-types.md#objecttype-checkcommand)
accordingly.


### Groups <a id="differences-1x-2-groups"></a>

In Icinga 2 hosts, services, and users are added to groups using the `groups`
attribute in the object. The old way of listing all group members in the group's
`members` attribute is available through `assign where` and `ignore where`
expressions by using [group assign](03-monitoring-basics.md#group-assign-intro).

```
object Host "web-dev" {
  import "generic-host"
}

object HostGroup "dev-hosts" {
  display_name = "Dev Hosts"
  assign where match("*-dev", host.name)
}
```

#### Add Service to Hostgroup where Host is Member <a id="differences-1x-2-service-hostgroup-host"></a>

In order to associate a service with all hosts in a host group the [apply](03-monitoring-basics.md#using-apply)
keyword can be used:

```
apply Service "ping4" {
  import "generic-service"

  check_command = "ping4"

  assign where "dev-hosts" in host.groups
}
```

### Notifications <a id="differences-1x-2-notifications"></a>

Notifications are a new object type in Icinga 2. Imagine the following
notification configuration problem in Icinga 1.x:

* Service A should notify contact X via SMS
* Service B should notify contact X via Mail
* Service C should notify contact Y via Mail and SMS
* Contact X and Y should also be used for authorization

The only way achieving a semi-clean solution is to

* Create contact X-sms, set service_notification_command for sms, assign contact
  to service A
* Create contact X-mail, set service_notification_command for mail, assign
  contact to service B
* Create contact Y, set service_notification_command for sms and mail, assign
  contact to service C
* Create contact X without notification commands, assign to service A and B

Basically you are required to create duplicated contacts for either each
notification method or used for authorization only.

Icinga 2 attempts to solve that problem in this way

* Create user X, set SMS and Mail attributes, used for authorization
* Create user Y, set SMS and Mail attributes, used for authorization
* Create notification A-SMS, set command for sms, add user X,
  assign notification A-SMS to service A
* Create notification B-Mail, set command for mail, add user X,
  assign notification Mail to service B
* Create notification C-SMS, set command for sms, add user Y,
  assign notification C-SMS to service C
* Create notification C-Mail, set command for mail, add user Y,
  assign notification C-Mail to service C

Previously in Icinga 1.x it looked like this:

```
service -> (contact, contactgroup) -> notification command
```

In Icinga 2 it will look like this:

```
Service -> Notification -> NotificationCommand
                        -> User, UserGroup
```

#### Escalations <a id="differences-1x-2-escalations"></a>

Escalations in Icinga 1.x require a separated object matching on existing
objects. Escalations happen between a defined start and end time which is
calculated from the notification_interval:

```
start = notification start + (notification_interval * first_notification)
end = notification start + (notification_interval * last_notification)
```

In theory first_notification and last_notification can be set to readable
numbers. In practice users are manipulating those attributes in combination
with notification_interval in order to get a start and end time.

In Icinga 2 the notification object can be used as notification escalation
if the start and end times are defined within the 'times' attribute using
duration literals (e.g. 30m).

The Icinga 2 escalation does not replace the current running notification.
In Icinga 1.x it's required to copy the contacts from the service notification
to the escalation to guarantee the normal notifications once an escalation
happens.
That's not necessary with Icinga 2 only requiring an additional notification
object for the escalation itself.

#### Notification Options <a id="differences-1x-2-notification-options"></a>

Unlike Icinga 1.x with the 'notification_options' attribute with comma-separated
state and type filters, Icinga 2 uses two configuration attributes for that.
All state and type filter use long names OR'd with a pipe together

```
notification_options w,u,c,r,f,s

states = [ Warning, Unknown, Critical ]
types = [ Problem, Recovery, FlappingStart, FlappingEnd, DowntimeStart, DowntimeEnd, DowntimeRemoved ]
```

Icinga 2 adds more fine-grained type filters for acknowledgements, downtime,
and flapping type (start, end, ...).

### Dependencies and Parents <a id="differences-1x-2-dependencies-parents"></a>

In Icinga 1.x it's possible to define host parents to determine network reachability
and keep a host's state unreachable rather than down.
Furthermore there are host and service dependencies preventing unnecessary checks and
notifications. A host must not depend on a service, and vice versa. All dependencies
are configured as separate objects and cannot be set directly on the host or service
object.

A service can now depend on a host, and vice versa. A service has an implicit dependency
(parent) to its host. A host to host dependency acts implicitly as host parent relation.

The former `host_name` and `dependent_host_name` have been renamed to `parent_host_name`
and `child_host_name` (same for the service attribute). When using apply rules the
child attributes may be omitted.

For detailed examples on how to use the dependencies please check the [dependencies](03-monitoring-basics.md#dependencies)
chapter.

Dependencies can be applied to hosts or services using the [apply rules](17-language-reference.md#apply).

The `StatusDataWriter`, `IdoMysqlConnection` and `LivestatusListener` types
support the Icinga 1.x schema with dependencies and parent attributes for
compatibility reasons.

### Flapping <a id="differences-1x-2-flapping"></a>

The Icinga 1.x flapping detection uses the last 21 states of a service. This
value is hardcoded and cannot be changed. The algorithm on determining a flapping state
is as follows:

```
flapping value = (number of actual state changes / number of possible state changes)
```

The flapping value is then compared to the low and high flapping thresholds.

The algorithm used in Icinga 2 does not store the past states but calculates the flapping
threshold from a single value based on counters and half-life values. Icinga 2 compares
the value with a single flapping threshold configuration attribute.

### Check Result Freshness <a id="differences-1x-2-check-result-freshness"></a>

Freshness of check results must be enabled explicitly in Icinga 1.x. The attribute
`freshness_threshold` defines the threshold in seconds. Once the threshold is triggered, an
active freshness check is executed defined by the `check_command` attribute. Both check
methods (active and passive) use the same freshness check method.

In Icinga 2 active check freshness is determined by the `check_interval` attribute and no
incoming check results in that period of time (last check + check interval). Passive check
freshness is calculated from the `check_interval` attribute if set. There is no extra
`freshness_threshold` attribute in Icinga 2. If the freshness checks are invalid, a new
service check is forced.

### Real Reload <a id="differences-1x-2-real-reload"></a>

In Nagios / Icinga 1.x a daemon reload does the following:

* receive reload signal SIGHUP
* stop all events (checks, notifications, etc.)
* read the configuration from disk and validate all config objects in a single threaded fashion
* validation NOT ok: stop the daemon (cannot restore old config state)
* validation ok: start with new objects, dump status.dat / ido

Unlike Icinga 1.x the Icinga 2 daemon reload does not block any event
execution during config validation:

* receive reload signal SIGHUP
* fork a child process, start configuration validation in parallel work queues
* parent process continues with old configuration objects and the event scheduling
(doing checks, replicating cluster events, triggering alert notifications, etc.)
* validation NOT ok: child process terminates, parent process continues with old configuration state
(this is **essential** for the [cluster config synchronisation](06-distributed-monitoring.md#distributed-monitoring-top-down-config-sync))
* validation ok: child process signals parent process to terminate and save its current state
(all events until now) into the icinga2 state file
* parent process shuts down writing icinga2.state file
* child process waits for parent process gone, reads the icinga2 state file and synchronizes all historical and status data
* child becomes the new session leader

The DB IDO configuration dump and status/historical event updates use a queue
not blocking event execution. Same goes for any other enabled feature.
The configuration validation itself runs in parallel allowing fast verification checks.

That way your monitoring does not stop during a configuration reload.


### State Retention <a id="differences-1x-2-state-retention"></a>

Icinga 1.x uses the `retention.dat` file to save its state in order to be able
to reload it after a restart. In Icinga 2 this file is called `icinga2.state`.

The format is **not** compatible with Icinga 1.x.

### Logging <a id="differences-1x-2-logging"></a>

Icinga 1.x supports syslog facilities and writes its own `icinga.log` log file
and archives. These logs are used in Icinga 1.x to generate
historical reports.

Icinga 2 compat library provides the CompatLogger object which writes the icinga.log and archive
in Icinga 1.x format in order to stay compatible with addons.

The native Icinga 2 logging facilities are split into three configuration objects: SyslogLogger,
FileLogger, StreamLogger. Each of them has their own severity and target configuration.

The Icinga 2 daemon log does not log any alerts but is considered an application log only.

### Broker Modules and Features <a id="differences-1x-2-broker-modules-features"></a>

Icinga 1.x broker modules are incompatible with Icinga 2.

In order to provide compatibility with Icinga 1.x the functionality of several
popular broker modules was implemented for Icinga 2:

* IDOUtils
* Livestatus
* Cluster (allows for high availability and load balancing)


### Distributed Monitoring <a id="differences-1x-2-distributed-monitoring"></a>

Icinga 1.x uses the native "obsess over host/service" method which requires the NSCA addon
passing the slave's check results passively onto the master's external command pipe.
While this method may be used for check load distribution, it does not provide any configuration
distribution out-of-the-box. Furthermore comments, downtimes, and other stateful runtime data is
not synced between the master and slave nodes. There are addons available solving the check
and configuration distribution problems Icinga 1.x distributed monitoring currently suffers from.

Icinga 2 implements a new built-in
[distributed monitoring architecture](06-distributed-monitoring.md#distributed-monitoring-scenarios),
including config and check distribution, IPv4/IPv6 support, SSL certificates and zone support for DMZ.
High Availability and load balancing are also part of the Icinga 2 Cluster feature, next to local replay
logs on connection loss ensuring that the event history is kept in sync.
