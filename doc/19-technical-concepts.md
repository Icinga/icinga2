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

