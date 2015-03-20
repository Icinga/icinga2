# Release Workflow

Here's a short check-list for releases:

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

