# Quality Assurance

Review and test the changes and issues for this version.
https://dev.icinga.org/projects/i2/roadmap

# Release Workflow

## Authors

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files:

    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS

## Version

Update the version number in the following files:

* [icinga2.spec]: Version: (.*)
* [icinga2.nuspec]: <version>(.*)</version>
* [tools/chocolateyInstall.ps1]: Icinga2-v(.*).exe

## Changelog

Update the [ChangeLog](ChangeLog), [doc/1-about.md](doc/1-about.md) files using
the changelog.py script. Also generate HTML for the wordpress release announcement.

Changelog:

    $ ./changelog.py --version 2.3.5 --project i2

Docs:

    $ ./changelog.py --version 2.3.5 --project i2 --links

Wordpress:

    $ ./changelog.py --version 2.3.5 --project i2 --html --links

## Git Tag

Commit these changes to the "master" branch:

    $ git commit -v -a -m "Release version <VERSION>"

For minor releases: Cherry-pick this commit into the "support" branch.

Create a signed tag (tags/v<VERSION>) on the "master" branch (for major
releases) or the "support" branch (for minor releases).

GB:

    $ git tag -u EE8E0720 -m "Version <VERSION>" v<VERSION>

MF:

    $ git tag -u D14A1F16 -m "Version <VERSION>" v<VERSION>

Push the tag.

    $ git push --tags

For major releases: Create a new "support" branch:

    $ git checkout master
    $ git checkout -b support/2.3
    $ git push -u origin support/2.3

For minor releases: Push the support branch and cherry-pick the release commit into master:

    $ git push -u origin support/2.3
    $ git checkout master
    $ git cherry-pick support/2.3
    $ git push origin master

# External Dependencies

## Build Server

### Linux

* Build the newly created git tag for Debian/RHEL/SuSE.
* Provision the vagrant boxes and test the release packages.
* Start a new docker container and install/run icinga2

Example for CentOS7:

    $ sudo docker run -ti centos:latest bash

    # yum -y install http://packages.icinga.org/epel/7/release/noarch/icinga-rpm-release-7-1.el7.centos.noarch.rpm
    # yum -y install icinga2
    # icinga2 daemon -C

    # systemctl start icinga2
    # tail -f /var/log/icinga2/icinga2.log

### Windows

* Build the newly created git tag for Windows.
* Test the [setup wizard](http://packages.icinga.org/windows/) inside a Windows VM.

## Github Release

Create a new release from the newly created git tag.
https://github.com/Icinga/icinga2/releases

## Online Documentation

Ssh into the web box, navigate into `icinga2-latest/module/icinga2`
and pull the current icinga2 revision to update what's new".

## Announcement

* Create a new blog post on www.icinga.org/blog
* Send announcement mail to icinga-announce@lists.icinga.org
* Social media: [Twitter](https://twitter.com/icinga), [Facebook](https://www.facebook.com/icinga), [G+](http://plus.google.com/+icinga), [Xing](https://www.xing.com/communities/groups/icinga-da4b-1060043), [LinkedIn](https://www.linkedin.com/groups/Icinga-1921830/about)
