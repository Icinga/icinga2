# <a id="troubleshooting"></a> Icinga 2 Troubleshooting

## <a id="troubleshooting-information-required"></a> Which information is required

* Run `icinga2 troubleshoot` to collect required troubleshooting information
* Alternative, manual steps:
	* `icinga2 --version`
	* `icinga2 feature list`
	* `icinga2 daemon --validate`
	* Relevant output from your main and debug log ( `icinga2 object list --type='filelogger'` )
	* The newest Icinga 2 crash log, if relevant
	* Your icinga2.conf and, if you run multiple Icinga 2 instances, your zones.conf
* How was Icinga 2 installed (and which repository in case) and which distribution are you using
* Provide complete configuration snippets explaining your problem in detail
* If the check command failed - what's the output of your manual plugin tests?
* In case of [debugging](18-debug.md#debug) Icinga 2, the full back traces and outputs

## <a id="troubleshooting-enable-debug-output"></a> Enable Debug Output

Run Icinga 2 in the foreground with debugging enabled. Specify the console
log severity as an additional parameter argument to `-x`.

    # /usr/sbin/icinga2 daemon -x notice

The log level can be one of `critical`, `warning`, `information`, `notice`
and `debug`.

Alternatively you can enable the debug log:

    # icinga2 feature enable debuglog
    # service icinga2 restart

You can find the debug log file in `/var/log/icinga2/debug.log`.

## <a id="list-configuration-objects"></a> List Configuration Objects

The `icinga2 object list` CLI command can be used to list all configuration objects and their
attributes. The tool also shows where each of the attributes was modified.

That way you can also identify which objects have been created from your [apply rules](16-language-reference.md#apply).

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

Icinga 2 features a number of built-in [check command definitions](7-icinga-template-library.md#plugin-check-commands) which are
included using

    include <itl>
    include <plugins>

in the [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf) configuration file. These files are not considered configuration files and will be overridden
on upgrade, so please send modifications as proposed patches upstream. The default include path is set to
`LocalStateDir + "/share/icinga2/includes"`.

You should add your own command definitions to a new file in `conf.d/` called `commands.conf`
or similar.

## <a id="checks-not-executed"></a> Checks are not executed

* Check the debug log to see if the check command gets executed
* Verify that failed depedencies do not prevent command execution
* Make sure that the plugin is executable by the Icinga 2 user (run a manual test)
* Make sure the [checker](8-cli-commands.md#features) feature is enabled.

Examples:

    # sudo -u icinga /usr/lib/nagios/plugins/check_ping -4 -H 127.0.0.1 -c 5000,100% -w 3000,80%

    # icinga2 feature enable checker
    The feature 'checker' is already enabled.


## <a id="notifications-not-sent"></a> Notifications are not sent

* Check the debug log to see if a notification is triggered
* If yes, verify that all conditions are satisfied
* Are any errors on the notification command execution logged?

Verify the following configuration

* Is the host/service `enable_notifications` attribute set, and if, to which value?
* Do the notification attributes `states`, `types`, `period` match the notification conditions?
* Do the user attributes `states`, `types`, `period` match the notification conditions?
* Are there any notification `begin` and `end` times configured?
* Make sure the [notification](8-cli-commands.md#features) feature is enabled.
* Does the referenced NotificationCommand work when executed as Icinga user on the shell?

If notifications are to be sent via mail make sure that the mail program specified exists.
The name and location depends on the distribution so the preconfigured setting might have to be
changed on your system.

Examples:

    # icinga2 feature enable notification
    The feature 'notification' is already enabled.

## <a id="feature-not-working"></a> Feature is not working

* Make sure that the feature configuration is enabled by symlinking from `features-available/`
to `features-enabled` and that the latter is included in [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf).
* Are the feature attributes set correctly according to the documentation?
* Any errors on the logs?

## <a id="configuration-ignored"></a> Configuration is ignored

* Make sure that the line(s) are not [commented out](16-language-reference.md#comments) (starting with `//` or `#`, or
encapsulated by `/* ... */`).
* Is the configuration file included in [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf)?

## <a id="configuration-attribute-inheritance"></a> Configuration attributes are inherited from

Icinga 2 allows you to import templates using the [import](16-language-reference.md#template-imports) keyword. If these templates
contain additional attributes, your objects will automatically inherit them. You can override
or modify these attributes in the current object.

## <a id="troubleshooting-cluster"></a> Cluster Troubleshooting

You should configure the [cluster health checks](9-monitoring-remote-systems.md#cluster-health-check) if you haven't
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

Use tools like `netstat`, `tcpdump`, `nmap`, etc to make sure that the cluster communication
happens (default port is `5665`).

    # tcpdump -n port 5665 -i any

    # netstat -tulpen | grep icinga

    # nmap yourclusternode.localdomain

### <a id="troubleshooting-cluster-ssl-errors"></a> Cluster Troubleshooting SSL Errors

If the cluster communication fails with cryptic SSL error messages, make sure to check
the following

* File permissions on the SSL certificate files
* Does the used CA match for all cluster endpoints?

Examples:

    # ls -la /etc/icinga2/pki


### <a id="troubleshooting-cluster-message-errors"></a> Cluster Troubleshooting Message Errors

At some point, when the network connection is broken or gone, the Icinga 2 instances
will be disconnected. If the connection can't be re-established between zones and endpoints,
they remain in a Split-Brain-mode and history may differ.

Although the Icinga 2 cluster protocol stores historical events in a replay log for later synchronisation,
you should make sure to check why the network connection failed.

### <a id="troubleshooting-cluster-config-sync"></a> Cluster Troubleshooting Config Sync

If the cluster zones do not sync their configuration, make sure to check the following:

* Within a config master zone, only one configuration master is allowed to have its config in `/etc/icinga2/zones.d`.
** The master syncs the configuration to `/var/lib/icinga2/api/zones/` during startup and only syncs valid configuration to the other nodes
** The other nodes receive the configuration into `/var/lib/icinga2/api/zones/`
* The `icinga2.log` log file will indicate whether this ApiListener [accepts config](9-monitoring-remote-systems.md#zone-config-sync-permissions), or not
