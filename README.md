[![Build Status](https://travis-ci.org/Icinga/icinga2.svg?branch=master)](https://travis-ci.org/Icinga/icinga2)

# Icinga 2

![Icinga Logo](https://www.icinga.com/wp-content/uploads/2014/06/icinga_logo.png)

#### Table of Contents

1. [About][About]
2. [License][License]
3. [Installation][Installation]
4. [Documentation][Documentation]
5. [Support][Support]
6. [Development and Contributions][Development]

## About

Icinga 2 is an open source monitoring system which checks the availability of your
network resources, notifies users of outages, and generates performance data for reporting.

Scalable and extensible, Icinga 2 can monitor large, complex environments across
multiple locations.

Icinga 2 as monitoring core works best with [Icinga Web 2](https://www.icinga.com/products/icinga-web-2/)
as web interface.

More information can be found at [www.icinga.com](https://www.icinga.com/products/icinga-2/)
and inside the [documentation](doc/1-about.md).

## License

Icinga 2 and the Icinga 2 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
COPYING file included in the source package.

## Installation

Read the [INSTALL.md](INSTALL.md) file for more information about how to install it.

## Documentation

The documentation is located in the [doc/](doc/) directory. The latest documentation
is also available on https://docs.icinga.com

## Support

Check the project website at https://www.icinga.com for status updates. Join the
[community channels](https://www.icinga.com/community/get-involved/) for questions
or ask an Icinga partner for [professional support](https://www.icinga.com/services/support/).

## Development

The Git repository is located on [GitHub](https://github.com/Icinga/icinga2).

Icinga 2 is written in C++ and can be built on Linux/Unix and Windows.
Read more about development builds in the [INSTALL.md](INSTALL.md) file.

### Contributing

There are many ways to contribute to Icinga -- whether it be sending patches,
testing, reporting bugs, or reviewing and updating the documentation. Every
contribution is appreciated!

Read the [contributing section](https://www.icinga.com/community/get-involved/) and
get familiar with the code.

Pull requests on [GitHub](https://github.com/Icinga/icinga2) are preferred.

### Testing

Basic unit test coverage is provided by running `make test` during package builds.
Read the [INSTALL.md](INSTALL.md) file for more information about development builds.

Snapshot packages from the laster development branch are available inside the
[package repository](http://packages.icinga.com).

You can help test-drive the latest Icinga 2 snapshot packages inside the
[Icinga 2 Vagrant boxes](https://github.com/icinga/icinga-vagrant).


[About]: #about
[License]: #license
[Installation]: #installation
[Documentation]: #documentation
[Support]: #support
[Development]: #development
