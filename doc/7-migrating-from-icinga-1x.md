# Migrating from Icinga 1.x

## Configuration Migration

There are plenty of differences and behavior changes introduced with the new
Icinga 2 configuration format. In order to ease migration from Icinga 1.x,
Icinga 2 ships its own config conversion script.

### Configuration Conversion Script

Due to the complexity of the Icinga 1.x configuration format the conversion
script might not work for all the use cases out there.

> **Note**
>
> While automated conversion will quickly create a working Icinga 2 configuration
> it does not keep the original organisation structure or any special kind of
> group or template logic. Please keep that mind when using the script.

The config conversion script provides support for basic Icinga 1.x
configuration format conversion to native Icinga 2 configuration syntax.

The conversion script tries to preserve your existing template structure and
adds new templates where appropriate.

The script will also detect the "attach service to hostgroup and put
hosts as members" trick from 1.x and convert that into Icinga2's template
system.

Additionally the old "service with contacts and notification commands" logic
will be converted into Icinga2's logic with new notification objects,
allowing to define notifications directly on the service definition then.

Commands will be split by type (Check, Event, Notification) and relinked where
possible. The host's `check_command` is dropped, and a possible host check service
looked up, if possible. Otherwise a new service object will be added and linked.

Notifications will be built based on the service->contact relations, and
escalations will also be merged into notifications, having times begin/end as
additional attributes. Notification options will be converted into the new Icinga2
logic.

Dependencies and Parents are resolved into host and service dependencies with
many objects tricks as well.

Timeperiods will be converted as is, because Icinga2's ITL provides the legacy-timeperiod
template which supports that format for compatibility reasons.

Custom attributes like custom variables, `*_urls`, etc will be collected into the
custom dictionary, while possible macros are automatically converted into the macro
dictionary (freely definable macros in Icinga 2).

The conversion script uses templates from the Icinga Template Library where
possible.

Regular expressions are not supported, also for the reason that this is optional
in Icinga 1.x.

> **Note**
>
> Please check the provided README file for additional notes and possible
> scaveats.

    # cd tools/configconvert
    # ./icinga2_convert_v1_v2.pl -c /etc/icinga/icinga.cfg -o conf/


### Manual Config Conversion

For a long term migration of your configuration you should consider recreating your
configuration based on the Icinga 2 proposed way of doing configuration right.

Please read the next chapter to get an idea about the differences between 1.x and 2.
