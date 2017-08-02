# Release Workflow

Specify the release version.

    VERSION=2.7.0

## Issues

Check issues at https://github.com/Icinga/icinga2

## Backport Commits

For minor versions you need to manually backports any and all commits from the
master branch which should be part of this release.

## Authors

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files:

    $ git checkout master
    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS

## Version

Update the version number in the following file:

* [icinga2.spec]: Version: (.*)

Example:

    gsed -i "s/Version: .*/Version: $VERSION/g" icinga2.spec

## Changelog

Update the [CHANGELOG.md](CHANGELOG.md) file.

## Git Tag

Commit these changes to the "master" branch:

    $ git commit -v -a -m "Release version $VERSION"

For minor releases: Cherry-pick this commit into the "support" branch.

Create a signed tag (tags/v<VERSION>) on the "master" branch (for major
releases) or the "support" branch (for minor releases).

GB:

    $ git tag -u EE8E0720 -m "Version $VERSION" v$VERSION

MF:

    $ git tag -u D14A1F16 -m "Version $VERSION" v$VERSION

Push the tag.

    $ git push --tags

For major releases: Create a new "support" branch:

    $ git checkout master
    $ git checkout -b support/2.7
    $ git push -u origin support/2.7

For minor releases: Push the support branch, cherry-pick the release commit
into master and merge the support branch:

    $ git push -u origin support/2.7
    $ git checkout master
    $ git cherry-pick support/2.7
    $ git merge --strategy=ours support/2.7
    $ git push origin master

# External Dependencies

## Build Server

* Verify package build changes for this version.
* Test the snapshot packages for all distributions beforehand.
* Build the newly created Git tag for Debian/RHEL/SuSE.
* Build the newly created Git tag for Windows.

## Release Tests

* Test DB IDO with MySQL and PostgreSQL.
* Provision the vagrant boxes and test the release packages.
* Test the [setup wizard](https://packages.icinga.com/windows/) inside a Windows VM.

* Start a new docker container and install/run icinga2.

Example for CentOS7:

    $ docker run -ti centos:latest bash

    # yum -y install https://packages.icinga.com/epel/7/release/noarch/icinga-rpm-release-7-2.el7.centos.noarch.rpm
    # yum -y install icinga2
    # icinga2 daemon -C

    # systemctl start icinga2
    # tail -f /var/log/icinga2/icinga2.log

## GitHub Release

Create a new release for the newly created Git tag.
https://github.com/Icinga/icinga2/releases

## Chocolatey

Navigate to the git repository on your Windows box which
already has chocolatey installed. Pull/checkout the release.

Create the nupkg package:

    cpack

Install the created icinga2 package locally:

    choco install icinga2 -version 2.7.0 -fdv "%cd%" -source "'%cd%;https://chocolatey.org/api/v2/'"

Upload the package to [chocolatey](https://chocolatey.org/packages/upload).

## Online Documentation

SSH into the web box, navigate into `icinga2-latest/module/icinga2`
and pull the current support branch.

## Announcement

* Create a new blog post on www.icinga.com/blog
* Send announcement mail to icinga-announce@lists.icinga.org
* Social media: [Twitter](https://twitter.com/icinga), [Facebook](https://www.facebook.com/icinga), [G+](https://plus.google.com/+icinga), [Xing](https://www.xing.com/communities/groups/icinga-da4b-1060043), [LinkedIn](https://www.linkedin.com/groups/Icinga-1921830/about)
* Update IRC channel topic

# After the release

* Add new minor version on [GitHub](https://github.com/Icinga/icinga2/milestones).
* Close the released version on [GitHub](https://github.com/Icinga/icinga2/milestones).
