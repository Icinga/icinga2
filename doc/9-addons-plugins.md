# <a id="addons-plugins"></a> Icinga 2 Addons and Plugins

## <a id="addons"></a> Addons

### <a id="addons-graphing-reporting"></a> Graphing Addons

#### <a id="addons-graphing-pnp"></a> PNP

[PNP](http://www.pnp4nagios.org) must be configured using the
[bulk mode with npcd and npcdmod](http://docs.pnp4nagios.org/pnp-0.6/modes#bulk_mode_with_npcd_and_npcdmod)
hence Icinga 2's [PerfdataWriter](3-monitoring-basics.md#performance-data) acts as npcdmod. NPCD will collect
the rotated performance data files.

#### <a id="addons-graphing-ingraph"></a> inGraph

[inGraph](https://www.netways.org/projects/ingraph/wiki) requires the ingraph-collector addon
to be configured to point at the perfdata files. Icinga 2's [PerfdataWriter](3-monitoring-basics.md#performance-data) will
write to the performance data spool directory.

#### <a id="addons-graphing-graphite"></a> Graphite

There are Graphite addons available for collecting the performance data files as well. But
natively you can use the [GraphiteWriter](3-monitoring-basics.md#graphite-carbon-cache-writer) feature.

#### <a id="addons-reporting"></a> Icinga Reporting

By enabling the DB IDO feature you can use the Icinga Reporting package.


### <a id="addons-visualization"></a> Visualization

#### <a id="addons-visualization-nagvis"></a> NagVis

By using either Livestatus or DB IDO as a backend you can create your own network maps
based on your monitoring configuration and status data using [NagVis](http://www.nagvis.org).

### <a id="addons-web-interfaces"></a> Web Interfaces

As well as the Icinga supported web interfaces (Classic UI 1.x, Web 1.x, Web 2) there are a
number of community provided web interfaces too:

* [Thruk](http://www.thruk.org) based on the [Livestatus](11-livestatus.md#setting-up-livestatus) feature


## <a id="plugins"></a> Plugins

For some services you may need additional 'check plugins' which are not provided
by the official Monitoring Plugins project.

All existing Nagios or Icinga 1.x plugins work with Icinga 2. Here's a
list of popular community sites which host check plugins:

* [Icinga Exchange](https://exchange.icinga.org)
* [Icinga Wiki](https://wiki.icinga.org)

The recommended way of setting up these plugins is to copy them to a common directory
and create a new global constant, e.g. `CustomPluginDir` in your [constants.conf](4-configuring-icinga-2.md#constants-conf)
configuration file:

    # cp check_snmp_int.pl /opt/plugins
    # chmod +x /opt/plugins/check_snmp_int.pl

    # cat /etc/icinga2/constants.conf
    /**
     * This file defines global constants which can be used in
     * the other configuration files. At a minimum the
     * PluginDir constant should be defined.
     */

    const PluginDir = "/usr/lib/nagios/plugins"
    const CustomPluginDir = "/opt/monitoring"

Prior to using the check plugin with Icinga 2 you should ensure that it is working properly
by trying to run it on the console using whichever user Icinga 2 is running as:

    # su - icinga -s /bin/bash
    $ /opt/plugins/check_snmp_int.pl --help

Additional libraries may be required for some plugins. Please consult the plugin
documentation and/or plugin provided README for installation instructions.
Sometimes plugins contain hard-coded paths to other components. Instead of changing
the plugin it might be easier to create logical links which is (more) update-safe.

Each plugin requires a [CheckCommand](5-object-types.md#objecttype-checkcommand) object in your
configuration which can be used in the [Service](5-object-types.md#objecttype-service) or
[Host](5-object-types.md#objecttype-host) object definition.

There are the following conventions to follow when adding a new command object definition:

* Always import the `plugin-check-command` template
* Use [command-arguments](#) whenever possible. The `command` attribute must be an array
in `[ ... ]` then for shell escaping.
* Define a unique `prefix` for the command's specific command arguments. That way you can safely
set them on host/service level and you'll always know which command they control.
* Use command argument default values, e.g. for thresholds
* Use [advanced conditions](5-object-types.md#objecttype-checkcommand) like `set_if` definitions.

Example for a custom `my-snmp-int` check command:

    object CheckCommand "my-snmp-int" {
      import "plugin-check-command"

      command = [ PluginDir + "/check_snmp_int.pl" ]

      arguments = {
	    "-H" = "$snmp_address$"
	    "-C" = "$snmp_community$"
		"-p" = "$snmp_port$"
		"-2" = {
          set_if = "$snmp_v2$"
		}
		"-n" = "$snmp_interface$"
		"-f" = {
			set_if = "$snmp_perf$"
		}
		"-w" = "$snmp_warn$"
		"-c" = "$snmp_crit$"
      }

      vars.snmp_v2 = true
      vars.snmp_perf = true
	  vars.snmp_warn = "300,400"
	  vars.snmp_crit = "0,600"
    }

You can find an existing `CheckCommand` definition for the `check_snmp_int.pl` plugin
shipped with the optional [Manubulon Plugin Check Command](6-icinga-template-library.md#snmp-manubulon-plugin-check-commands)
definitions already.


For further information on your monitoring configuration read the
[monitoring basics](3-monitoring-basics.md#monitoring-basics).
You can find plugins (additional to the ones at [Monitoring Plugins](https://www.monitoring-plugins.org)) over at
[Icinga Exchange](https://exchange.icinga.org)

More details on the plugins can also be found on the Icinga Wiki at https://wiki.icinga.org

## <a id="plugin-api"></a> Plugin API

Currently Icinga 2 supports the native plugin API specification from the `Monitoring Plugins`
project.

The `Monitoring Plugin API` is defined in the [Monitoring Plugins Development Guidelines](https://www.monitoring-plugins.org/doc/guidelines.html).

There are no output length restrictions using Icinga 2. This is different to the
[Icinga 1.x plugin api definition](http://docs.icinga.org/latest/en/pluginapi.html#outputlengthrestrictions).

## <a id="configuration-tools"></a> Configuration Tools

If you require your favourite configuration tool to export Icinga 2 configuration, please get in
touch with their developers. The Icinga project does not provide a configuration web interface
or similar.

> **Tip**
>
> Get to know the new configuration format and the advanced [apply](3-monitoring-basics.md#using-apply) rules and
> use [syntax highlighting](9-addons-plugins.md#configuration-syntax-highlighting) in vim/nano.

If you're looking for puppet manifests, chef cookbooks, ansible recipes, etc - we're happy
to integrate them upstream, so please get in touch at [https://support.icinga.org](https://support.icinga.org).

These tools are in development and require feedback and tests:

* [Ansible Roles](https://github.com/Icinga/icinga2-ansible)
* [Puppet Module](https://github.com/Icinga/puppet-icinga2)

## <a id="configuration-syntax-highlighting"></a> Configuration Syntax Highlighting

Icinga 2 ships configuration examples for syntax highlighting using the `vim` and `nano` editors.
The RHEL, SUSE and Debian package `icinga2-common` install these files into
`/usr/share/*/icinga2-common/syntax`. Sources provide these files in `tools/syntax`.

### <a id="configuration-syntax-highlighting-vim"></a> Configuration Syntax Highlighting using Vim

Create a new local vim configuration storage, if not already existing.
Edit `vim/ftdetect/icinga2.vim` if your paths to the Icinga 2 configuration
differ.

    $ PREFIX=~/.vim
    $ mkdir -p $PREFIX/{syntax,ftdetect}
    $ cp vim/syntax/icinga2.vim $PREFIX/syntax/
    $ cp vim/ftdetect/icinga2.vim $PREFIX/ftdetect/

Test it:

    $ vim /etc/icinga2/conf.d/templates.conf

### <a id="configuration-syntax-highlighting-nano"></a> Configuration Syntax Highlighting using Nano

Copy the `/etc/nanorc` sample file to your home directory. Create the `/etc/nano` directory
and copy the provided `icinga2.nanorc` into it.

    $ cp /etc/nanorc ~/.nanorc

    # mkdir -p /etc/nano
    # cp icinga2.nanorc /etc/nano/

Then include the icinga2.nanorc file in your ~/.nanorc by adding the following line:

    $ vim ~/.nanorc

    ## Icinga 2
    include "/etc/nano/icinga2.nanorc"

Test it:

    $ nano /etc/icinga2/conf.d/templates.conf


