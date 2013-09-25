Configuration Introduction
==========================

In Icinga 2 configuration is based on objects. There’s no difference in
defining global settings for the core application or for a specific
runtime configuration object.

There are different types for the main application, its components and
tools. The runtime configuration objects such as hosts, services, etc
are defined using the same syntax.

Each configuration object must be unique by its name. Otherwise Icinga 2
will bail early on verifying the parsed configuration.

Main Configuration
==================

Starting Icinga 2 requires the main configuration file called
"icinga2.conf". That’s the location where everything is defined or
included. Icinga 2 will only know the content of that file and included
configuration file snippets.

    # /usr/bin/icinga2 -c /etc/icinga2/icinga2.conf

> **Note**
>
> You can use just the main configuration file and put everything in
> there. Though that is not advised because configuration may be
> expanded over time. Rather organize runtime configuration objects into
> their own files and/or directories and include that in the main
> configuration file.

Configuration Syntax
====================

/\* TODO \*/

Details on the syntax can be found in the chapter
icinga2-config-syntax.html[Configuration Syntax]

Configuration Types
===================

/\* TODO \*/

Details on the available types can be found in the chapter
icinga2-config-types.html[Configuration Types]

Configuration Templates
=======================

Icinga 2 ships with the **Icinga Template Library (ITL)**. This is a set
of predefined templates and definitions available in your actual
configuration.

> **Note**
>
> Do not change the ITL’s files. They will be overridden on upgrade.
> Submit a patch upstream or include your very own configuration
> snippet.

Include the basic ITL set in your main configuration like

    include <itl/itl.conf>

> **Note**
>
> Icinga 2 recognizes the ITL’s installation path and looks for that
> specific file then.

Having Icinga 2 installed in standalone mode make sure to include
itl/standalone.conf as well (see sample configuration).

    include <itl/standalone.conf>

/\* vim: set syntax=asciidoc filetype=asciidoc: \*/
