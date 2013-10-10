# Migrating from Icinga 1.x

## Configuration Migration

The Icinga 2 configuration format introduces plenty of behavioral changes. In
order to ease migration from Icinga 1.x,
Icinga 2 ships its own config conversion script.

### Configuration Conversion Script

Due to the complexity of the Icinga 1.x configuration format the conversion
script might not currently work for all use cases.

The config conversion script provides support for basic Icinga 1.x
configuration format conversion to native Icinga 2 configuration syntax.

The conversion script tries to preserve your existing template structure and
adds new templates where appropriate. However, the original file structure is
not preserved.

The conversion script uses templates from the Icinga Template Library where
possible.

> **Note**
>
> Please check the provided README file for additional notes and possible
> scaveats.

    # cd tools/configconvert
    # ./icinga2_convert_v1_v2.pl -c /etc/icinga/icinga.cfg -o conf/


### Manual Config Conversion

For a long-term migration of your configuration you should consider re-creating
your configuration based on the Icinga 2 proposed way of doing configuration right.

Please read the next chapter to get an idea about the differences between 1.x and 2.
