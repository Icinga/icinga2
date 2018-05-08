# Release Workflow <a id="release-workflow"></a>

#### Table of Content

- [1. Preparations](#preparations)
  - [1.1. Issues](#issues)
  - [1.2. Backport Commits](#backport-commits)
  - [1.3. Authors](#authors)
- [2. Version](#version)
- [3. Changelog](#changelog)
- [4. Git Tag](#git-tag)
- [5. Package Builds](#package-builds)
  - [5.1. RPM Packages](#rpm-packages)
  - [5.2. DEB Packages](#deb-packages)
- [6. Build Server](#build-server)
- [7. Release Tests](#release-tests)
- [8. GitHub Release](#github-release)
- [9. Chocolatey](#chocolatey)
- [10. Post Release](#post-release)
  - [10.1. Online Documentation](#online-documentation)
  - [10.2. Announcement](#announcement)
  - [10.3. Project Management](#project-management)

## Preparations <a id="preparations"></a>

Specify the release version.

```
VERSION=2.8.3
```

Add your signing key to your Git configuration file, if not already there.

```
vim $HOME/.gitconfig

[user]
        email = michael.friedrich@icinga.com
        name = Michael Friedrich
        signingkey = D14A1F16
```

### Issues <a id="issues"></a>

Check issues at https://github.com/Icinga/icinga2

### Backport Commits <a id="backport-commits"></a>

For minor versions you need to manually backports any and all commits from the
master branch which should be part of this release.

### Authors <a id="authors"></a>

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files:

```
git checkout master
git log --use-mailmap | grep '^Author:' | cut -f2- -d' ' | sort | uniq > AUTHORS
```

## Version <a id="version"></a>

Update the version in the version file:

```
sed -i "s/Version: .*/Version: $VERSION/g" VERSION
```

## Changelog <a id="changelog"></a>

Update the [CHANGELOG.md](CHANGELOG.md) file.

Export these environment variables:

```
export ICINGA_GITHUB_AUTH_USERNAME='user'
export ICINGA_GITHUB_AUTH_TOKEN='token'
export ICINGA_GITHUB_PROJECT='icinga/icinga2'
```

Run the script which updates the [CHANGELOG.md](CHANGELOG.md) file.

```
./changelog.py
git diff
```

## Git Tag  <a id="git-tag"></a>

> **Major Releases**: Commit these changes to the `master` branch.
>
> **Minor Releases**: Commit changes to the `support` branch.

```
git commit -v -a -m "Release version $VERSION"
```

Create a signed tag (tags/v<VERSION>) on the `master` branch (for major
releases) or the `support` branch (for minor releases).

```
git tag -s -m "Version $VERSION" v$VERSION
```

Push the tag:

```
git push --tags
```

**For major releases:** Create a new `support` branch:

```
git checkout master
git checkout -b support/2.9
git push -u origin support/2.9
```

**For minor releases:** Push the support branch, cherry-pick the release commit
into master and merge the support branch:

```
git push -u origin support/2.8
git checkout master
git cherry-pick support/2.8
git merge --strategy=ours support/2.8
git push origin master
```

## Package Builds  <a id="package-builds"></a>

### RPM Packages  <a id="rpm-packages"></a>

```
git clone git@github.com:icinga/rpm-icinga2.git && cd rpm-icinga2
```

#### Branch Workflow

**Major releases** are branched off `master`.

```
git checkout master && git pull
```

**Bugfix releases** are created in the `release` branch and later merged to master.

```
git checkout release && git pull
```

#### Release Commit

Set the `Version`, `Revision` and `changelog` inside the spec file.

```
sed -i "s/Version: .*/Version: $VERSION/g" icinga2.spec

vim icinga2.spec

%changelog
* Tue Apr 24 2018 Jean Flach <jean.flach@icinga.com> 2.8.3-1
- Update to 2.8.3
```

```
git commit -av -m "Release 2.8.3-1"
git push
```

**Note for major releases**: Update release branch to latest.
`git checkout release && git pull && git merge master && git push`

**Note for minor releases**: Cherry-pick the release commit into master.
`git checkout master && git pull && git cherry-pick release && git push`


### DEB Packages  <a id="deb-packages"></a>

```
git clone git@github.com:icinga/deb-icinga2.git && cd deb-icinga2
```

#### Branch Workflow

**Major releases** are branched off `master`.

```
git checkout master && git pull
```

**Bugfix releases** are created in the `release` branch and later merged to master.

```
git checkout release && git pull
```

#### Release Commit

Set the `Version`, `Revision` and `changelog` inside the spec file.

```
./dch 2.8.3-1 "Update to 2.8.3"
```

```
git commit -av -m "Release 2.8.3-1"
git push
```

```
git commit -av -m "Release 2.8.3-1"
```

**Note for major releases**: Update release branch to latest.
`git checkout release && git pull && git merge master && git push`

**Note for minor releases**: Cherry-pick the release commit into master.
`git checkout master && git pull && git cherry-pick release && git push`


#### DEB with dch on macOS

```
docker run -v `pwd`:/mnt/packaging -ti ubuntu:xenial bash

apt-get update
apt-get install git dev-tools vim
cd /mnt/packaging

git config --global user.name "Michael Friedrich"
git config --global user.email "michael.friedrich@icinga.com"

./dch 2.8.3-1 "Update to 2.8.3"
```


## Build Server <a id="build-server"></a>

* Verify package build changes for this version.
* Test the snapshot packages for all distributions beforehand.
* Build the newly created Git tag for Debian/RHEL/SuSE.
* Build the newly created Git tag for Windows.

## Release Tests  <a id="release-tests"></a>

* Test DB IDO with MySQL and PostgreSQL.
* Provision the vagrant boxes and test the release packages.
* Test the [setup wizard](https://packages.icinga.com/windows/) inside a Windows VM.
* Start a new docker container and install/run icinga2.

Example for CentOS7:

```
docker run -ti centos:latest bash

yum -y install https://packages.icinga.com/epel/icinga-rpm-release-7-latest.noarch.rpm
yum -y install icinga2
icinga2 daemon -C
```

## GitHub Release  <a id="github-release"></a>

Create a new release for the newly created Git tag: https://github.com/Icinga/icinga2/releases

## Chocolatey  <a id="chocolatey"></a>

Navigate to the git repository on your Windows box which
already has chocolatey installed. Pull/checkout the release.

Create the nupkg package:

```
cpack
```

Install the created icinga2 package locally:

```
choco install icinga2 -version 2.8.3 -fdv "%cd%" -source "'%cd%;https://chocolatey.org/api/v2/'"
```

Upload the package to [chocolatey](https://chocolatey.org/packages/upload).

## Post Release  <a id="post-release"></a>

### Online Documentation  <a id="online-documentation"></a>

Ask @bobapple to update the documentation at docs.icinga.com.

### Announcement  <a id="announcement"></a>

* Create a new blog post on www.icinga.com/blog
* Social media: [Twitter](https://twitter.com/icinga), [Facebook](https://www.facebook.com/icinga), [G+](https://plus.google.com/+icinga), [Xing](https://www.xing.com/communities/groups/icinga-da4b-1060043), [LinkedIn](https://www.linkedin.com/groups/Icinga-1921830/about)
* Update IRC channel topic

### Project Management  <a id="project-management"></a>

* Add new minor version on [GitHub](https://github.com/Icinga/icinga2/milestones).
* Close the released version on [GitHub](https://github.com/Icinga/icinga2/milestones).
