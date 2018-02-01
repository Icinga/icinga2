# Icinga Template Library <a id="icinga-template-library"></a>

The Icinga Template Library (ITL) implements standard templates and object definitions.

Beginning with Icinga 2.9 this was split from the Icinga 2 source code.

Here are the new locations for the ITL:

<!-- TODO: check URLs before merge!!! -->
* [Source on GitHub](https://github.com/Icinga/icinga-template-library)
* [Documentation][https://icinga.com/docs/icinga-template-library/latest]

<!-- TODO: verify package name -->
When you have installed Icinga via packages the templates should automatically be available under the package
`icinga2-templates` in your system. They are usually installed at `/usr/share/icinga2/includes`.

To find the documentation for what you are searching for, follow one of the links:

<!-- TODO: check URLs before merge!!! -->
* [Generic ITL templates](https://icinga.com/docs/icinga-template-library/latest/doc/09-generic-templates)
* [CheckCommand definitions for Icinga 2 Internal Checks](https://icinga.com/docs/icinga-template-library/latest/doc/10-icinga-internal)
* [CheckCommand definitions for Monitoring Plugins](https://icinga.com/docs/icinga-template-library/latest/doc/11-monitoring-plugins)
* [CheckCommand definitions for Icinga 2 Windows Plugins](https://icinga.com/docs/icinga-template-library/latest/doc/12-windows-plugins)
* [CheckCommand definitions for NSClient++](https://icinga.com/docs/icinga-template-library/latest/doc/13-nsclient)
* [CheckCommand definitions for Manubulon SNMP](https://icinga.com/docs/icinga-template-library/latest/doc/14-manubulon)
* Contributed CheckCommand definitions, please see the [Documentation](https://www.icinga.com/docs/icinga2/latest/) index

The ITL content is usually updated with packages. Please do not modify templates and/or objects as changes will be
overridden without further notice.

You are advised to create your own CheckCommand definitions, or modifications,
in `/etc/icinga2` or with the Icinga Director.
