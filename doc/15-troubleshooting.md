# <a id="troubleshooting"></a> Icinga 2 Troubleshooting

## <a id="troubleshooting-information-required"></a> Which information is required

* Run `icinga2 troubleshoot` to collect required troubleshooting information
* Alternative, manual steps:
	* `icinga2 --version`
	* `icinga2 feature list`
	* `icinga2 daemon --validate`
	* Relevant output from your main and debug log ( `icinga2 object list --type='filelogger'` )
	* The newest Icinga 2 crash log if relevant
	* Your icinga2.conf and, if you run multiple Icinga 2 instances, your zones.conf
* How was Icinga 2 installed (and which repository in case) and which distribution are you using
* Provide complete configuration snippets explaining your problem in detail
* If the check command failed, what's the output of your manual plugin tests?
* In case of [debugging](20-development.md#development) Icinga 2, the full back traces and outputs

## <a id="troubleshooting-enable-debug-output"></a> Enable Debug Output

Enable the `debuglog` feature:

    # icinga2 feature enable debuglog
    # service icinga2 restart

You can find the debug log file in `/var/log/icinga2/debug.log`.

Alternatively you may run Icinga 2 in the foreground with debugging enabled. Specify the console
log severity as an additional parameter argument to `-x`.

    # /usr/sbin/icinga2 daemon -x notice

The log level can be one of `critical`, `warning`, `information`, `notice`
and `debug`.

## <a id="list-configuration-objects"></a> List Configuration Objects

The `icinga2 object list` CLI command can be used to list all configuration objects and their
attributes. The tool also shows where each of the attributes was modified.

> **Tip**
>
> Use the Icinga 2 API to access [config objects at runtime](12-icinga2-api.md#icinga2-api-config-objects) directly.

That way you can also identify which objects have been created from your [apply rules](17-language-reference.md#apply).

    # icinga2 object list

    Object 'localhost!ssh' of type 'Service':
      * __name = 'localhost!ssh'
      * check_command = 'ssh'
        % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 5:3-5:23
      * check_interval = 60
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 24:3-24:21
      * host_name = 'localhost'
        % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 4:3-4:25
      * max_check_attempts = 3
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 23:3-23:24
      * name = 'ssh'
      * retry_interval = 30
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 25:3-25:22
      * templates = [ 'ssh', 'generic-service' ]
        % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 1:0-7:1
        % += modified in '/etc/icinga2/conf.d/templates.conf', lines 22:1-26:1
      * type = 'Service'
      * vars
        % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19
        * sla = '24x7'
          % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19

    [...]

You can also filter by name and type:

    # icinga2 object list --name *ssh* --type Service
    Object 'localhost!ssh' of type 'Service':
      * __name = 'localhost!ssh'
      * check_command = 'ssh'
        % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 5:3-5:23
      * check_interval = 60
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 24:3-24:21
      * host_name = 'localhost'
        % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 4:3-4:25
      * max_check_attempts = 3
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 23:3-23:24
      * name = 'ssh'
      * retry_interval = 30
        % = modified in '/etc/icinga2/conf.d/templates.conf', lines 25:3-25:22
      * templates = [ 'ssh', 'generic-service' ]
        % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 1:0-7:1
        % += modified in '/etc/icinga2/conf.d/templates.conf', lines 22:1-26:1
      * type = 'Service'
      * vars
        % += modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19
        * sla = '24x7'
          % = modified in '/etc/icinga2/conf.d/hosts/localhost/ssh.conf', lines 6:3-6:19

    Found 1 Service objects.

    [2014-10-15 14:27:19 +0200] information/cli: Parsed 175 objects.

## <a id="check-command-definitions"></a> Where are the check command definitions?

Icinga 2 features a number of built-in [check command definitions](10-icinga-template-library.md#plugin-check-commands) which are
included using

    include <itl>
    include <plugins>

in the [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf) configuration file. These files are not considered configuration files and will be overridden
on upgrade, so please send modifications as proposed patches upstream. The default include path is set to
`LocalStateDir + "/share/icinga2/includes"`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.

## <a id="troubleshooting-checks"></a> Checks

### <a id="checks-executed-command"></a> Executed Command for Checks

* Use the Icinga 2 API to [query](12-icinga2-api.md#icinga2-api-config-objects-query) host/service objects
for their check result containing the executed shell command.
* Use the Icinga 2 [console cli command](11-cli-commands.md#cli-command-console)
to fetch the checkable object, its check result and the executed shell command.
* Alternatively enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) and look for the executed command.

Example for a service object query using a [regex match]() on the name:

    $ curl -k -s -u root:icinga -H 'Accept: application/json' -H 'X-HTTP-Method-Override: GET' -X POST 'https://localhost:5665/v1/objects/services' \
    -d '{ "filter": "regex(pattern, service.name)", "filter_vars": { "pattern": "^http" }, "attrs": [ "__name", "last_check_result" ] }' | python -m json.tool
    {
        "results": [
            {
                "attrs": {
                    "__name": "example.localdomain!http",
                    "last_check_result": {
                        "active": true,
                        "check_source": "example.localdomain",
                        "command": [
                            "/usr/local/sbin/check_http",
                            "-I",
                            "127.0.0.1",
                            "-u",
                            "/"
                        ],

      ...

                    }
                },
                "joins": {},
                "meta": {},
                "name": "example.localdomain!http",
                "type": "Service"
            }
        ]
    }

Example for using the `icinga2 console` CLI command evaluation functionality:

    $ ICINGA2_API_PASSWORD=icinga icinga2 console --connect 'https://root@localhost:5665/' \
    --eval 'get_service("example.localdomain", "http").last_check_result.command' | python -m json.tool
    [
        "/usr/local/sbin/check_http",
        "-I",
        "127.0.0.1",
        "-u",
        "/"
    ]

Example for searching the debug log:

    # icinga2 feature enable debuglog
    # systemctl restart icinga2
    # tail -f /var/log/icinga2/debug.log | grep "notice/Process"


### <a id="checks-not-executed"></a> Checks are not executed

* Check the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) to see if the check command gets executed.
* Verify that failed depedencies do not prevent command execution.
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test).
* Make sure the [checker](11-cli-commands.md#enable-features) feature is enabled.
* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Examples:

    # sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%

    # icinga2 feature enable checker
    The feature 'checker' is already enabled.

Fetch all check result events matching the `event.service` name `random`:

    $ curl -k -s -u root:icinga -X POST 'https://localhost:5665/v1/events?queue=debugchecks&types=CheckResult&filter=match%28%22random*%22,event.service%29'


## <a id="notifications-not-sent"></a> Notifications are not sent

* Check the debug log to see if a notification is triggered.
* If yes, verify that all conditions are satisfied.
* Are any errors on the notification command execution logged?

Verify the following configuration:

* Is the host/service `enable_notifications` attribute set, and if so, to which value?
* Do the notification attributes `states`, `types`, `period` match the notification conditions?
* Do the user attributes `states`, `types`, `period` match the notification conditions?
* Are there any notification `begin` and `end` times configured?
* Make sure the [notification](11-cli-commands.md#enable-features) feature is enabled.
* Does the referenced NotificationCommand work when executed as Icinga user on the shell?

If notifications are to be sent via mail, make sure that the mail program specified inside the
[NotificationCommand object](9-object-types.md#objecttype-notificationcommand) exists.
The name and location depends on the distribution so the preconfigured setting might have to be
changed on your system.


Examples:

    # icinga2 feature enable notification
    The feature 'notification' is already enabled.

You can use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live notification streams:

    $ curl -k -s -u root:icinga -X POST 'https://localhost:5665/v1/events?queue=debugnotifications&types=Notification'


## <a id="feature-not-working"></a> Feature is not working

* Make sure that the feature configuration is enabled by symlinking from `features-available/`
to `features-enabled` and that the latter is included in [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf).
* Are the feature attributes set correctly according to the documentation?
* Any errors on the logs?

## <a id="configuration-ignored"></a> Configuration is ignored

* Make sure that the line(s) are not [commented out](17-language-reference.md#comments) (starting with `//` or `#`, or
encapsulated by `/* ... */`).
* Is the configuration file included in [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf)?

## <a id="configuration-attribute-inheritance"></a> Configuration attributes are inherited from

Icinga 2 allows you to import templates using the [import](17-language-reference.md#template-imports) keyword. If these templates
contain additional attributes, your objects will automatically inherit them. You can override
or modify these attributes in the current object.

## <a id="configuration-value-dollar-sign"></a> Configuration Value with Single Dollar Sign

In case your configuration validation fails with a missing closing dollar sign error message, you
did not properly escape the single dollar sign preventing its usage as [runtime macro](3-monitoring-basics.md#runtime-macros).

    critical/config: Error: Validation failed for Object 'ping4' (Type: 'Service') at /etc/icinga2/zones.d/global-templates/windows.conf:24: Closing $ not found in macro format string 'top-syntax=${list}'.


## <a id="troubleshooting-cluster"></a> Cluster and Clients Troubleshooting

This applies to any Icinga 2 node in a [distributed monitoring setup](6-distributed-monitoring.md#distributed-monitoring-scenarios).

You should configure the [cluster health checks](6-distributed-monitoring.md#distributed-monitoring-health-checks) if you haven't
done so already.

> **Note**
>
> Some problems just exist due to wrong file permissions or packet filters applied. Make
> sure to check these in the first place.

### <a id="troubleshooting-cluster-connection-errors"></a> Cluster Troubleshooting Connection Errors

General connection errors normally lead you to one of the following problems:

* Wrong network configuration
* Packet loss on the connection
* Firewall rules preventing traffic

Use tools like `netstat`, `tcpdump`, `nmap`, etc. to make sure that the cluster communication
happens (default port is `5665`).

    # tcpdump -n port 5665 -i any

    # netstat -tulpen | grep icinga

    # nmap yourclusternode.localdomain

### <a id="troubleshooting-cluster-ssl-errors"></a> Cluster Troubleshooting SSL Errors

If the cluster communication fails with SSL error messages, make sure to check
the following

* File permissions on the SSL certificate files
* Does the used CA match for all cluster endpoints?
  * Verify the `Issuer` being your trusted CA
  * Verify the `Subject` containing your endpoint's common name (CN)
  * Check the validity of the certificate itself

Steps on the client `icinga2-node2.localdomain`:

    # ls -la /etc/icinga2/pki

    # cd /etc/icinga2/pki/
    # openssl x509 -in icinga2-node2.localdomain.crt -text
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number: 2 (0x2)
        Signature Algorithm: sha1WithRSAEncryption
            Issuer: C=DE, ST=Bavaria, L=Nuremberg, O=NETWAYS GmbH, OU=Monitoring, CN=Icinga CA
            Validity
                Not Before: Jan  7 13:17:38 2014 GMT
                Not After : Jan  5 13:17:38 2024 GMT
            Subject: C=DE, ST=Bavaria, L=Nuremberg, O=NETWAYS GmbH, OU=Monitoring, CN=icinga2-node2.localdomain
            Subject Public Key Info:
                Public Key Algorithm: rsaEncryption
                    Public-Key: (4096 bit)
                    Modulus:
                    ...

Try to manually connect from `icinga2-node2.localdomain` to the master node `icinga2-node1.localdomain`:

    # openssl s_client -CAfile /etc/icinga2/pki/ca.crt -cert /etc/icinga2/pki/icinga2-node2.localdomain.crt -key /etc/icinga2/pki/icinga2-node2.localdomain.key -connect icinga2-node1.localdomain:5665

    CONNECTED(00000003)
    ---
    ...

If the connection attempt fails or your CA does not match, [verify the master and client certificates](15-troubleshooting.md#troubleshooting-cluster-ssl-certificate-verification).

#### <a id="troubleshooting-cluster-unauthenticated-clients"></a> Cluster Troubleshooting Unauthenticated Clients

Unauthenticated nodes are able to connect. This is required for client setups.

Master:

    [2015-07-13 18:29:25 +0200] information/ApiListener: New client connection for identity 'icinga-client' (unauthenticated)

Client as command execution bridge:

    [2015-07-13 18:29:26 +1000] notice/ApiEvents: Discarding 'execute command' message from 'icinga-master': Invalid endpoint origin (client not allowed).

If these messages do not go away, make sure to [verify the master and client certificates](15-troubleshooting.md#troubleshooting-cluster-ssl-certificate-verification).

#### <a id="troubleshooting-cluster-ssl-certificate-verification"></a> Cluster Troubleshooting SSL Certificate Verification

Make sure to verify the client's certificate and its received `ca.crt` in `/etc/icinga2/pki` and ensure that
both instances are signed by the **same CA**.

    # openssl verify -verbose -CAfile /etc/icinga2/pki/ca.crt /etc/icinga2/pki/icinga2-node1.localdomain.crt
    icinga2-node1.localdomain.crt: OK

    # openssl verify -verbose -CAfile /etc/icinga2/pki/ca.crt /etc/icinga2/pki/icinga2-node2.localdomain.crt
    icinga2-node2.localdomain.crt: OK

Fetch the `ca.crt` file from the client node and compare it to your master's `ca.crt` file:

    # scp icinga2-node2:/etc/icinga2/pki/ca.crt test-client-ca.crt
    # diff -ur /etc/icinga2/pki/ca.crt test-client-ca.crt

On SLES11 you'll need to use the `openssl1` command instead of `openssl`.

### <a id="troubleshooting-cluster-message-errors"></a> Cluster Troubleshooting Message Errors

At some point, when the network connection is broken or gone, the Icinga 2 instances
will be disconnected. If the connection can't be re-established between endpoints in the same HA zone,
they remain in a Split-Brain-mode and history may differ.

Although the Icinga 2 cluster protocol stores historical events in a [replay log](15-troubleshooting.md#troubleshooting-cluster-replay-log)
for later synchronisation, you should make sure to check why the network connection failed.

### <a id="troubleshooting-cluster-command-endpoint-errors"></a> Cluster Troubleshooting Command Endpoint Errors

Command endpoints can be used [for clients](6-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)
as well as inside an [High-Availability cluster](6-distributed-monitoring.md#distributed-monitoring-scenarios).

There is no cli command for manually executing the check, but you can verify
the following (e.g. by invoking a forced check from the web interface):

* `/var/log/icinga2/icinga2.log` contains connection and execution errors.
 * The ApiListener is not enabled to [accept commands](6-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint).
 * `CheckCommand` definition not found on the remote client.
 * Referenced check plugin not found on the remote client.
 * Runtime warnings and errors, e.g. unresolved runtime macros or configuration problems.
* Specific error messages are also populated into `UNKNOWN` check results including a detailed error message in their output.
* Verify the `check_source` object attribute. This is populated by the node executing the check.
* More verbose logs are found inside the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output).

* Use the Icinga 2 API [event streams](12-icinga2-api.md#icinga2-api-event-streams) to receive live check result streams.

Fetch all check result events matching the `event.service` name `remote-client`:

    $ curl -k -s -u root:icinga -X POST 'https://localhost:5665/v1/events?queue=debugcommandendpoint&types=CheckResult&filter=match%28%22remote-client*%22,event.service%29'



### <a id="troubleshooting-cluster-config-sync"></a> Cluster Troubleshooting Config Sync

If the cluster zones do not sync their configuration, make sure to check the following:

* Within a config master zone, only one configuration master is allowed to have its config in `/etc/icinga2/zones.d`.
** The master syncs the configuration to `/var/lib/icinga2/api/zones/` during startup and only syncs valid configuration to the other nodes.
** The other nodes receive the configuration into `/var/lib/icinga2/api/zones/`.
* The `icinga2.log` log file in `/var/log/icinga2` will indicate whether this ApiListener
[accepts config](6-distributed-monitoring.md#distributed-monitoring-top-down-config-sync), or not.

Verify the object's [version](9-object-types.md#object-types) attribute on all nodes to
check whether the config update and reload was succesful or not.

### <a id="troubleshooting-cluster-check-results"></a> Cluster Troubleshooting Overdue Check Results

If your master does not receive check results (or any other events) from the child zones
(satellite, clients, etc.), make sure to check whether the client sending in events
is allowed to do so.

The [distributed monitoring conventions](6-distributed-monitoring.md#distributed-monitoring-conventions)
apply. So, if there's a mismatch between your client node's endpoint name and its provided
certificate's CN, the master will deny all events.

> **Tip**
>
> [Icinga Web 2](2-getting-started.md#setting-up-icingaweb2) provides a dashboard view
> for overdue check results.

Enable the [debug log](15-troubleshooting.md#troubleshooting-enable-debug-output) on the master
for more verbose insights.

If the client cannot authenticate, it's a more general [problem](15-troubleshooting.md#troubleshooting-cluster-unauthenticated-clients).

The client's endpoint is not configured on nor trusted by the master node:

    Discarding 'check result' message from 'icinga2b': Invalid endpoint origin (client not allowed).

The check result message sent by the client does not belong to the zone the checkable object is
in on the master:

    Discarding 'check result' message from 'icinga2b': Unauthorized access.


### <a id="troubleshooting-cluster-replay-log"></a> Cluster Troubleshooting Replay Log

If your `/var/lib/icinga2/api/log` directory grows, it generally means that your cluster
cannot replay the log on connection loss and re-establishment. A master node for example
will store all events for not connected endpoints in the same and child zones.

Check the following:

* All clients are connected? (e.g. [cluster health check](6-distributed-monitoring.md#distributed-monitoring-health-checks)).
* Check your [connection](15-troubleshooting.md#troubleshooting-cluster-connection-errors) in general.
* Does the log replay work, e.g. are all events processed and the directory gets cleared up over time?
* Decrease the `log_duration` attribute value for that specific [endpoint](9-object-types.md#objecttype-endpoint).
