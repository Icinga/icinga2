# <a id="about-icinga2"></a> About Icinga 2

## <a id="what-is-icinga2"></a> What is Icinga 2?

Icinga 2 is an open source monitoring system which checks the availability of your
network resources, notifies users of outages and generates performance data for reporting.

Scalable and extensible, Icinga 2 can monitor complex, large environments across
multiple locations.

## <a id="licensing"></a> Licensing

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the source package.

## <a id="support"></a> Support

Support for Icinga 2 is available in a number of ways. Please have a look at
the support overview page at [https://support.icinga.org].

## <a id="contribute"></a> Contribute

There are many ways to contribute to Icinga - be it by sending patches, testing and
reporting bugs, reviewing and updating the documentation. Every contribution
is appreciated!

Please get in touch with the Icinga team at [https://www.icinga.org/ecosystem/].

## <a id="whats-new"></a> What's new

### What's New in Version 0.0.10

* Make Host and Service checkable. #5919
* Support new lines in addition to commas to separate object attributes. #5901
* Add group membership assign rules. #5910
* Support nested groups. #5858
* Add apply target type. #5924
* Add relative object names. #5925
* Merge macros and custom into 'vars' dictionary. Changed runtime macros and environment variable export. #5855
* Add support for modified attributes for custom attributes. #5956
* Allow to assign var values to existing vars evaluted on runtime. #5959
* Rename/shorten attribute names and filter variables. #5857 
* Remove the 'Icinga' prefix for global constants. #5960
* Global option to enable/disable host/service checks. #5975
* Add legacy attributes to host, service and group objects: `address{,6}'`, `notes`, `notes_url`, `action_url`, `icon_image`, `icon_image_alt`. #5856
* Support "#" hash comments. #5994
* Cluster: Spanning Tree like communication. #5467
* Properly implement the Process class for Windows. #3684

#### Changes

> **Note**
>
> Configuration updates required!

* Configuration Changes. For a detailed list of changes check out [#5909](https://dev.icinga.org/issues/5909)
* DB IDO schema upgrade required.

### Archive

Please check the `ChangeLog` file.
