# Technical Concepts <a id="technical-concepts"></a>

This chapter provides technical concepts and design insights
into specific Icinga 2 components such as:

* [Application](19-technical-concepts.md#technical-concepts-application)
* [Configuration](19-technical-concepts.md#technical-concepts-configuration)
* [Features](19-technical-concepts.md#technical-concepts-features)
* [Check Scheduler](19-technical-concepts.md#technical-concepts-check-scheduler)
* [Checks](19-technical-concepts.md#technical-concepts-checks)
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


### References <a id="technical-concepts-configuration-references"></a>

* [The Icinga Config Compiler: An Overview](https://www.netways.de/blog/2018/07/12/the-icinga-config-compiler-an-overview/)
* [A parser/lexer/compiler for the Leonardo language](https://github.com/EmilGedda/Leonardo)
* [I wrote a programming language. Here’s how you can, too.](https://medium.freecodecamp.org/the-programming-language-pipeline-91d3f449c919)
* [http://onoffswitch.net/building-a-custom-lexer/](http://onoffswitch.net/building-a-custom-lexer/)
* [Writing an Interpreter with Lex, Yacc, and Memphis](http://memphis.compilertools.net/interpreter.html)
* [Flex](https://github.com/westes/flex)
* [GNU Bison](https://www.gnu.org/software/bison/)

## Core <a id="technical-concepts-core"></a>

### Core: Reload Handling <a id="technical-concepts-core-reload"></a>

The initial design of the reload state machine looks like this:

* receive reload signal SIGHUP
* fork a child process, start configuration validation in parallel work queues
* parent process continues with old configuration objects and the event scheduling
(doing checks, replicating cluster events, triggering alert notifications, etc.)
* validation NOT ok: child process terminates, parent process continues with old configuration state
* validation ok: child process signals parent process to terminate and save its current state (all events until now) into the icinga2 state file
* parent process shuts down writing icinga2.state file
* child process waits for parent process gone, reads the icinga2 state file and synchronizes all historical and status data
* child becomes the new session leader

Since Icinga 2.6, there are two processes when checked with `ps aux | grep icinga2` or `pidof icinga2`.
This was to ensure that feature file descriptors don't leak into the plugin process (e.g. DB IDO MySQL sockets).

Icinga 2.9 changed the reload handling a bit with SIGUSR2 signals
and systemd notifies.

With systemd, it could occur that the tree was broken thus resulting
in killing all remaining processes on stop, instead of a clean exit.
You can read the full story [here](https://github.com/Icinga/icinga2/issues/7309).

With 2.11 you'll now see 3 processes:

- The umbrella process which takes care about signal handling and process spawning/stopping
- The main process with the check scheduler, notifications, etc.
- The execution helper process

During reload, the umbrella process spawns a new reload process which validates the configuration.
Once successful, the new reload process signals the umbrella process that it is finished.
The umbrella process forwards the signal and tells the old main process to shutdown.
The old main process writes the icinga2.state file. The umbrella process signals
the reload process that the main process terminated.

The reload process was in idle wait before, and now continues to read the written
state file and run the event loop (checks, notifications, "events", ...). The reload
process itself also spawns the execution helper process again.


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


## Checks<a id="technical-concepts-checks"></a>

### Check Latency and Execution Time <a id="technical-concepts-checks-latency"></a>

Each check command execution logs the start and end time where
Icinga 2 (and the end user) is able to calculate the plugin execution time from it.

```cpp
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

```cpp
(GetScheduleEnd() - GetScheduleStart()) - CalculateExecutionTime()
```

### Severity <a id="technical-concepts-checks-severity"></a>

The severity attribute is introduced with Icinga v2.11 and provides
a bit mask calculated value from specific checkable object states.

The severity value is pre-calculated for visualization interfaces
such as Icinga Web which sorts the problem dashboard by severity by default.

The higher the severity number is, the more important the problem is.
However, the formula can change across Icinga 2 releases.


## Cluster <a id="technical-concepts-cluster"></a>

This documentation refers to technical roles between cluster
endpoints.

- The `server` or `parent` role accepts incoming connection attempts and handles requests
- The `client` role actively connects to remote endpoints receiving config/commands, requesting certificates, etc.

A client role is not necessarily bound to the Icinga agent.
It may also be a satellite which actively connects to the
master.

### Communication <a id="technical-concepts-cluster-communication"></a>

Icinga 2 uses its own certificate authority (CA) by default. The
public and private CA keys can be generated on the signing master.

Each node certificate must be signed by the private CA key.

Note: The following description uses `parent node` and `child node`.
This also applies to nodes in the same cluster zone.

During the connection attempt, a TLS handshake is performed.
If the public certificate of a child node is not signed by the same
CA, the child node is not trusted and the connection will be closed.

If the TLS handshake succeeds, the parent node reads the
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
cluster environments with masters, satellites and agents.

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
down to its child node (e.g. from a satellite to an agent).


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

> **Note**
>
> The `client` in this case can be either a satellite or an agent.

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

```cpp
authority = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()] == my_endpoint;
```

`ConfigObject::SetAuthority(bool authority)` triggers the following events:

* Authority is true and object now paused: Resume the object and set `paused` to `false`.
* Authority is false, object not paused: Pause the object and set `paused` to true.

**This results in activated but paused objects on one endpoint.** You can verify
that by querying the `paused` attribute for all objects via REST API
or debug console on both endpoints.

Endpoints inside an HA zone calculate the object authority independent from each other.
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

#### High Availability: Checker <a id="technical-concepts-cluster-ha-checker"></a>

The `checker` feature only executes checks for `Checkable` objects (Host, Service)
where it is authoritative.

That way each node only executes checks for a segment of the overall configuration objects.

The cluster message routing ensures that all check results are synchronized
to nodes which are not authoritative for this configuration object.


#### High Availability: Notifications <a id="technical-concepts-cluster-notifications"></a>

The `notification` feature only sends notifications for `Notification` objects
where it is authoritative.

That way each node only executes notifications for a segment of all notification objects.

Notified users and other event details are synchronized throughout the cluster.
This is required if for example the DB IDO feature is active on the other node.

#### High Availability: DB IDO <a id="technical-concepts-cluster-ha-ido"></a>

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


### Config Sync <a id="technical-concepts-cluster-config-sync"></a>

The visible feature for the user is to put configuration files in `/etc/icinga2/zones.d/<zonename>`
and have them synced automatically to all involved zones and endpoints.

This not only includes host and service objects being checked
in a satellite zone, but also additional config objects such as
commands, groups, timeperiods and also templates.

Additional thoughts and complexity added:

- Putting files into zone directory names removes the burden to set the `zone` attribute on each object in this directory. This is done automatically by the config compiler.
- Inclusion of `zones.d` happens automatically, the user shouldn't be bothered about this.
- Before the REST API was created, only static configuration files in `/etc/icinga2/zones.d` existed. With the addition of config packages, additional `zones.d` targets must be registered (e.g. used by the Director)
- Only one config master is allowed. This one identifies itself with configuration files in `/etc/icinga2/zones.d`. This is not necessarily the zone master seen in the debug logs, that one is important for message routing internally.
- Objects and templates which cannot be bound into a specific zone (e.g. hosts in the satellite zone) must be made available "globally".
- Users must be able to deny the synchronisation of specific zones, e.g. for security reasons.

#### Config Sync: Config Master <a id="technical-concepts-cluster-config-sync-config-master"></a>

All zones must be configured and included in the `zones.conf` config file beforehand.
The zone names are the identifier for the directories underneath the `/etc/icinga2/zones.d`
directory. If a zone is not configured, it will not be included in the config sync - keep this
in mind for troubleshooting.

When the config master starts, the content of `/etc/icinga2/zones.d` is automatically
included. There's no need for an additional entry in `icinga2.conf` like `conf.d`.
You can verify this by running the config validation on debug level:

```
icinga2 daemon -C -x debug | grep 'zones.d'

[2019-06-19 15:16:19 +0200] notice/ConfigCompiler: Compiling config file: /etc/icinga2/zones.d/global-templates/commands.conf
```

Once the config validation succeeds, the startup routine for the daemon
copies the files into the "production" directory in `/var/lib/icinga2/api/zones`.
This directory is used for all endpoints where Icinga stores the received configuration.
With the exception of the config master retrieving this from `/etc/icinga2/zones.d` instead.

These operations are logged for better visibility.

```
[2019-06-19 15:26:38 +0200] information/ApiListener: Copying 1 zone configuration files for zone 'global-templates' to '/var/lib/icinga2/api/zones/global-templates'.
[2019-06-19 15:26:38 +0200] information/ApiListener: Updating configuration file: /var/lib/icinga2/api/zones/global-templates//_etc/commands.conf
```

The master is finished at this point. Depending on the cluster configuration,
the next iteration is a connected endpoint after successful TLS handshake and certificate
authentication.

It calls `SendConfigUpdate(client)` which sends the [config::Update](19-technical-concepts.md#technical-concepts-json-rpc-messages-config-update)
JSON-RPC message including all required zones and their configuration file content.


#### Config Sync: Receive Config <a id="technical-concepts-cluster-config-sync-receive-config"></a>

The secondary master endpoint and endpoints in a child zone will be connected to the config
master. The endpoint receives the [config::Update](19-technical-concepts.md#technical-concepts-json-rpc-messages-config-update)
JSON-RPC message and processes the content in `ConfigUpdateHandler()`. This method checks
whether config should be accepted. In addition to that, it locks a local mutex to avoid race conditions
with multiple syncs in parallel.

After that, the received configuration content is analysed.

> **Note**
>
> The cluster design allows that satellite endpoints may connect to the secondary master first.
> There is no immediate need to always connect to the config master first, especially since
> the satellite endpoints don't know that.
>
> The secondary master not only stores the master zone config files, but also all child zones.
> This is also the case for any HA enabled zone with more than one endpoint.


2.11 puts the received configuration files into a staging directory in
`/var/lib/icinga2/api/zones-stage`. Previous versions directly wrote the
files into production which could have led to broken configuration on the
next manual restart.

```
[2019-06-19 16:08:29 +0200] information/ApiListener: New client connection for identity 'master1' to [127.0.0.1]:5665
[2019-06-19 16:08:30 +0200] information/ApiListener: Applying config update from endpoint 'master1' of zone 'master'.
[2019-06-19 16:08:30 +0200] information/ApiListener: Received configuration for zone 'agent' from endpoint 'master1'. Comparing the checksums.
[2019-06-19 16:08:30 +0200] information/ApiListener: Stage: Updating received configuration file '/var/lib/icinga2/api/zones-stage/agent//_etc/host.conf' for zone 'agent'.
[2019-06-19 16:08:30 +0200] information/ApiListener: Applying configuration file update for path '/var/lib/icinga2/api/zones-stage/agent' (176 Bytes).
[2019-06-19 16:08:30 +0200] information/ApiListener: Received configuration for zone 'master' from endpoint 'master1'. Comparing the checksums.
[2019-06-19 16:08:30 +0200] information/ApiListener: Applying configuration file update for path '/var/lib/icinga2/api/zones-stage/master' (17 Bytes).
[2019-06-19 16:08:30 +0200] information/ApiListener: Received configuration from endpoint 'master1' is different to production, triggering validation and reload.
```

It then validates the received configuration in its own config stage. There is
an parameter override in place which disables the automatic inclusion of the production
config in `/var/lib/icinga2/api/zones`.

Once completed, the reload is triggered. This follows the same configurable timeout
as with the global reload.

```
[2019-06-19 16:52:26 +0200] information/ApiListener: Config validation for stage '/var/lib/icinga2/api/zones-stage/' was OK, replacing into '/var/lib/icinga2/api/zones/' and triggering reload.
[2019-06-19 16:52:27 +0200] information/Application: Got reload command: Started new instance with PID '19945' (timeout is 300s).
[2019-06-19 16:52:28 +0200] information/Application: Reload requested, letting new process take over.
```

Whenever the staged configuration validation fails, Icinga logs this including a reference
to the startup log file which includes additional errors.

```
[2019-06-19 15:45:27 +0200] critical/ApiListener: Config validation failed for staged cluster config sync in '/var/lib/icinga2/api/zones-stage/'. Aborting. Logs: '/var/lib/icinga2/api/zones-stage//startup.log'
```


#### Config Sync: Changes and Reload <a id="technical-concepts-cluster-config-sync-changes-reload"></a>

Whenever a new configuration is received, it is validated and upon success, the
daemon automatically reloads. While the daemon continues with checks, the reload
cannot hand over open TCP connections. That being said, reloading the daemon everytime
a configuration is synchronized would lead into many not connected endpoints.

Therefore the cluster config sync checks whether the configuration files actually
changed, and will only trigger a reload when such a change happened.

2.11 calculates a checksum from each file content and compares this to the
production configuration. Previous versions used additional metadata with timestamps from
files which sometimes led to problems with asynchronous dates.

> **Note**
>
> For compatibility reasons, the timestamp metadata algorithm is still intact, e.g.
> when the client is 2.11 already, but the parent endpoint is still on 2.10.

Icinga logs a warning when this happens.

```
Received configuration update without checksums from parent endpoint satellite1. This behaviour is deprecated. Please upgrade the parent endpoint to 2.11+
```


The debug log provides more details on the actual checksums and checks. Future output
may change, use this solely for troubleshooting and debugging whenever the cluster
config sync fails.

```
[2019-06-19 16:13:16 +0200] information/ApiListener: Received configuration for zone 'agent' from endpoint 'master1'. Comparing the checksums.
[2019-06-19 16:13:16 +0200] debug/ApiListener: Checking for config change between stage and production. Old (3): '{"/.checksums":"7ede1276a9a32019c1412a52779804a976e163943e268ec4066e6b6ec4d15d73","/.timestamp":"ec4354b0eca455f7c2ca386fddf5b9ea810d826d402b3b6ac56ba63b55c2892c","/_etc/host.conf":"35d4823684d83a5ab0ca853c9a3aa8e592adfca66210762cdf2e54339ccf0a44"}' vs. new (3): '{"/.checksums":"84a586435d732327e2152e7c9b6d85a340cc917b89ae30972042f3dc344ea7cf","/.timestamp":"0fd6facf35e49ab1b2a161872fa7ad794564eba08624373d99d31c32a7a4c7d3","/_etc/host.conf":"0d62075e89be14088de1979644b40f33a8f185fcb4bb6ff1f7da2f63c7723fcb"}'.
[2019-06-19 16:13:16 +0200] debug/ApiListener: Checking /_etc/host.conf for checksum: 35d4823684d83a5ab0ca853c9a3aa8e592adfca66210762cdf2e54339ccf0a44
[2019-06-19 16:13:16 +0200] debug/ApiListener: Path '/_etc/host.conf' doesn't match old checksum '0d62075e89be14088de1979644b40f33a8f185fcb4bb6ff1f7da2f63c7723fcb' with new checksum '35d4823684d83a5ab0ca853c9a3aa8e592adfca66210762cdf2e54339ccf0a44'.
```


#### Config Sync: Trust <a id="technical-concepts-cluster-config-sync-trust"></a>

The config sync follows the "top down" approach, where the master endpoint in the master
zone is allowed to synchronize configuration to the child zone, e.g. the satellite zone.

Endpoints in the same zone, e.g. a secondary master, receive configuration for the same
zone and all child zones.

Endpoints in the satellite zone trust the parent zone, and will accept the pushed
configuration via JSON-RPC cluster messages. By default, this is disabled and must
be enabled with the `accept_config` attribute in the ApiListener feature (manually or with CLI
helpers).

The satellite zone will not only accept zone configuration for its own zone, but also
all configured child zones. That is why it is important to configure the zone hierarchy
on the satellite as well.

Child zones are not allowed to sync configuration up to the parent zone. Each Icinga instance
evaluates this in startup and knows on endpoint connect which config zones need to be synced.


Global zones have a special trust relationship: They are synced to all child zones, be it
a satellite zone or agent zone. Since checkable objects such as a Host or a Service object
must have only one endpoint as authority, they cannot be put into a global zone (denied by
the config compiler).

Apply rules and templates are allowed, since they are evaluated in the endpoint which received
the synced configuration. Keep in mind that there may be differences on the master and the satellite
when e.g. hostgroup membership is used for assign where expressions, but the groups are only
available on the master.


### Cluster: Message Routing <a id="technical-concepts-cluster-message-routing"></a>

One fundamental part of the cluster message routing is the MessageOrigin object.
This is created when a new JSON-RPC message is received in `JsonRpcConnection::MessageHandler()`.

It contains

- FromZone being extracted from the endpoint object which owns the JsonRpcConnection
- FromClient being the JsonRpcConnection bound to the endpoint object

These attributes are checked in message receive api handlers for security access. E.g. whether a
message origin is from a child zone which is not allowed, etc.
This is explained in the [JSON-RPC messages](19-technical-concepts.md#technical-concepts-json-rpc-messages) chapter.

Whenever such a message is processed on the client, it may trigger additional cluster events
which are sent back to other endpoints. Therefore it is key to always pass the MessageOrigin
`origin` when processing these messages locally.

Example:

- Client receives a CheckResult from another endpoint in the same zone, call it `sender` for now
- Calls ProcessCheckResult() to store the CR and calculcate states, notifications, etc.
- Calls the OnNewCheckResult() signal to trigger IDO updates

OnNewCheckResult() also calls a registered cluster handler which forwards the CheckResult to other cluster members.

Without any origin details, this CheckResult would be relayed to the `sender` endpoint again.
Which processes the message, ProcessCheckResult(), OnNewCheckResult(), sends back and so on.

That creates a loop which our cluster protocol needs to prevent at all cost.

RelayMessageOne() takes care of the routing. This involves fetching the targetZone for this message and its endpoints.

- Don't relay messages to ourselves.
- Don't relay messages to disconnected endpoints.
- Don't relay the message to the zone through more than one endpoint unless this is our own zone.
- Don't relay messages back to the endpoint which we got the message from. **THIS**
- Don't relay messages back to the zone which we got the message from.
- Only relay message to the zone master if we're not currently the zone master.

```
 e1 is zone master, e2 and e3 are zone members.

 Message is sent from e2 or e3:
   !isMaster == true
   targetEndpoint e1 is zone master -> send the message
   targetEndpoint e3 is not zone master -> skip it, avoid routing loops

 Message is sent from e1:
   !isMaster == false -> send the messages to e2 and e3 being the zone routing master.
```

With passing the `origin` the following condition prevents sending a message back to sender:

```cpp
if (origin && origin->FromClient && targetEndpoint == origin->FromClient->GetEndpoint()) {
```

This message then simply gets skipped for this specific Endpoint and is never sent.

This analysis originates from a long-lasting [downtime loop bug](https://github.com/Icinga/icinga2/issues/7198).

## TLS Network IO <a id="technical-concepts-tls-network-io"></a>

### TLS Connection Handling <a id="technical-concepts-tls-network-io-connection-handling"></a>

Icinga supports two connection directions, controlled via the `host` attribute
inside the Endpoint objects:

* Outgoing connection attempts
* Incoming connection handling

Once the connection is established, higher layers can exchange JSON-RPC and
HTTP messages. It doesn't matter which direction these message go.

This offers a big advantage over single direction connections, just like
polling via HTTP only. Also, connections are kept alive as long as data
is transmitted.

When the master connects to the child zone member(s), this requires more
resources there. Keep this in mind when endpoints are not reachable, the
TCP timeout blocks other resources. Moving a satellite zone in the middle
between masters and agents helps to split the tasks - the master
processes and stores data, deploys configuration and serves the API. The
satellites schedule the checks, connect to the agents and receive
check results.

Agents/Clients can also connect to the parent endpoints - be it a master or
a satellite. This is the preferred way out of a DMZ, and also reduces the
overhead with connecting to e.g. 2000 agents on the master. You can
benchmark this when TCP connections are broken and timeouts are encountered.

#### Master Processes Incoming Connection <a id="technical-concepts-tls-network-io-connection-handling-incoming"></a>

* The node starts a new ApiListener, this invokes `AddListener()`
    * Setup TLS Context (SslContext)
    * Initialize global I/O engine and create a TCP acceptor
    * Resolve bind host/port (optional)
    * Listen on IPv4 and IPv6
    * Re-use socket address and port
    * Listen on port 5665 with `INT_MAX` possible sockets
* Spawn a new Coroutine which listens for new incoming connections as 'TCP server' pattern
    * Accept new connections asynchronously
    * Spawn a new Coroutine which handles the new client connection in a different context, Role: Server

#### Master Connects Outgoing <a id="technical-concepts-tls-network-io-connection-handling-outgoing"></a>

* The node starts a timer in a 10 seconds interval with `ApiReconnectTimerHandler()` as callback
    * Loop over all configured zones, exclude global zones and not direct parent/child zones
    * Get the endpoints configured in the zones, exclude: local endpoint, no 'host' attribute, already connected or in progress
    * Call `AddConnection()`
* Spawn a new Coroutine after making the TLS context
    * Use the global I/O engine for socket I/O
    * Create TLS stream
    * Connect to endpoint host/port details
    * Handle the client connection, Role: Client

#### TLS Handshake <a id="technical-concepts-tls-network-io-connection-handling-handshake"></a>

* Create a TLS connection in sslConn and perform an asynchronous TLS handshake
* Get the peer certificate
* Verify the presented certificate: `ssl::verify_peer` and `ssl::verify_client_once`
* Get the certificate CN and compare it against the endpoint name - if not matching, return and close the connection

#### Data Exchange <a id="technical-concepts-tls-network-io-connection-data-exchange"></a>

Everything runs through TLS, we don't use any "raw" connections nor plain message handling.

HTTP and JSON-RPC messages share the same port and API, so additional handling is required.

On a new connection and successful TLS handshake, the first byte is read. This either
is a JSON-RPC message in Netstring format starting with a number, or plain HTTP.

```
HTTP/1.1

2:{}
```

Depending on this, `ClientJsonRpc` or `ClientHttp` are assigned.

JSON-RPC:

* Create a new JsonRpcConnection object
    * When the endpoint object is configured, spawn a Coroutine which takes care of syncing the client (file and runtime config, replay log, etc.)
    * No endpoint treats this connection as anonymous client, with a configurable limit. This client may send a CSR signing request for example.
    * Start the JsonRpcConnection - this spawns Coroutines to HandleIncomingMessages, WriteOutgoingMessages, HandleAndWriteHeartbeats and CheckLiveness

HTTP:

* Create a new HttpServerConnection
     * Start the HttpServerConnection - this spawns Coroutines to ProcessMessages and CheckLiveness


All the mentioned Coroutines run asynchronously using the global I/O engine's context.
More details on this topic can be found in [this blogpost](https://www.netways.de/blog/2019/04/04/modern-c-programming-coroutines-with-boost/).

The lower levels of context switching and sharing or event polling are
hidden in Boost ASIO, Beast, Coroutine and Context libraries.

#### Data Exchange: Coroutines and I/O Engine <a id="technical-concepts-tls-network-io-connection-data-exchange-coroutines"></a>

Light-weight and fast operations such as connection handling or TLS handshakes
are performed in the default `IoBoundWorkSlot` pool inside the I/O engine.

The I/O engine has another pool available: `CpuBoundWork`.

This is used for processing CPU intensive tasks, such as handling a HTTP request.
Depending on the available CPU cores, this is limited to `std::thread::hardware_concurrency() * 3u / 2u`.

```
1 core * 3 / 2 = 1
2 cores * 3 / 2 = 3
8 cores * 3 / 2 = 12
16 cores * 3 / 2 = 24
```

The I/O engine itself is used with all network I/O in Icinga, not only the cluster
and the REST API. Features such as Graphite, InfluxDB, etc. also consume its functionality.

There are 2 * CPU cores threads available which run the event loop
in the I/O engine. This polls the I/O service with `m_IoService.run();`
and triggers an asynchronous event progress for waiting coroutines.

<!--
## REST API <a id="technical-concepts-rest-api"></a>

Icinga 2 provides its own HTTP server which shares the port 5665 with
the JSON-RPC cluster protocol.
-->

## JSON-RPC Message API <a id="technical-concepts-json-rpc-messages"></a>

**The JSON-RPC message API is not a public API for end users.** In case you want
to interact with Icinga, use the [REST API](12-icinga2-api.md#icinga2-api).

This section describes the internal cluster messages exchanged between endpoints.

> **Tip**
>
> Debug builds with `icinga2 daemon -DInternal.DebugJsonRpc=1` unveils the JSON-RPC messages.

### Registered Handler Functions

Functions by example:

Event Sender: `Checkable::OnNewCheckResult`

```
On<xyz>.connect(&xyzHandler)
```

Event Receiver (Client): `CheckResultAPIHandler` in `REGISTER_APIFUNCTION`

```
<xyz>APIHandler()
```

### Messages

#### icinga::Hello <a id="technical-concepts-json-rpc-messages-icinga-hello"></a>

> Location: `apilistener.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | icinga::Hello
params    | Dictionary

##### Params

Key                  | Type        | Description
---------------------|-------------|------------------
capabilities         | Number      | Bitmask, see `lib/remote/apilistener.hpp`.
version              | Number      | Icinga 2 version, e.g. 21300 for v2.13.0.

##### Functions

Event Sender: When a new client connects in `NewClientHandlerInternal()`.
Event Receiver: `HelloAPIHandler`

##### Permissions

None, this is a required message.

#### event::Heartbeat <a id="technical-concepts-json-rpc-messages-event-heartbeat"></a>

> Location: `jsonrpcconnection-heartbeat.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::Heartbeat
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
timeout   | Number        | Heartbeat timeout, sender sets 120s.


##### Functions

Event Sender: `JsonRpcConnection::HeartbeatTimerHandler`
Event Receiver: `HeartbeatAPIHandler`

Both sender and receiver exchange this heartbeat message. If the sender detects
that a client endpoint hasn't sent anything in the updated timeout span, it disconnects
the client. This is to avoid stale connections with no message processing.

##### Permissions

None, this is a required message.

#### event::CheckResult <a id="technical-concepts-json-rpc-messages-event-checkresult"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::CheckResult
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
host      | String        | Host name
service   | String        | Service name
cr        | Serialized CR | Check result

##### Functions

Event Sender: `Checkable::OnNewCheckResult`
Event Receiver: `CheckResultAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Hosts/services do not exist
* Origin is a remote command endpoint different to the configured, and whose zone is not allowed to access this checkable.

#### event::SetNextCheck <a id="technical-concepts-json-rpc-messages-event-setnextcheck"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetNextCheck
params    | Dictionary

##### Params

Key         | Type          | Description
------------|---------------|------------------
host        | String        | Host name
service     | String        | Service name
next\_check | Timestamp     | Next scheduled time as UNIX timestamp.

##### Functions

Event Sender: `Checkable::OnNextCheckChanged`
Event Receiver: `NextCheckChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SetLastCheckStarted <a id="technical-concepts-json-rpc-messages-event-setlastcheckstarted"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetLastCheckStarted
params    | Dictionary

##### Params

Key                  | Type      | Description
---------------------|-----------|------------------
host                 | String    | Host name
service              | String    | Service name
last\_check\_started | Timestamp | Last check's start time as UNIX timestamp.

##### Functions

Event Sender: `Checkable::OnLastCheckStartedChanged`
Event Receiver: `LastCheckStartedChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SetStateBeforeSuppression <a id="technical-concepts-json-rpc-messages-event-setstatebeforesuppression"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------------------------------
jsonrpc   | 2.0
method    | event::SetStateBeforeSuppression
params    | Dictionary

##### Params

Key                        | Type   | Description
---------------------------|--------|-----------------------------------------------
host                       | String | Host name
service                    | String | Service name
state\_before\_suppression | Number | Checkable state before the current suppression

##### Functions

Event Sender: `Checkable::OnStateBeforeSuppressionChanged`
Event Receiver: `StateBeforeSuppressionChangedAPIHandler`

Used to sync the checkable state from before a notification suppression (for example
because the checkable is in a downtime) started within the same HA zone.

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint is not within the local zone.

#### event::SetSuppressedNotifications <a id="technical-concepts-json-rpc-messages-event-setsupressednotifications"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetSuppressedNotifications
params    | Dictionary

##### Params

Key         		 | Type          | Description
-------------------------|---------------|------------------
host        		 | String        | Host name
service     		 | String        | Service name
supressed\_notifications | Number 	 | Bitmask for suppressed notifications.

##### Functions

Event Sender: `Checkable::OnSuppressedNotificationsChanged`
Event Receiver: `SuppressedNotificationsChangedAPIHandler`

Used to sync the notification state of a host or service object within the same HA zone.

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint is not within the local zone.

#### event::SetSuppressedNotificationTypes <a id="technical-concepts-json-rpc-messages-event-setsuppressednotificationtypes"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetSuppressedNotificationTypes
params    | Dictionary

##### Params

Key         		 | Type   | Description
-------------------------|--------|------------------
notification             | String | Notification name
supressed\_notifications | Number | Bitmask for suppressed notifications.

Used to sync the state of a notification object within the same HA zone.

##### Functions

Event Sender: `Notification::OnSuppressedNotificationsChanged`
Event Receiver: `SuppressedNotificationTypesChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Notification does not exist.
* Origin endpoint is not within the local zone.


#### event::SetNextNotification <a id="technical-concepts-json-rpc-messages-event-setnextnotification"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetNextNotification
params    | Dictionary

##### Params

Key                | Type          | Description
-------------------|---------------|------------------
host               | String        | Host name
service            | String        | Service name
notification       | String        | Notification name
next\_notification | Timestamp     | Next scheduled notification time as UNIX timestamp.

##### Functions

Event Sender: `Notification::OnNextNotificationChanged`
Event Receiver: `NextNotificationChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Notification does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::UpdateLastNotifiedStatePerUser <a id="technical-concepts-json-rpc-messages-event-updatelastnotifiedstateperuser"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::UpdateLastNotifiedStatePerUser
params    | Dictionary

##### Params

Key          | Type   | Description
-------------|--------|------------------
notification | String | Notification name
user         | String | User name
state        | Number | Checkable state the user just got a problem notification for

Used to sync the state of a notification object within the same HA zone.

##### Functions

Event Sender: `Notification::OnLastNotifiedStatePerUserUpdated`
Event Receiver: `LastNotifiedStatePerUserUpdatedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Notification does not exist.
* Origin endpoint is not within the local zone.

#### event::ClearLastNotifiedStatePerUser <a id="technical-concepts-json-rpc-messages-event-clearlastnotifiedstateperuser"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::ClearLastNotifiedStatePerUser
params    | Dictionary

##### Params

Key          | Type   | Description
-------------|--------|------------------
notification | String | Notification name

Used to sync the state of a notification object within the same HA zone.

##### Functions

Event Sender: `Notification::OnLastNotifiedStatePerUserCleared`
Event Receiver: `LastNotifiedStatePerUserClearedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Notification does not exist.
* Origin endpoint is not within the local zone.

#### event::SetForceNextCheck <a id="technical-concepts-json-rpc-messages-event-setforcenextcheck"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetForceNextCheck
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
host      | String        | Host name
service   | String        | Service name
forced    | Boolean       | Forced next check (execute now)

##### Functions

Event Sender: `Checkable::OnForceNextCheckChanged`
Event Receiver: `ForceNextCheckChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SetForceNextNotification <a id="technical-concepts-json-rpc-messages-event-setforcenextnotification"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetForceNextNotification
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
host      | String        | Host name
service   | String        | Service name
forced    | Boolean       | Forced next check (execute now)

##### Functions

Event Sender: `Checkable::SetForceNextNotification`
Event Receiver: `ForceNextNotificationChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SetAcknowledgement <a id="technical-concepts-json-rpc-messages-event-setacknowledgement"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetAcknowledgement
params    | Dictionary

##### Params

Key        | Type          | Description
-----------|---------------|------------------
host       | String        | Host name
service    | String        | Service name
author     | String        | Acknowledgement author name.
comment    | String        | Acknowledgement comment content.
acktype    | Number        | Acknowledgement type (0=None, 1=Normal, 2=Sticky)
notify     | Boolean       | Notification should be sent.
persistent | Boolean       | Whether the comment is persistent.
expiry     | Timestamp     | Optional expire time as UNIX timestamp.

##### Functions

Event Sender: `Checkable::OnForceNextCheckChanged`
Event Receiver: `ForceNextCheckChangedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::ClearAcknowledgement <a id="technical-concepts-json-rpc-messages-event-clearacknowledgement"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::ClearAcknowledgement
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
host      | String        | Host name
service   | String        | Service name

##### Functions

Event Sender: `Checkable::OnAcknowledgementCleared`
Event Receiver: `AcknowledgementClearedAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SendNotifications <a id="technical-concepts-json-rpc-messages-event-sendnotifications"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SendNotifications
params    | Dictionary

##### Params

Key       | Type          | Description
----------|---------------|------------------
host      | String        | Host name
service   | String        | Service name
cr        | Serialized CR | Check result
type      | Number        | enum NotificationType, same as `types` for notification objects.
author    | String        | Author name
text      | String        | Notification text

##### Functions

Event Sender: `Checkable::OnNotificationsRequested`
Event Receiver: `SendNotificationsAPIHandler`

Signals that notifications have to be sent within the same HA zone. This is relevant if the checkable and its
notifications are active on different endpoints.

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint is not within the local zone.

#### event::NotificationSentUser <a id="technical-concepts-json-rpc-messages-event-notificationsentuser"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::NotificationSentUser
params    | Dictionary

##### Params

Key           | Type            | Description
--------------|-----------------|------------------
host          | String          | Host name
service       | String          | Service name
notification  | String          | Notification name.
user          | String          | Notified user name.
type          | Number          | enum NotificationType, same as `types` in Notification objects.
cr            | Serialized CR   | Check result.
author        | String          | Notification author (for specific types)
text          | String          | Notification text (for specific types)
command       | String          | Notification command name.

##### Functions

Event Sender: `Checkable::OnNotificationSentToUser`
Event Receiver: `NotificationSentUserAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone the same as the receiver. This binds notification messages to the HA zone.

#### event::NotificationSentToAllUsers <a id="technical-concepts-json-rpc-messages-event-notificationsenttoallusers"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::NotificationSentToAllUsers
params    | Dictionary

##### Params

Key                         | Type            | Description
----------------------------|-----------------|------------------
host                        | String          | Host name
service                     | String          | Service name
notification                | String          | Notification name.
users                       | Array of String | Notified user names.
type                        | Number          | enum NotificationType, same as `types` in Notification objects.
cr                          | Serialized CR   | Check result.
author                      | String          | Notification author (for specific types)
text                        | String          | Notification text (for specific types)
last\_notification          | Timestamp       | Last notification time as UNIX timestamp.
next\_notification          | Timestamp       | Next scheduled notification time as UNIX timestamp.
notification\_number        | Number          | Current notification number in problem state.
last\_problem\_notification | Timestamp       | Last problem notification time as UNIX timestamp.
no\_more\_notifications     | Boolean         | Whether to send future notifications when this notification becomes active on this HA node.

##### Functions

Event Sender: `Checkable::OnNotificationSentToAllUsers`
Event Receiver: `NotificationSentToAllUsersAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone the same as the receiver. This binds notification messages to the HA zone.

#### event::ExecuteCommand <a id="technical-concepts-json-rpc-messages-event-executecommand"></a>

> Location: `clusterevents-check.cpp` and `checkable-check.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::ExecuteCommand
params    | Dictionary

##### Params

Key            | Type          | Description
---------------|---------------|------------------
host           | String        | Host name.
service        | String        | Service name.
command\_type  | String        | `check_command` or `event_command`.
command        | String        | CheckCommand or EventCommand name.
check\_timeout | Number        | Check timeout of the checkable object, if specified as `check_timeout` attribute.
macros         | Dictionary    | Command arguments as key/value pairs for remote execution.
endpoint       | String        | The endpoint to execute the command on.
deadline       | Number        | A Unix timestamp indicating the execution deadline
source         | String        | The execution UUID


##### Functions

**Event Sender:** This gets constructed directly in `Checkable::ExecuteCheck()`, `Checkable::ExecuteEventHandler()` or `ApiActions::ExecuteCommand()` when a remote command endpoint is configured.

* `Get{CheckCommand,EventCommand}()->Execute()` simulates an execution and extracts all command arguments into the `macro` dictionary (inside lib/methods tasks).
* When the endpoint is connected, the message is constructed and sent directly.
* When the endpoint is not connected and not syncing replay logs and 5m after application start, generate an UNKNOWN check result for the user ("not connected").

**Event Receiver:** `ExecuteCommandAPIHandler`

Special handling, calls `ClusterEvents::EnqueueCheck()` for command endpoint checks.
This function enqueues check tasks into a queue which is controlled in `RemoteCheckThreadProc()`.
If the `endpoint` parameter is specified and is not equal to the local endpoint then the message is forwarded to the correct endpoint zone.

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Origin endpoint's zone is not a parent zone of the receiver endpoint.
* `accept_commands = false` in the `api` feature configuration sends back an UNKNOWN check result to the sender.

The receiver constructs a virtual host object and looks for the local CheckCommand object.

Returns UNKNOWN as check result to the sender

* when the CheckCommand object does not exist.
* when there was an exception triggered from check execution, e.g. the plugin binary could not be executed or similar.

The returned messages are synced directly to the sender's endpoint, no cluster broadcast.

> **Note**: EventCommand errors are just logged on the remote endpoint.

#### event::UpdateExecutions <a id="technical-concepts-json-rpc-messages-event-updateexecutions"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::UpdateExecutions
params    | Dictionary

##### Params

Key            | Type          | Description
---------------|---------------|------------------
host           | String        | Host name.
service        | String        | Service name.
executions     | Dictionary    | Executions to be updated

##### Functions

**Event Sender:** `ClusterEvents::ExecutedCommandAPIHandler`, `ClusterEvents::UpdateExecutionsAPIHandler`, `ApiActions::ExecuteCommand`
**Event Receiver:** `ClusterEvents::UpdateExecutionsAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::ExecutedCommand <a id="technical-concepts-json-rpc-messages-event-executedcommand"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::ExecutedCommand
params    | Dictionary

##### Params

Key            | Type          | Description
---------------|---------------|------------------
host           | String        | Host name.
service        | String        | Service name.
execution      | String        | The execution ID executed.
exitStatus     | Number        | The command exit status.
output         | String        | The command output.
start          | Number        | The unix timestamp at the start of the command execution
end            | Number        | The unix timestamp at the end of the command execution

##### Functions

**Event Sender:** `ClusterEvents::ExecuteCheckFromQueue`, `ClusterEvents::ExecuteCommandAPIHandler`
**Event Receiver:** `ClusterEvents::ExecutedCommandAPIHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Checkable does not exist.
* Origin endpoint's zone is not allowed to access this checkable.

#### event::SetRemovalInfo <a id="technical-concepts-json-rpc-messages-event-setremovalinfo"></a>

> Location: `clusterevents.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | event::SetRemovalInfo
params    | Dictionary

##### Params

Key            | Type        | Description
---------------|-------------|---------------------------------
object\_type   | String      | Object type (`"Comment"` or `"Downtime"`)
object\_name   | String      | Object name
removed\_by    | String      | Name of the removal requestor
remove\_time   | Timestamp   | Time of the remove operation

##### Functions

**Event Sender**: `Comment::OnRemovalInfoChanged` and `Downtime::OnRemovalInfoChanged`
**Event Receiver**: `SetRemovalInfoAPIHandler`

This message is used to synchronize information about manual comment and downtime removals before deleting the
corresponding object.

##### Permissions

This message is only accepted from the local zone and from parent zones.

#### config::Update <a id="technical-concepts-json-rpc-messages-config-update"></a>

> Location: `apilistener-filesync.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | config::Update
params    | Dictionary

##### Params

Key        | Type          | Description
-----------|---------------|------------------
update     | Dictionary    | Config file paths and their content.
update\_v2 | Dictionary    | Additional meta config files introduced in 2.4+ for compatibility reasons.

##### Functions

**Event Sender:** `SendConfigUpdate()` called in `ApiListener::SyncClient()` when a new client endpoint connects.
**Event Receiver:** `ConfigUpdateHandler` reads the config update content and stores them in `/var/lib/icinga2/api`.
When it detects a configuration change, the function requests and application restart.

##### Permissions

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* The origin sender is not in a parent zone of the receiver.
* `api` feature does not accept config.

Config updates will be ignored when:

* The zone is not configured on the receiver endpoint.
* The zone is authoritative on this instance (this only happens on a master which has `/etc/icinga2/zones.d` populated, and prevents sync loops)

#### config::UpdateObject <a id="technical-concepts-json-rpc-messages-config-updateobject"></a>

> Location: `apilistener-configsync.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | config::UpdateObject
params    | Dictionary

##### Params

Key                  | Type        | Description
---------------------|-------------|------------------
name                 | String      | Object name.
type                 | String      | Object type name.
version              | Number      | Object version.
config               | String      | Config file content for `_api` packages.
modified\_attributes | Dictionary  | Modified attributes at runtime as key value pairs.
original\_attributes | Array       | Original attributes as array of keys.


##### Functions

**Event Sender:** Either on client connect (full sync), or runtime created/updated object

`ApiListener::SendRuntimeConfigObjects()` gets called when a new endpoint is connected
and runtime created config objects need to be synced. This invokes a call to `UpdateConfigObject()`
to only sync this JsonRpcConnection client.

`ConfigObject::OnActiveChanged` (created or deleted) or `ConfigObject::OnVersionChanged` (updated)
also call `UpdateConfigObject()`.

**Event Receiver:** `ConfigUpdateObjectAPIHandler` calls `ConfigObjectUtility::CreateObject()` in order
to create the object if it is not already existing. Afterwards, all modified attributes are applied
and in case, original attributes are restored. The object version is set as well, keeping it in sync
with the sender.

##### Permissions

###### Sender

Client receiver connects:

The sender only syncs config object updates to a client which can access
the config object, in `ApiListener::SendRuntimeConfigObjects()`.

In addition to that, the client endpoint's zone is checked whether this zone may access
the config object.

Runtime updated object:

Only if the config object belongs to the `_api` package.


###### Receiver

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Origin sender endpoint's zone is in a child zone.
* `api` feature does not accept config
* The received config object type does not exist (this is to prevent failures with older nodes and new object types).

Error handling:

* Log an error if `CreateObject` fails (only if the object does not already exist)
* Local object version is newer than the received version, object will not be updated.
* Compare modified and original attributes and restore any type of change here.


#### config::DeleteObject <a id="technical-concepts-json-rpc-messages-config-deleteobject"></a>

> Location: `apilistener-configsync.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | config::DeleteObject
params    | Dictionary

##### Params

Key                 | Type        | Description
--------------------|-------------|------------------
name                | String      | Object name.
type                | String      | Object type name.
version             | Number      | Object version.

##### Functions

**Event Sender:**

`ConfigObject::OnActiveChanged` (created or deleted) or `ConfigObject::OnVersionChanged` (updated)
call `DeleteConfigObject()`.

**Event Receiver:** `ConfigDeleteObjectAPIHandler`

##### Permissions

###### Sender

Runtime deleted object:

Only if the config object belongs to the `_api` package.

###### Receiver

The receiver will not process messages from not configured endpoints.

Message updates will be dropped when:

* Origin sender endpoint's zone is in a child zone.
* `api` feature does not accept config
* The received config object type does not exist (this is to prevent failures with older nodes and new object types).
* The object in question was not created at runtime, it does not belong to the `_api` package.

Error handling:

* Log an error if `DeleteObject` fails (only if the object does not already exist)

#### pki::RequestCertificate <a id="technical-concepts-json-rpc-messages-pki-requestcertificate"></a>

> Location: `jsonrpcconnection-pki.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | pki::RequestCertificate
params    | Dictionary

##### Params

Key           | Type          | Description
--------------|---------------|------------------
ticket        | String        | Own ticket, or as satellite in CA proxy from local store.
cert\_request | String        | Certificate request content from local store, optional.

##### Functions

Event Sender: `RequestCertificateHandler`
Event Receiver: `RequestCertificateHandler`

##### Permissions

This is an anonymous request, and the number of anonymous clients can be configured
in the `api` feature.

Only valid certificate request messages are processed, and valid signed certificates
won't be signed again.

#### pki::UpdateCertificate <a id="technical-concepts-json-rpc-messages-pki-updatecertificate"></a>

> Location: `jsonrpcconnection-pki.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | pki::UpdateCertificate
params    | Dictionary

##### Params

Key                  | Type          | Description
---------------------|---------------|------------------
status\_code         | Number        | Status code, 0=ok.
cert                 | String        | Signed certificate content.
ca                   | String        | Public CA certificate content.
fingerprint\_request | String        | Certificate fingerprint from the CSR.


##### Functions

**Event Sender:**

* When a client requests a certificate in `RequestCertificateHandler` and the satellite
already has a signed certificate, the `pki::UpdateCertificate` message is constructed and sent back.
* When the endpoint holding the master's CA private key (and TicketSalt private key) is able to sign
the request, the `pki::UpdateCertificate` message is constructed and sent back.

**Event Receiver:** `UpdateCertificateHandler`

##### Permissions

Message updates are dropped when

* The origin sender is not in a parent zone of the receiver.
* The certificate fingerprint is in an invalid format.

#### log::SetLogPosition <a id="technical-concepts-json-rpc-messages-log-setlogposition"></a>

> Location: `apilistener.cpp` and `jsonrpcconnection.cpp`

##### Message Body

Key       | Value
----------|---------
jsonrpc   | 2.0
method    | log::SetLogPosition
params    | Dictionary

##### Params

Key                 | Type          | Description
--------------------|---------------|------------------
log\_position       | Timestamp     | The endpoint's log position as UNIX timestamp.


##### Functions

**Event Sender:**

During log replay to a client endpoint in `ApiListener::ReplayLog()`, each processed
file generates a message which updates the log position timestamp.

`ApiListener::ApiTimerHandler()` invokes a check to keep all connected endpoints and
their log position in sync during replay log.

**Event Receiver:** `SetLogPositionHandler`

##### Permissions

The receiver will not process messages from not configured endpoints.
