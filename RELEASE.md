# Quality Assurance

Review and test the changes and issues for this version.
https://dev.icinga.org/projects/i2/roadmap

# Release Workflow

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files:

    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS

Update the version number in the icinga2.spec file.

Update the [ChangeLog](ChangeLog), [doc/1-about.md](doc/1-about.md) files using
the changelog.py script.

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
    $ git checkout -b support/2.x
    $ git push -u origin support/2.x

# External Dependencies

## Build Server

* Build the newly created git tag for Debian/RHEL/SuSE.
* Provision the vagrant boxes and test the release packages.

## Github Release

Create a new release from the newly created git tag.
https://github.com/Icinga/icinga2/releases

## Online Documentation

Ssh into the web box, navigate into `icinga2-latest/module/icinga2`
and pull the current icinga2 revision to update what's new".

## Announcement

* Create a new blog post on www.icinga.org
* Send announcement mail to icinga-announce@lists.icinga.org
* Social media
