# Technical Concepts <a id="technical-concepts"></a>

This chapter provides technical concepts and design insights
into specific Icinga 2 components such as:

* [Application](19-technical-concepts.md#technical-concepts-application)
* [Configuration](19-technical-concepts.md#technical-concepts-configuration)
* [Features](19-technical-concepts.md#technical-concepts-features)
* [Check Scheduler](19-technical-concepts.md#technical-concepts-check-scheduler)
* [Cluster](19-technical-concepts.md#technical-concepts-cluster)
* [TLS Network IO](19-technical-concepts.md#technical-concepts-tls-network-io)

## Application <a id="technical-concepts-application"></a>

### CLI Commands <a id="technical-concepts-application-cli-commands"></a>

The Icinga 2 application is managed with different CLI sub commands.
`daemon` takes care about loading the configuration files, running the
application as daemon, etc.
Other sub commands allow to enable features, generate and request
TLS certificates or enter the debug console.

The main entry point for each CLI command parses the command line
parameters and then triggers the required actions.

### daemon CLI command <a id="technical-concepts-application-cli-commands-daemon"></a>

This CLI command loads the configuration files, starting with `icinga2.conf`.
The [configuration compiler](19-technical-concepts.md#technical-concepts-configuration) parses the
file and detects additional file includes, constants, and any other DSL
specific declaration.

At this stage, the configuration will already be checked against the
defined grammar in the scanner, and custom object validators will also be
checked.

If the user provided `-C/--validate`, the CLI command returns with the
validation exit code.

When running as daemon, additional parameters are checked, e.g. whether
this application was triggered by a reload, needs to daemonize with fork()
involved and update the object's authority. The latter is important for
HA-enabled cluster zones.

## Configuration <a id="technical-concepts-configuration"></a>

### Lexer <a id="technical-concepts-configuration-lexer"></a>

The lexer stage does not understand the DSL itself, it only
maps specific character sequences into identifiers.

This allows Icinga to detect the beginning of a string with `"`,
reading the following characters and determining the end of the
string with again `"`.

Other parts covered by the lexer a escape sequences insides a string,
e.g. `"\"abc"`.

The lexer also identifiers logical operators, e.g. `&` or `in`,
specific keywords like `object`, `import`, etc. and comment blocks.

Please check `lib/config/config_lexer.ll` for details.

Icinga uses [Flex](https://github.com/westes/flex) in the first stage.

> Flex (The Fast Lexical Analyzer)
>
> Flex is a fast lexical analyser generator. It is a tool for generating programs
> that perform pattern-matching on text. Flex is a free (but non-GNU) implementation
> of the original Unix lex program.

### Parser <a id="technical-concepts-configuration-parser"></a>

The parser stage puts the identifiers from the lexer into more
context with flow control and sequences.

The following comparison is parsed into a left term, an operator
and a right term.

```
x > 5
```

The DSL contains many elements which require a specific order,
and sometimes only a left term for example.

The parser also takes care of parsing an object declaration for
example. It already knows from the lexer that `object` marks the
beginning of an object. It then expects a type string afterwards,
and the object name - which can be either a string with double quotes
or a previously defined constant.

An opening bracket `{` in this specific context starts the object
scope, which also is stored for later scope specific variable access.

If there's an apply rule defined, this follows the same principle.
The config parser detects the scope of an apply rule and generates
Icinga 2 C++ code for the parsed string tokens.

```
assign where host.vars.sla == "24x7"
```

is parsed into an assign token identifier, and the string expression
is compiled into a new `ApplyExpression` object.

The flow control inside the parser ensures that for example `ignore where`
can only be defined when a previous `assign where` was given - or when
inside an apply for rule.

Another example are specific object types which allow assign expression,
specifically group objects. Others objects must throw a configuration error.

Please check `lib/config/config_parser.yy` for more details,
and the [language reference](17-language-reference.md#language-reference) chapter for
documented DSL keywords and sequences.

> Icinga uses [Bison](https://en.wikipedia.org/wiki/GNU_bison) as parser generator
> which reads a specification of a context-free language, warns about any parsing
> ambiguities, and generates a parser in C++ which reads sequences of tokens and
> decides whether the sequence conforms to the syntax specified by the grammar.


### Compiler <a id="technical-concepts-configuration-compiler"></a>

The config compiler initializes the scanner inside the [lexer](19-technical-concepts.md#technical-concepts-configuration-lexer)
stage.

The configuration files are parsed into memory from inside the [daemon CLI command](19-technical-concepts.md#technical-concepts-application-cli-commands-daemon)
which invokes the config validation in `ValidateConfigFiles()`. This compiles the
files into an AST expression which is executed.

At this stage, the expressions generate so-called "config items" which
are a pre-stage of the later compiled object.

`ConfigItem::CommitItems` takes care of committing the items, and doing a
rollback on failure. It also checks against matching apply rules from the previous run
and generates statistics about the objects which can be seen by the config validation.

`ConfigItem::CommitNewItems` collects the registered types and items,
and checks for a specific required order, e.g. a service object needs
a host object first.

The following stages happen then:

- **Commit**: A workqueue then commits the items in a parallel fashion for this specific type. The object gets its name, and the AST expression is executed. It is then registered into the item into `m_Object` as reference.
- **OnAllConfigLoaded**: Special signal for each object to pre-load required object attributes, resolve group membership, initialize functions and timers.
- **CreateChildObjects**: Run apply rules for this specific type.
- **CommitNewItems**: Apply rules may generate new config items, this is to ensure that they again run through the stages.

Note that the items are now committed and the configuration is validated and loaded
into memory. The final config objects are not yet activated though.

This only happens after the validation, when the application is about to be run
with `ConfigItem::ActivateItems`.

Each item has an object created in `m_Object` which is checked in a loop.
Again, the dependency order of activated objects is important here, e.g. logger features come first, then
config objects and last the checker, api, etc. features. This is done by sorting the objects
based on their type specific activation priority.

The following signals are triggered in the stages:

- **PreActivate**: Setting the `active` flag for the config object.
- **Activate**: Calls `Start()` on the object, sets the local HA authority and notifies subscribers that this object is now activated (e.g. for config updates in the DB backend).



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

Since this check result signal is blocking, many of the features include a work queue
with asynchronous task handling.

The GraphiteWriter uses a TCP socket to communicate with the carbon cache
daemon of Graphite. The InfluxDBWriter is instead writing bulk metric messages
to InfluxDB's HTTP API, similar to Elasticsearch.


## Check Scheduler <a id="technical-concepts-check-scheduler"></a>

The check scheduler starts a thread which loops forever. It waits for
check events being inserted into `m_IdleCheckables`.

If the current pending check event number is larger than the configured
max concurrent checks, the thread waits up until it there's slots again.

In addition, further checks on enabled checks, check periods, etc. are
performed. Once all conditions have passed, the next check timestamp is
calculated and updated. This also is the timestamp where Icinga expects
a new check result ("freshness check").

The object is removed from idle checkables, and inserted into the
pending checkables list. This can be seen via REST API metrics for the
checker component feature as well.

The actual check execution happens asynchronously using the application's
thread pool.

Once the check returns, it is removed from pending checkables and again
inserted into idle checkables. This ensures that the scheduler takes this
checkable event into account in the next iteration.

### Start <a id="technical-concepts-check-scheduler-start"></a>

When checkable objects get activated during the startup phase,
the checker feature registers a handler for this event. This is due
to the fact that the `checker` feature is fully optional, and e.g. not
used on command endpoint clients.

Whenever such an object activation signal is triggered, Icinga 2 checks
whether it is [authoritative for this object](19-technical-concepts.md#technical-concepts-cluster-ha-object-authority).
This means that inside an HA enabled zone with two endpoints, only non-paused checkable objects are
actively inserted into the idle checkable list for the check scheduler.

### Initial Check <a id="technical-concepts-check-scheduler-initial"></a>

When a new checkable object (host or service) is initially added to the
configuration, Icinga 2 performs the following during startup:

* `Checkable::Start()` is called and calculates the first check time
* With a spread delta, the next check time is actually set.

If the next check should happen within a time frame of 60 seconds,
Icinga 2 calculates a delta from a random value. The minimum of `check_interval`
and 60 seconds is used as basis, multiplied with a random value between 0 and 1.

In the best case, this check gets immediately executed after application start.
The worst case scenario is that the check is scheduled 60 seconds after start
the latest.

The reasons for delaying and spreading checks during startup is that
the application typically needs more resources at this time (cluster connections,
feature warmup, initial syncs, etc.). Immediate check execution with
thousands of checks could lead into performance problems, and additional
events for each received check results.

Therefore the initial check window is 60 seconds on application startup,
random seed for all checkables. This is not predictable over multiple restarts
for specific checkable objects, the delta changes every time.

### Scheduling Offset <a id="technical-concepts-check-scheduler-offset"></a>

There's a high chance that many checkable objects get executed at the same time
and interval after startup. The initial scheduling spreads that a little, but
Icinga 2 also attempts to ensure to keep fixed intervals, even with high check latency.

During startup, Icinga 2 calculates the scheduling offset from a random number:

* `Checkable::Checkable()` calls `SetSchedulingOffset()` with `Utility::Random()`
* The offset is a pseudo-random integral value between `0` and `RAND_MAX`.

Whenever the next check time is updated with `Checkable::UpdateNextCheck()`,
the scheduling offset is taken into account.

Depending on the state type (SOFT or HARD), either the `retry_interval` or `check_interval`
is used. If the interval is greater than 1 second, the time adjustment is calculated in the
following way:

`now * 100 + offset` divided by `interval * 100`, using the remainder (that's what `fmod()` is for)
and dividing this again onto base 100.

Example: offset is 6500, interval 300, now is 1542190472.

```
1542190472 * 100 + 6500 = 154219053714
300 * 100 = 30000
154219053714 / 30000 = 5140635.1238

(5140635.1238 - 5140635.0) * 30000 = 3714
3714 / 100 = 37.14
```

37.15 seconds as an offset would be far too much, so this is again used as a calculation divider for the
real offset with the base of 5 times the actual interval.

Again, the remainder is calculated from the offset and `interval * 5`. This is divided onto base 100 again,
with an additional 0.5 seconds delay.

Example: offset is 6500, interval 300.

```
6500 / 300 = 21.666666666666667
(21.666666666666667 - 21.0) * 300 = 200
200 / 100 = 2
2 + 0.5 = 2.5
```

The minimum value between the first adjustment and the second offset calculation based on the interval is
taken, in the above example `2.5` wins.

The actual next check time substracts the adjusted time from the future interval addition to provide
a more widespread scheduling time among all checkable objects.

`nextCheck = now - adj + interval`

You may ask, what other values can happen with this offset calculation. Consider calculating more examples
with different interval settings.

Example: offset is 34567, interval 60, now is 1542190472.

```
1542190472 * 100 + 34567 = 154219081767
60 * 100 = 6000
154219081767 / 6000 = 25703180.2945
(25703180.2945 - 25703180.0) * 6000 / 100 = 17.67

34567 / 60 = 576.116666666666667
(576.116666666666667 - 576.0) * 60 / 100 + 0.5 = 1.2
```

`1m` interval starts at `now + 1.2s`.

Example: offset is 12345, interval 86400, now is 1542190472.

```
1542190472 * 100 + 12345 = 154219059545
86400 * 100 = 8640000
154219059545 / 8640000 = 17849.428188078703704
(17849.428188078703704 - 17849) * 8640000 = 3699545
3699545 / 100 = 36995.45

12345 / 86400 = 0.142881944444444
0.142881944444444 * 86400 / 100 + 0.5 = 123.95
```

`1d` interval starts at `now + 2m4s`.

> **Note**
>
> In case you have a better algorithm at hand, feel free to discuss this in a PR on GitHub.
> It needs to fulfill two things: 1) spread and shuffle execution times on each `next_check` update
> 2) not too narrowed window for both long and short intervals
> Application startup and initial checks need to be handled with care in a slightly different
> fashion.

When `SetNextCheck()` is called, there are signals registered. One of them sits
inside the `CheckerComponent` class whose handler `CheckerComponent::NextCheckChangedHandler()`
deletes/inserts the next check event from the scheduling queue. This basically
is a list with multiple indexes with the keys for scheduling info and the object.


### Check Latency and Execution Time <a id="technical-concepts-check-scheduler-latency"></a>

Each check command execution logs the start and end time where
Icinga 2 (and the end user) is able to calculate the plugin execution time from it.

```
GetExecutionEnd() - GetExecutionStart()
```

The higher the execution time, the higher the command timeout must be set. Furthermore
users and developers are encouraged to look into plugin optimizations to minimize the
execution time. Sometimes it is better to let an external daemon/script do the checks
and feed them back via REST API.

Icinga 2 stores the scheduled start and end time for a check. If the actual
check execution time differs from the scheduled time, e.g. due to performance
problems or limited execution slots (concurrent checks), this value is stored
and computed from inside the check result.

The difference between the two deltas is called `check latency`.

```
(GetScheduleEnd() - GetScheduleStart()) - CalculateExecutionTime()
```


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

General high availability is automatically enabled between two endpoints in the same
cluster zone.

**This requires the same configuration and enabled features on both nodes.**

HA zone members trust each other and share event updates as cluster messages.
This includes for example check results, next check timestamp updates, acknowledgements
or notifications.

This ensures that both nodes are synchronized. If one node goes away, the
remaining node takes over and continues as normal.

#### High Availability: Object Authority <a id="technical-concepts-cluster-ha-object-authority"></a>

Cluster nodes automatically determine the authority for configuration
objects. By default, all config objects are set to `HARunEverywhere` and
as such the object authority is true for any config object on any instance.

Specific objects can override and influence this setting, e.g. with `HARunOnce`
instead prior to config object activation.

This is done when the daemon starts and in a regular interval inside
the ApiListener class, specifically calling `ApiListener::UpdateObjectAuthority()`.

The algorithm works like this:

* Determine whether this instance is assigned to a local zone and endpoint.
* Collects all endpoints in this zone if they are connected.
* If there's two endpoints, but only us seeing ourselves and the application start is less than 60 seconds in the past, do nothing (wait for cluster reconnect to take place, grace period).
* Sort the collected endpoints by name.
* Iterate over all config types and their respective objects
 * Ignore !active objects
 * Ignore objects which are !HARunOnce. This means, they can run multiple times in a zone and don't need an authority update.
 * If this instance doesn't have a local zone, set authority to true. This is for non-clustered standalone environments where everything belongs to this instance.
 * Calculate the object authority based on the connected endpoint names.
 * Set the authority (true or false)

The object authority calculation works "offline" without any message exchange.
Each instance alculates the SDBM hash of the config object name, puts that in contrast
modulo the connected endpoints size.
This index is used to lookup the corresponding endpoint in the connected endpoints array,
including the local endpoint. Whether the local endpoint is equal to the selected endpoint,
or not, this sets the authority to `true` or `false`.

```
authority = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()] == my_endpoint;
```

`ConfigObject::SetAuthority(bool authority)` triggers the following events:

* Authority is true and object now paused: Resume the object and set `paused` to `false`.
* Authority is false, object not paused: Pause the object and set `paused` to true.

**This results in activated but paused objects on one endpoint.** You can verify
that by querying the `paused` attribute for all objects via REST API
or debug console on both endpoints.

Endpoints inside a HA zone calculate the object authority independent from each other.
This object authority is important for selected features explained below.

Since features are configuration objects too, you must ensure that all nodes
inside the HA zone share the same enabled features. If configured otherwise,
one might have a checker feature on the left node, nothing on the right node.
This leads to late check results because one half is not executed by the right
node which holds half of the object authorities.

By default, features are enabled to "Run-Everywhere". Specific features which
support HA awareness, provide the `enable_ha` configuration attribute. When `enable_ha`
is set to `true` (usually the default), "Run-Once" is set and the feature pauses on one side.

```
vim /etc/icinga2/features-enabled/graphite.conf

object GraphiteWriter "graphite" {
  ...
  enable_ha = true
}
```

Once such a feature is paused, there won't be any more event handling, e.g. the Elasticsearch
feature won't process any checkresults nor write to the Elasticsearch REST API.

When the cluster connection drops, the feature configuration object is updated with
the new object authority by the ApiListener timer and resumes its operation. You can see
that by grepping the log file for `resumed` and `paused`.

```
[2018-10-24 13:28:28 +0200] information/GraphiteWriter: 'g-ha' paused.
```

```
[2018-10-24 13:28:28 +0200] information/GraphiteWriter: 'g-ha' resumed.
```

Specific features with HA capabilities are explained below.

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


## TLS Network IO <a id="technical-concepts-tls-network-io"></a>

### TLS Connection Handling <a id="technical-concepts-tls-network-io-connection-handling"></a>

TLS-Handshake timeouts occur if the server is busy with reconnect handling and other tasks which run in isolated threads. Icinga 2 uses threads in many ways, e.g. for timers to wake them up, wait for check results, etc.

In terms of the cluster communication, the following flow applies.

#### Master Connects <a id="technical-concepts-tls-network-io-connection-handling-master"></a>

* The master initializes the connection in a loop through all known zones it should connect to, extracting the endpoints and their host/port attribute.
* This calls `AddConnection()` whereas a `Tcp::Connect()` is called to create a TCP socket.
* A new thread is spawned for future connection handling, this binds `ApiListener::NewClientHandler()`.
* On top of the TCP socket, a new TLS stream is created.
* The master performs a `TLS->Handshake()`
* Certificates are verified and the endpoint name is compared to the CN.


#### Clients Processes Connection <a id="technical-concepts-tls-network-io-connection-handling-client"></a>

* The client listens for new incoming connections as 'TCP server' pattern inside `ListenerThreadProc()` with an endless loop.
* Once a new connection is detected, `TCP->Accept()` performs the initial socket establishment.
* A new thread is spawned for future connection handling, this binds `ApiListener::NewClientHandler()`, Role being Server.
* On top of the TCP socket, a new TLS stream is created.
* The client performs a `TLS->Handshake()`.


#### Data Transmission between Server and Client Role <a id="technical-concepts-tls-network-io-connection-handling-data-transmission"></a>

Once the TLS handshake and certificate verification is completed, the role is either `Client` or `Server`.

* Client: Send "Hello" message.
* Server: `TLS->WaitForData()` waits for incoming messages from the remote client.

`Client` in this case is the instance which initiated the connection. If the master is doing this,
the Icinga 2 client/agent acts as "server" which accepts incoming connections.


### Asynchronous Socket IO <a id="technical-concepts-tls-network-io-async-socket-io"></a>

Everything runs through TLS, we don't use any "raw" connections nor plain message handling.

The TLS handshake and further read/write operations are not performed in a synchronous fashion
in the new client's thread. Instead, all clients share an asynchronous "event pool".

The TlsStream constructor registers a new SocketEvent by calling its constructor. It binds the
previously created TCP socket and itself into the created SocketEvent object.

`SocketEvent::InitializeEngine()` takes care of whether to use **epoll** (Linux) or
**poll** (BSD, Unix, Windows) as preferred socket poll engine. epoll has proven to be
faster on Linux systems.

The selected engine is stored as `l_SocketIOEngine` and later `Start()` ensures to do the following:

* Use a fixed number for creating IO threads.
* Create a `dumb_socketpair` which basically is a pipe from `in->out` and multiplexes the TCP socket
into a local Unix socket. This removes the complexity and slowlyness of the kernel dealing with the TCP stack and new events.
* `InitializeThread()` prepares epoll with `epoll_create`, socket descriptors and event mapping for later wakeup.
* Each event FD has its own "worker event thread" which deals with incoming data, called `ThreadProc` as endless loop.

By default, there are 8 of these worker threads.

In the `ThreadProc` loop, the following happens:

* `epoll_wait` gets called and provides an event whether new data is `ready` (via socket IO from the Kernel).
* The event created with `epoll_event` holds the `.fd.data` attribute which references the multiplexed event FD (and therefore tcp socket FD).
* All events in this cycle are stored with their descriptors in a list.
* Once the epoll loop is finished, the collected events are processed and the socketevent descriptor (which is the TlsStream object) calls `OnEvent()`.

#### On Socket Event State Machine <a id="technical-concepts-tls-network-io-async-socket-io-on-event"></a>

`OnEvent` implements the "state machine" depending on the current desired action. By default, this is `TlsActionNone`.

Once `TlsStream->Handshake()` is called, this initializes the current action to
`TlsActionHandshake` and performs `SSL_do_handshake()`. This function returns > 0
when successful, anything below needs to be dealt separately.

If the handshake was successful, the registered condition variable `m_CV` gets signalled
and the thread waiting for the handshake in `TlsStream->Handshake()` wakes up and continues
within the `ApiListener::NewClientHandler()` function.

Once the handshake is completed, current action is changed to either `TlsActionRead` or `TlsActionWrite`.
This happens in the beginning of the state machine when there is no action selected yet.

* **Read**: Received events indicate POLLIN (or POLLERR/POLLHUP as error, but normally mean "read").
* **Write**: The send buffer of the TLS stream is greater 0 bytes, and the received events allow POLLOUT on the event socket.
* Nothing matched: Change the event sockets to POLLIN ("read"), and return, waiting for the next event.

This also depends on the returned error codes of the SSL interface functions. Whenever `SSL_WANT_READ` occurs,
the event polling needs be changed to use `POLLIN`, vice versa for `SSL_WANT_WRITE` and `POLLOUT`.

In the scenario where the master actively connects to the clients, the client will wait for data and
change the event sockets to `Read` once there's something coming on the sockets.

Action         | Description
---------------|---------------
Read           | Calls `SSL_read()` with a fixed buffer size of 64 KB. If rc > 0, the receive buffer of the TLS stream is filled and success indicated. This endless loop continues until a) `SSL_pending()` says no more data from remote b) Maximum bytes are read. If `success` is true, the condition variable notifies the thread in `WaitForData` to wake up.
Write          | The send buffer of the TLS stream `Peek()`s the first 64KB and calls `SSL_write()` to send them over the socket. The returned value is the number of bytes written, this is adjusted within the send buffer in the `Read()` call (it also optimizes the memory usage).
Handshake      | Calls `SSL_do_handshake()` and if successful, the condition variable wakes up the thread waiting for it in `Handshake()`.

##### TLS Error Handling

TLS error code           | Description
-------------------------|-------------------------
`SSL_WANT_READ`          | The next event should read again, change events to `POLLIN`.
`SSL_ERROR_WANT_WRITE`   | The next event should write, change events to `POLLOUT`.
`SSL_ERROR_ZERO_RETURN`  | Nothing was returned, close the TLS stream and immediately return.
default                  | Extract the error code and log a fancy error for the user. Close the connection.

From this [question](https://stackoverflow.com/questions/3952104/how-to-handle-openssl-ssl-error-want-read-want-write-on-non-blocking-sockets):

```
With non-blocking sockets, SSL_WANT_READ means "wait for the socket to be readable, then call this function again."; conversely, SSL_WANT_WRITE means "wait for the socket to be writeable, then call this function again.". You can get either SSL_WANT_WRITE or SSL_WANT_READ from both an SSL_read() or SSL_write() call.
```


##### Successful TLS Actions

* Initialize the next TLS action to `none`. This re-evaluates the conditions upon next event call.
* If the stream still contains data, adjust the socket events.
  * If the send buffer contains data, change events to `POLLIN|POLLOUT`.
  * Otherwise `POLLIN` to wait for data.
* Process data when the receive buffer has them available and we are actively handling events.
* If the TLS stream is supposed to shutdown, close everything including the TLS connection.

#### Data Processing <a id="technical-concepts-tls-network-io-async-socket-io-data-processing"></a>

Once a stream has data available, it calls `SignalDataAvailable()`. This holds a condition
variable which wakes up another thread in a handled which was previously registered, e.g.
for JsonRpcConnection, HttpServerConnection or HttpClientConnection objects.

All of them read data from the stream and process the messages. At this point the string is available as JSON already and later decoded (e.g. Icinga data structures, as Dictionary).



### General Design Patterns <a id="technical-concepts-tls-network-io-design-patterns"></a>

Taken from https://www.ibm.com/developerworks/aix/library/au-libev/index.html

```
One of the biggest problems facing many server deployments, particularly web server deployments, is the ability to handle a large number of connections. Whether you are building cloud-based services to handle network traffic, distributing your application over IBM Amazon EC instances, or providing a high-performance component for your web site, you need to be able to handle a large number of simultaneous connections.

A good example is the recent move to more dynamic web applications, especially those using AJAX techniques. If you are deploying a system that allows many thousands of clients to update information directly within a web page, such as a system providing live monitoring of an event or issue, then the speed at which you can effectively serve the information is vital. In a grid or cloud situation, you might have permanent open connections from thousands of clients simultaneously, and you need to be able to serve the requests and responses to each client.

Before looking at how libevent and libev are able to handle multiple network connections, let's take a brief look at some of the traditional solutions for handling this type of connectivity.

### Handling multiple clients

There are a number of different traditional methods that handle multiple connections, but usually they result in an issue handling large quantities of connections, either because they use too much memory, too much CPU, or they reach an operating system limit of some kind.

The main solutions used are:

* Round-robin: The early systems use a simple solution of round-robin selection, simply iterating over a list of open network connections and determining whether there is any data to read. This is both slow (especially as the number of connections increases) and inefficient (since other connections may be sending requests and expecting responses while you are servicing the current one). The other connections have to wait while you iterate through each one. If you have 100 connections and only one has data, you still have to work through the other 99 to get to the one that needs servicing.
* poll, epoll, and variations: This uses a modification of the round-robin approach, using a structure to hold an array of each of the connections to be monitored, with a callback mechanism so that when data is identified on a network socket, the handling function is called. The problem with poll is that the size of the structure can be quite large, and modifying the structure as you add new network connections to the list can increase the load and affect performance.
* select: The select() function call uses a static structure, which had previously been hard-coded to a relatively small number (1024 connections), which makes it impractical for very large deployments.
There are other implementations on individual platforms (such as /dev/poll on Solaris, or kqueue on FreeBSD/NetBSD) that may perform better on their chosen OS, but they are not portable and don't necessarily resolve the upper level problems of handling requests.

All of the above solutions use a simple loop to wait and handle requests, before dispatching the request to a separate function to handle the actual network interaction. The key is that the loop and network sockets need a lot of management code to ensure that you are listening, updating, and controlling the different connections and interfaces.

An alternative method of handling many different connections is to make use of the multi-threading support in most modern kernels to listen and handle connections, opening a new thread for each connection. This shifts the responsibility back to the operating system directly but implies a relatively large overhead in terms of RAM and CPU, as each thread will need it's own execution space. And if each thread (ergo network connection) is busy, then the context switching to each thread can be significant. Finally, many kernels are not designed to handle such a large number of active threads.
```


### Alternative Implementations and Libraries <a id="technical-concepts-tls-network-io-async-socket-io-alternatives"></a>

While analysing Icinga 2's socket IO event handling, the libraries and implementations
below have been collected too. [This thread](https://www.reddit.com/r/cpp/comments/5xxv61/a_modern_c_network_library_for_developing_high/)
also sheds more light in modern programming techniques.

Our main "problem" with Icinga 2 are modern compilers supporting the full C++11 feature set.
Recent analysis have proven that gcc on CentOS 6 or SLES11 are too old to use modern
programming techniques or anything which implemens C++14 at least.

Given the below projects, we are also not fans of wrapping C interfaces into
C++ code in case you want to look into possible patches.

One key thing for external code is [license compatibility](http://gplv3.fsf.org/wiki/index.php/Compatible_licenses#GPLv2-compatible_licenses) with GPLv2.
Modified BSD and Boost can be pulled into the `third-party/` directory, best header only and compiled
into the Icinga 2 binary.

#### C

* libevent: http://www.wangafu.net/~nickm/libevent-book/TOC.html
* libev: https://www.ibm.com/developerworks/aix/library/au-libev/index.html
* libuv: http://libuv.org

#### C++

* Asio (standalone header only or as Boost library): http://think-async.com (the Boost Software license is compatible with GPLv2)
* Poco project: https://github.com/pocoproject/poco
* cpp-netlib: https://github.com/cpp-netlib/cpp-netlib
* evpp: https://github.com/Qihoo360/evpp

