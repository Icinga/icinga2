# Technical Concepts <a id="technical-concepts"></a>

This chapter provides insights into specific Icinga 2
components, libraries, features and any other technical concept
and design.

<!--
## Application <a id="technical-concepts-application"></a>

### Libraries <a id="technical-concepts-application-libraries"></a>


## Configuration <a id="technical-concepts-configuration"></a>

### Compiler <a id="technical-concepts-configuration-compiler"></a>
-->

## Features <a id="technical-concepts-features"></a>

Features are implemented in specific libraries and can be enabled
using CLI commands.

Features either write specific data or receive data.

Examples for writing data: [DB IDO](14-features.md#db-ido), [Graphite](14-features.md#graphite-carbon-cache-writer), [InfluxDB](14-features.md#influxdb-writer). [GELF](14-features.md#gelfwriter), etc.
Examples for receiving data: [REST API](12-icinga2-api.md#icinga2-api), etc.

The implementation of features makes use of existing libraries
and functionality. This makes the code more abstract, but shorter
and easier to read.

Features register callback functions on specific events they want
to handle. For example the `GraphiteWriter` feature subscribes to
new CheckResult events.

Each time Icinga 2 receives and processes a new check result, this
event is triggered and forwarded to all subscribers.

The GraphiteWriter feature calls the registered function and processes
the received data. Features which connect Icinga 2 to external interfaces
normally parse and reformat the received data into an applicable format.

The GraphiteWriter uses a TCP socket to communicate with the carbon cache
daemon of Graphite. The InfluxDBWriter is instead writing bulk metric messages
to InfluxDB's HTTP API.



## Cluster <a id="technical-concepts-cluster"></a>

### Communication <a id="technical-concepts-cluster-communication"></a>

Icinga 2 uses its own certificate authority (CA) by default. The
public and private CA keys can be generated on the signing master.

Each node certificate must be signed by the private CA key.

Note: The following description uses `parent node` and `child node`.
This also applies to nodes in the same cluster zone.

During the connection attempt, an SSL handshake is performed.
If the public certificate of a child node is not signed by the same
CA, the child node is not trusted and the connection will be closed.

If the SSL handshake succeeds, the parent node reads the
certificate's common name (CN) of the child node and looks for
a local Endpoint object name configuration.

If there is no Endpoint object found, further communication
(runtime and config sync, etc.) is terminated.

The child node also checks the CN from the parent node's public
certificate. If the child node does not find any local Endpoint
object name configuration, it will not trust the parent node.

Both checks prevent accepting cluster messages from an untrusted
source endpoint.

If an Endpoint match was found, there is one additional security
mechanism in place: Endpoints belong to a Zone hierarchy.

Several cluster messages can only be sent "top down", others like
check results are allowed being sent from the child to the parent node.

Once this check succeeds the cluster messages are exchanged and processed.


### CSR Signing <a id="technical-concepts-cluster-csr-signing"></a>

In order to make things easier, Icinga 2 provides built-in methods
to allow child nodes to request a signed certificate from the
signing master.

Icinga 2 v2.8 introduces the possibility to request certificates
from indirectly connected nodes. This is required for multi level
cluster environments with masters, satellites and clients.

CSR Signing in general starts with the master setup. This step
ensures that the master is in a working CSR signing state with:

* public and private CA key in `/var/lib/icinga2/ca`
* private `TicketSalt` constant defined inside the `api` feature
* Cluster communication is ready and Icinga 2 listens on port 5665

The child node setup which is run with CLI commands will now
attempt to connect to the parent node. This is not necessarily
the signing master instance, but could also be a parent satellite node.

During this process the child node asks the user to verify the
parent node's public certificate to prevent MITM attacks.

There are two methods to request signed certificates:

* Add the ticket into the request. This ticket was generated on the master
beforehand and contains hashed details for which client it has been created.
The signing master uses this information to automatically sign the certificate
request.

* Do not add a ticket into the request. It will be sent to the signing master
which stores the pending request. Manual user interaction with CLI commands
is necessary to sign the request.

The certificate request is sent as `pki::RequestCertificate` cluster
message to the parent node.

If the parent node is not the signing master, it stores the request
in `/var/lib/icinga2/certificate-requests` and forwards the
cluster message to its parent node.

Once the message arrives on the signing master, it first verifies that
the sent certificate request is valid. This is to prevent unwanted errors
or modified requests from the "proxy" node.

After verification, the signing master checks if the request contains
a valid signing ticket. It hashes the certificate's common name and
compares the value to the received ticket number.

If the ticket is valid, the certificate request is immediately signed
with CA key. The request is sent back to the client inside a `pki::UpdateCertificate`
cluster message.

If the child node was not the certificate request origin, it only updates
the cached request for the child node and send another cluster message
down to its child node (e.g. from a satellite to a client).


If no ticket was specified, the signing master waits until the
`ca sign` CLI command manually signed the certificate.

> **Note**
>
> Push notifications for manual request signing is not yet implemented (TODO).

Once the child node reconnects it synchronizes all signed certificate requests.
This takes some minutes and requires all nodes to reconnect to each other.


#### CSR Signing: Clients without parent connection <a id="technical-concepts-cluster-csr-signing-clients-no-connection"></a>

There is an additional scenario: The setup on a child node does
not necessarily need a connection to the parent node.

This mode leaves the node in a semi-configured state. You need
to manually copy the master's public CA key into `/var/lib/icinga2/certs/ca.crt`
on the client before starting Icinga 2.

The parent node needs to actively connect to the child node.
Once this connections succeeds, the child node will actively
request a signed certificate.

The update procedure works the same way as above.

### High Availability <a id="technical-concepts-cluster-ha"></a>

High availability is automatically enabled between two nodes in the same
cluster zone.

This requires the same configuration and enabled features on both nodes.

HA zone members trust each other and share event updates as cluster messages.
This includes for example check results, next check timestamp updates, acknowledgements
or notifications.

This ensures that both nodes are synchronized. If one node goes away, the
remaining node takes over and continues as normal.


Cluster nodes automatically determine the authority for configuration
objects. This results in activated but paused objects. You can verify
that by querying the `paused` attribute for all objects via REST API
or debug console.

Nodes inside a HA zone calculate the object authority independent from each other.

The number of endpoints in a zone is defined through the configuration. This number
is used inside a local modulo calculation to determine whether the node feels
responsible for this object or not.

This object authority is important for selected features explained below.

Since features are configuration objects too, you must ensure that all nodes
inside the HA zone share the same enabled features. If configured otherwise,
one might have a checker feature on the left node, nothing on the right node.
This leads to late check results because one half is not executed by the right
node which holds half of the object authorities.

### High Availability: Checker <a id="technical-concepts-cluster-ha-checker"></a>

The `checker` feature only executes checks for `Checkable` objects (Host, Service)
where it is authoritative.

That way each node only executes checks for a segment of the overall configuration objects.

The cluster message routing ensures that all check results are synchronized
to nodes which are not authoritative for this configuration object.


### High Availability: Notifications <a id="technical-concepts-cluster-notifications"></a>

The `notification` feature only sends notifications for `Notification` objects
where it is authoritative.

That way each node only executes notifications for a segment of all notification objects.

Notified users and other event details are synchronized throughout the cluster.
This is required if for example the DB IDO feature is active on the other node.

### High Availability: DB IDO <a id="technical-concepts-cluster-ha-ido"></a>

If you don't have HA enabled for the IDO feature, both nodes will
write their status and historical data to their own separate database
backends.

In order to avoid data separation and a split view (each node would require its
own Icinga Web 2 installation on top), the high availability option was added
to the DB IDO feature. This is enabled by default with the `enable_ha` setting.

This requires a central database backend. Best practice is to use a MySQL cluster
with a virtual IP.

Both Icinga 2 nodes require the connection and credential details configured in
their DB IDO feature.

During startup Icinga 2 calculates whether the feature configuration object
is authoritative on this node or not. The order is an alpha-numeric
comparison, e.g. if you have `master1` and `master2`, Icinga 2 will enable
the DB IDO feature on `master2` by default.

If the connection between endpoints drops, the object authority is re-calculated.

In order to prevent data duplication in a split-brain scenario where both
nodes would write into the same database, there is another safety mechanism
in place.

The split-brain decision which node will write to the database is calculated
from a quorum inside the `programstatus` table. Each node
verifies whether the `endpoint_name` column is not itself on database connect.
In addition to that the DB IDO feature compares the `last_update_time` column
against the current timestamp plus the configured `failover_timeout` offset.

That way only one active DB IDO feature writes to the database, even if they
are not currently connected in a cluster zone. This prevents data duplication
in historical tables.

### Health Checks <a id="technical-concepts-cluster-health-checks"></a>

#### cluster-zone <a id="technical-concepts-cluster-health-checks-cluster-zone"></a>

This built-in check provides the possibility to check for connectivity between
zones.

If you for example need to know whether the `master` zone is connected and processing
messages with the child zone called `satellite` in this example, you can configure
the [cluster-zone](10-icinga-template-library.md#itl-icinga-cluster-zone) check as new service on all `master` zone hosts.

```
vim /etc/zones.d/master/host1.conf

object Service "cluster-zone-satellite" {
  check_command = "cluster-zone"
  host_name = "host1"

  vars.cluster_zone = "satellite"
}
```

The check itself changes to NOT-OK if one or more child endpoints in the child zone
are not connected to parent zone endpoints.

In addition to the overall connectivity check, the log lag is calculated based
on the to-be-sent replay log. Each instance stores that for its configured endpoint
objects.

This health check iterates over the target zone (`cluster_zone`) and their endpoints.

The log lag is greater than zero if

* the replay log synchronization is in progress and not yet finished or
* the endpoint is not connected, and no replay log sync happened (obviously).

The final log lag value is the worst value detected. If satellite1 has a log lag of
`1.5` and satellite2 only has `0.5`, the computed value will be `1.5.`.

You can control the check state by using optional warning and critical thresholds
for the log lag value.

If this service exists multiple times, e.g. for each master host object, the log lag
may differ based on the execution time. This happens for example on restart of
an instance when the log replay is in progress and a health check is executed at different
times.
If the endpoint is not connected, both master instances may have saved a different log replay
position from the last synchronisation.

The lag value is returned as performance metric key `slave_lag`.

Icinga 2 v2.9+ adds more performance metrics for these values:

* `last_messages_sent` and `last_messages_received` as UNIX timestamp
* `sum_messages_sent_per_second` and `sum_messages_received_per_second`
* `sum_bytes_sent_per_second` and `sum_bytes_received_per_second`


<!--
## REST API <a id="technical-concepts-rest-api"></a>

Icinga 2 provides its own HTTP server which shares the port 5665 with
the JSON-RPC cluster protocol.
-->
