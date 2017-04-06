

# <a id="contributing"></a> Contributing

#### Table of Contents

1. [Introduction][Introduction]
2. [Branches][Branches]
3. [Testing][Testing]
4. [Patches][Patches]

## <a id="contributing-intro"></a> Introduction

A roadmap of this project is located at https://github.com/Icinga/icinga2/milestones. Please consider
this roadmap when you start contributing to the project.

Before starting your work on Icinga 2, you should [fork the project](https://help.github.com/articles/fork-a-repo/)
to your GitHub account. This allows you to freely experiment with your changes.
When your changes are complete, submit a [pull request](https://help.github.com/articles/using-pull-requests/).
All pull requests will be reviewed and merged if they suit some general guidelines:

* Changes are located in a topic branch
* For new functionality, proper tests are written
* Changes should not solve certain problems on special environments

## <a id="contributing-branches"></a> Branches

Choosing a proper name for a branch helps us identify its purpose and possibly find an associated bug or feature.
Generally a branch name should include a topic such as `fix` or `feature` followed by a description and an issue number
if applicable. Branches should have only changes relevant to a specific issue.

```
git checkout -b fix/service-template-typo-1234
git checkout -b feature/config-handling-1235
```

## <a id="contributing-testing"></a> Testing

Basic unit test coverage is provided by running `make test` during package builds.
Read the [INSTALL.md](INSTALL.md) file for more information about development builds.

Snapshot packages from the laster development branch are available inside the
[package repository](https://packages.icinga.com).

You can help test-drive the latest Icinga 2 snapshot packages inside the
[Icinga 2 Vagrant boxes](https://github.com/icinga/icinga-vagrant).


## <a id="contributing-patches"></a> Patches

### <a id="contributing-source-code"></a> Source Code

Icinga 2 is written in C++ and uses the Boost libraries. We are also using the C++11 standard where applicable (please
note the minimum required compiler versions in the [INSTALL.md](INSTALL.md) file.

Icinga 2 can be built on Linux/Unix and Windows clients. In order to develop patches for Icinga 2,
you should prepare your own local build environment and know how to work with C++.

More tips:

* Requirements and source code installation is explained inside the [INSTALL.md](INSTALL.md) file.
* Debug requirements and GDB instructions can be found in the [documentation](https://github.com/Icinga/icinga2/blob/master/doc/20-development.md).
* If you are planning to debug a Windows client, setup a Windows environment with [Visual Studio](https://www.visualstudio.com/vs/community/). An example can be found in [this blogpost](https://blog.netways.de/2015/08/24/developing-icinga-2-on-windows-10-using-visual-studio-2015/).

### <a id="contributing-documentation"></a> Update the Documentation

The documentation is written in GitHub flavored [Markdown](https://guides.github.com/features/mastering-markdown/).
It is located in the `doc/` directory and can be edited with your preferred editor. You can also
edit it online on GitHub.

```
vim doc/2-getting-started.md
```

In order to review and test changes, you can install the [mkdocs](http://www.mkdocs.org) Python library.

```
pip install mkdocs
```

This allows you to start a local mkdocs viewer instance on http://localhost:8000

```
mkdocs serve
```

Changes on the chapter layout can be done inside the `mkdocs.yml` file in the main tree.

There also is a script to ensure that relative URLs to other sections are updated. This script
also checks for broken URLs.

```
./doc/update-links.py doc/*.md
```

### <a id="contributing-itl-checkcommands"></a> Contribute CheckCommand Definitions

The Icinga Template Library (ITL) and its plugin check commands provide a variety of CheckCommand
object definitions which can be included on-demand.

Advantages of sending them upstream:

* Everyone can use and update/fix them.
* One single place for configuration and documentation.
* Developers may suggest updates and help with best practices.
* You don't need to care about copying the command definitions to your satellites and clients.

#### <a id="contributing-itl-checkcommands-start"></a> Where do I start?

Get to know the check plugin and its options. Read the general documentation on how to integrate
your check plugins and how to create a good CheckCommand definition.

A good command definition uses:

* Command arguments including `value`, `description`, optional: `set_if`, `required`, etc.
* Comments `/* ... */` to describe difficult parts.
* Command name as prefix for the custom attributes referenced (e.g. `disk_`)
* Default values
	* If `host.address` is involved, set a custom attribute (e.g. `ping_address`) to the default `$address$`. This allows users to override the host's address later on by setting the custom attribute inside the service apply definitions.
	* If the plugin is also capable to use ipv6, import the `ipv4-or-ipv6` template and use `$check_address$` instead of `$address$`. This allows to fall back to ipv6 if only this address is set.
	* If `set_if` is involved, ensure to specify a sane default value if required.
* Templates if there are multiple plugins with the same basic behaviour (e.g. ping4 and ping6).
* Your love and enthusiasm in making it the perfect CheckCommand.

#### <a id="contributing-itl-checkcommands-overview"></a> I have created a CheckCommand, what now?

Icinga 2 developers love documentation. This isn't just because we want to annoy anyone sending a patch,
it's a matter of making your contribution visible to the community.

Your patch should consist of 2 parts:

* The CheckCommand definition.
* The documentation bits.

[Fork the repository](https://help.github.com/articles/fork-a-repo/) and ensure that the master branch is up-to-date.

Create a new fix or feature branch and start your work.

```
git checkout -b feature/itl-check-printer
```

#### <a id="contributing-itl-checkcommands-add"></a> Add CheckCommand Definition to Contrib Plugins

There already exists a defined structure for contributed plugins. Navigate to `itl/plugins-contrib.d`
and verify where your command definitions fits into.

```
cd itl/plugins-contrib.d/
ls
```

If you want to add or modify an existing Monitoring Plugin please use `itl/command-plugins.conf` instead.

```
vim itl/command-plugins-conf
```

##### Existing Configuration File

Just edit it, and add your CheckCommand definition.

```
vim operating-system.conf
```

Proceed to the documentation.

##### New type for CheckCommand Definition

Create a new file with .conf suffix.

```
	$ vim printer.conf
```

Add the file to `itl/CMakeLists.txt` in the FILES line in **alpha-numeric order**.
This ensures that the installation and packages properly include your newly created file.

```
vim CMakeLists.txt

-FILES ipmi.conf network-components.conf operating-system.conf virtualization.conf vmware.conf
+FILES ipmi.conf network-components.conf operating-system.conf printer.conf virtualization.conf vmware.conf
```

Add the newly created file to your git commit.

```
git add printer.conf
```

Do not commit it yet but finish with the documentation.

#### <a id="contributing-itl-checkcommands-docs"></a> Create CheckCommand Documentation

Edit the documentation file in the `doc/` directory. More details on documentation
updates can be found [here](CONTRIBUTING.md#contributing-documentation).

```
vim doc/7-icinga-template-library.md
```

The CheckCommand documentation should be located in the same chapter
similar to the configuration file you have just added/modified.

Create a section for your plugin, add a description and a table of parameters. Each parameter should have at least:

* optional or required
* description of its purpose
* the default value, if any

Look at the existing documentation and "copy" the same style and layout.


#### <a id="contributing-itl-checkcommands-patch"></a> Send a Patch

Commit your changes which includes a descriptive commit message.

```
git commit -av
Add printer CheckCommand definition

Explain its purpose and possible enhancements/shortcomings.

refs #existingticketnumberifany
```
Push the branch to the remote origin and create a [pull request](https://help.github.com/articles/using-pull-requests/).

```
git push --set-upstream origin feature/itl-check-printer
hub pull-request
```

In case developers ask for changes during review, please add them
to the branch and push those changes.

<!-- TOC URLs -->
[Introduction]: #contributing-intro
[Branches]: #contributing-branches
[Testing]: #contributing-testing
[Patches]: #contributing-patches
