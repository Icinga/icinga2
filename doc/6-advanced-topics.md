# Advanced Topics

## Downtimes

Downtimes can be scheduled for planned server maintenance or
any other targetted service outage you are aware of in advance.

Downtimes will suppress any notifications, and may trigger other
downtimes too. If the downtime was set by accident, or the duration
exceeds the maintenance, you can manually cancel the downtime.
Planned downtimes will also be taken into account for SLA reporting
tools calculating the SLAs based on the state and downtime history.

> **Note**
>
> Downtimes may overlap with their start and end times. If there
> are multiple downtimes triggered, the overall downtime depth
> will be more than `1`. This is useful when you want to extend
> your maintenance window taking longer than expected.

### Fixed and Flexible Downtimes

A `fixed` downtime will be activated at the defined start time, and
removed at the end time. During this time window the service state
will change to `NOT-OK` and then actually trigger the downtime.
Notifications are suppressed and the downtime depth is incremented.

Common scenarios are a planned distribution upgrade on your linux
servers, or database updates in your warehouse. The customer knows
about a fixed downtime window between 23:00 and 24:00. After 24:00
all problems should be alerted again. Solution is simple -
schedule a `fixed` downtime starting at 23:00 and ending at 24:00.

Unlike a `fixed` downtime, a `flexible` downtime end does not necessarily
happen at the provided end time. Instead the downtime will be triggered
in the time span defined by start and end time, but then last a defined
duration in minutes.

Imagine the following scenario: Your service is frequently polled
by users trying to grab free deleted domains for immediate registration.
Between 07:30 and 08:00 the impact will hit for 15 minutes and generate
a network outage visible to the monitoring. The service is still alive,
but answering too slow to Icinga 2 service checks.
For that reason, you may want to schedule a downtime between 07:30 and
08:00 with a duration of 15 minutes. The downtime will then last from
its trigger time until the duration is over. After that, the downtime
is removed (may happen before or after the actual end time!).

### Scheduling a downtime

This can either happen through a web interface (Icinga 1.x Classic UI or Web)
or by using the external command pipe provided by the `ExternalCommandListener`
configuration.

Fixed downtimes require a start and end time (a duration will be ignored).
Flexible downtimes need a start and end time for the time span, and a duration
independent from that time span.

> **Note**
>
> Modern web interfaces treat services in a downtime as `handled`.

### Triggered Downtimes

This is optional when scheduling a downtime. If there is already a downtime
scheduled for a future maintenance, the current downtime can be triggered by
that downtime. This renders useful if you have scheduled a host downtime and
are now scheduling a child host's downtime getting triggered by the parent
downtime on NOT-OK state change.

## Comments

Comments can be added at runtime and are persistent over restarts. You can
add useful information for others on repeating incidents (for example
"last time syslog at 100% cpu on 17.10.2013 due to stale nfs mount") which
is primarly accessible using web interfaces.

Adding and deleting comment actions are possible through the external command pipe
provided with the `ExternalCommandListener` configuration. The caller must
pass the comment id in case of manipulating an existing comment.

## Acknowledgements

If a problem is alerted and notified you may signal the other notification
receipients that you are aware of the problem and will handle it.

By sending an acknowledgement to Icinga 2 (using the external command pipe
provided with `ExternalCommandListener` configuration) all future notifications
are suppressed, a new comment is added with the provided description and
a notification with the type `NotificationFilterAcknowledgement` is sent
to all notified users.

> **Note**
>
> Modern web interfaces treat acknowledged problems as `handled`.

### Expiring Acknowledgements

Once a problem is acknowledged it may disappear from your `handled problems`
dashboard and no-one ever looks at it again since it will suppress
notifications too.

This `fire-and-forget` action is quite common. If you're sure that a
current problem should be resolved in the future at a defined time,
you can define an expiration time when acknowledging the problem.

Icinga 2 will clear the acknowledgement when expired and start to
re-notify if the problem persists.

## Cluster

An Icinga 2 cluster consists of two or more nodes and can reside on multiple
architectures. The base concept of Icinga 2 is the possibility to add additional
features using components. In case of a cluster setup you have to add the
cluster feature to all nodes. Before you start configuring the diffent nodes
it's necessary to setup the underlying communication layer based on SSL.

### Certificate Authority and Certificates

Icinga 2 comes with two scripts helping you to create CA and node certificates
for you Icinga 2 Cluster.

The first step is the creation of CA using the following command:

    icinga2-build-ca

Please make sure to export a variable containing an empty folder for the created
CA files:

    export ICINGA_CA="/root/icinga-ca"

In the next step you have to create a certificate and a key file for every node
using the following command:

    icinga2-build-key icinga-node-1

Please create a certificate and a key file for every node in the Icinga 2
Cluster and save the CA key in case you want to set up certificates for
additional nodes at a later date.

### Enable the Cluster Configuration

Until the cluster-component is moved into an independent feature you have to
enable the required libraries in the icinga2.conf configuration file:

    library "cluster"

### Configure the ClusterListener Object

The ClusterListener needs to be configured on every node in the cluster with the
following settings:

  Configuration Setting    |Value
  -------------------------|------------------------------------
  ca_path                  | path to ca.crt file
  cert_path                | path to server certificate
  key_path                 | path to server key
  bind_port                | port for incoming and outgoing conns
  peers                    | array of all reachable nodes
  ------------------------- ------------------------------------

A sample config part can look like this:

    /**
     * Load cluster library and configure ClusterListener using certificate files
     */
    library "cluster"

    object ClusterListener "cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt",
      cert_path = "/etc/icinga2/ca/icinga-node-1.crt",
      key_path = "/etc/icinga2/ca/icinga-node-1.key",

      bind_port = 8888,

      peers = [ "icinga-node-2" ]
    }

Peers configures the direction used to connect multiple nodes together. If have
a three node cluster consisting of

* node-1
* node-2
* node-3

and `node-3` is only reachable from `node-2`, you have to consider this in your
peer configuration

### Configure Cluster Endpoints

In addition to the configured port and hostname every endpoint can have specific
abilities to send configuration files to other nodes and limit the hosts allowed
to send configuration files.

  Configuration Setting    |Value
  -------------------------|------------------------------------
  host                     | hostname
  port                     | port
  accept_config            | defines all nodes allowed to send configs
  config_files             | defines all files to be send to that node - MUST BE AN ABSOLUTE PATH
  ------------------------- ------------------------------------

A sample config part can look like this:

    /**
     * Configure endpoints for cluster configuration
     */
	
    object Endpoint "icinga-node-1" {
      host = "icinga-node-1.localdomain",
      port = 8888,
      config_files = ["/etc/icinga2/conf.d/*.conf"]
    }

If you update the configuration files on the configured file sender, it will
force a restart on all receiving nodes after validating the new config.

By default these configuration files are saved in /var/lib/icinga2/cluster/config.
In order to load configuration files which were received from a remote Icinga 2
instance you will have to add the following include directive to your
`icinga2.conf` configuration file:

    include (IcingaLocalStateDir + "/lib/icinga2/cluster/config/*/*")

### Initial Sync

In order to make sure that all of your cluster nodes have the same state you will
have to pick one of the nodes as your initial "master" and copy its state file
to all the other nodes.

You can find the state file in `/var/lib/icinga2/icinga2.state`. Before copying
the state file you should make sure that all your cluster nodes are properly shut
down.

## Dependencies

Icinga 2 uses host and service dependencies as attribute directly on the host or
service object or template. A service can depend on a host, and vice versa. A
service has an implicit dependeny (parent) to its host. A host to host
dependency acts implicit as host parent relation.

A common scenario is the Icinga 2 server behind a router. Checking internet
access by pinging the Google DNS server `google-dns` is a common method, but
will fail in case the `dsl-router` host is down. Therefore the example below
defines a host dependency which acts implicit as parent relation too.

Furthermore the host may be reachable but ping samples are dropped by the
router's firewall.

    object Host "dsl-router" {
      services["ping4"] = {
        templates = "generic-service",
        check_command = "ping4"
      }

      macros = {
        address = "192.168.1.1",
      },
    }

    object Host "google-dns" {
      services["ping4"] = {
        templates = "generic-service",
        check_command = "ping4",
        service_dependencies = [
          { host = "dsl-router", service = "ping4" }
        ]
      }

      macros = {
        address = "8.8.8.8",
      }, 
      
      host_dependencies = [ "dsl-router" ]
    }

## Check Result Freshness

In Icinga 2 active check freshness is enabled by default. It is determined by the
`check_interval` attribute and no incoming check results in that period of time.

    threshold = last check execution time + check interval

Passive check freshness is calculated from the `check_interval` attribute if set.

    threshold = last check result time + check interval

If the freshness checks are invalid, a new check is executed defined by the
`check_command` attribute.

## Check Flapping

The flapping algorithm used in Icinga 2 does not store the past states but
calculcates the flapping threshold from a single value based on counters and
half-life values. Icinga 2 compares the value with a single flapping threshold
configuration attribute named `flapping_threshold`.

> **Note**
>
> Flapping must be explicitely enabled seting the `Service` object attribute
> `enable_flapping = 1`.

## Volatile Services

By default all services remain in a non-volatile state. When a problem
occurs, the `SOFT` state applies and once `max_check_attempts` attribute
is reached with the check counter, a `HARD` state transition happens.
Notifications are only triggered by `HARD` state changes and are then
re-sent defined by the `notification_interval` attribute.

It may be reasonable to have a volatile service which stays in a `HARD`
state type if the service stays in a `NOT-OK` state. That way each
service recheck will automatically trigger a notification unless the
service is acknowledged or in a scheduled downtime.

## Modified Attributes

Icinga 2 allows you to modify defined object attributes at runtime different to
the local configuration object attributes. These modified attributes are
stored as bit-shifted-value and made available in backends. Icinga 2 stores
modified attributes in its state file and restores them on restart.

Modified Attributes can be reset using external commands.


## Plugin API

Currently the native plugin api inherited from the `Nagios Plugins` project is available.
Future specifications will be documented here.

### Nagios Plugin API

The `Nagios Plugin API` is defined the [Nagios Plugins Development Guidelines](https://www.nagios-plugins.org/doc/guidelines.html).



