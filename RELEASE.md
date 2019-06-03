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
VERSION=2.10.4
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

Update the version:

```
sed -i "s/Version: .*/Version: $VERSION/g" VERSION
```

## Changelog <a id="changelog"></a>

Link to the milestone and closed=1 as filter.

Manually update the best of collected from the
milestone description.

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
git push

git checkout -b support/2.11
git push -u origin support/2.11
```

**For minor releases:** Push the support branch, cherry-pick the release commit
into master and merge the support branch:

```
git push -u origin support/2.10
git checkout master
git cherry-pick support/2.10
git merge --strategy=ours support/2.10
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
VERSION=2.10.4

sed -i "s/Version: .*/Version: $VERSION/g" icinga2.spec

vim icinga2.spec

%changelog
* Tue Mar 19 2019 Michael Friedrich <michael.friedrich@icinga.com> 2.10.4-1
- Update to 2.10.4
```

```
git commit -av -m "Release $VERSION-1"
git push
```

**Note for major releases**: Update release branch to latest.

```
git checkout release && git pull && git merge master && git push
```

**Note for minor releases**: Cherry-pick the release commit into master.

```
git checkout master && git pull && git cherry-pick release && git push
```


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

Set the `Version`, `Revision` and `changelog` by using the `dch` helper.

```
VERSION=2.10.4

./dch $VERSION-1 "Update to $VERSION"
```

```
git commit -av -m "Release $VERSION-1"
git push
```


**Note for major releases**: Update release branch to latest.

```
git checkout release && git pull && git merge master && git push
```

**Note for minor releases**: Cherry-pick the release commit into master.

```
git checkout master && git pull && git cherry-pick release && git push
```

#### DEB with dch on macOS

```
docker run -v `pwd`:/mnt/packaging -ti ubuntu:bionic bash

apt-get update && apt-get install git ubuntu-dev-tools vim -y
cd /mnt/packaging

git config --global user.name "Michael Friedrich"
git config --global user.email "michael.friedrich@icinga.com"

VERSION=2.10.4

./dch $VERSION-1 "Update to $VERSION"
```


## Build Server <a id="build-server"></a>

* Verify package build changes for this version.
* Test the snapshot packages for all distributions beforehand.
* Build the newly created Git tag for Debian/RHEL/SuSE.
  * Wait until all jobs have passed and then publish them one by one with `allow_release`
* Build the newly created Git tag for Windows: `refs/tags/v2.10.0` as source and `v2.10.0` as package name.

## Release Tests  <a id="release-tests"></a>

* Test DB IDO with MySQL and PostgreSQL.
* Provision the vagrant boxes and test the release packages.
* Test the [setup wizard](https://packages.icinga.com/windows/) inside a Windows VM.
* Start a new docker container and install/run icinga2.

### CentOS

```
docker run -ti centos:latest bash

yum -y install https://packages.icinga.com/epel/icinga-rpm-release-7-latest.noarch.rpm
yum -y install icinga2
icinga2 daemon -C
```

### Debian

```
docker run -ti debian:stretch bash

apt-get update && apt-get install -y wget curl gnupg apt-transport-https

DIST=$(awk -F"[)(]+" '/VERSION=/ {print $2}' /etc/os-release); \
 echo "deb https://packages.icinga.com/debian icinga-${DIST} main" > \
 /etc/apt/sources.list.d/${DIST}-icinga.list
 echo "deb-src https://packages.icinga.com/debian icinga-${DIST} main" >> \
 /etc/apt/sources.list.d/${DIST}-icinga.list

curl https://packages.icinga.com/icinga.key | apt-key add -
apt-get -y install icinga2
icinga2 daemon
```

## GitHub Release  <a id="github-release"></a>

Create a new release for the newly created Git tag: https://github.com/Icinga/icinga2/releases

> Hint: Choose [tags](https://github.com/Icinga/icinga2/tags), pick one to edit and
> make this a release. You can also create a draft release.

The release body should contain a short changelog, with links
into the roadmap, changelog and blogpost.


## Chocolatey  <a id="chocolatey"></a>

Navigate to the git repository on your Windows box which
already has chocolatey installed. Pull/checkout the release.

Create the nupkg package (or use the one generated on https://packages.icinga.com/windows):

```
cpack
```

Fetch the API key from https://chocolatey.org/account and use the `choco push`
command line.

```
choco apikey --key xxx --source https://push.chocolatey.org/

choco push Icinga2-v2.10.0.nupkg --source https://push.chocolatey.org/
```


## Post Release  <a id="post-release"></a>

### Online Documentation  <a id="online-documentation"></a>

> Only required for major releases.

Navigate to `puppet-customer/icinga.git` and do the following steps:

#### Testing

```
git checkout testing && git pull
vim files/var/www/docs/config/icinga2-latest.yml

git commit -av -m "icinga-web: Update docs for Icinga 2"

git push
```

SSH into the webserver and do a manual Puppet dry run with the testing environment.

```
puppet agent -t --environment testing --noop
```

Once succeeded, continue with production deployment.

#### Production

```
git checkout master && git pull
git merge testing
git push
```

SSH into the webserver and do a manual Puppet run from the production environment (default).

```
puppet agent -t
```

#### Manual Generation

SSH into the webserver or ask @bobapple.

```
cd /usr/local/icinga-docs-tools && ./build-docs.rb -c /var/www/docs/config/icinga2-latest.yml
```

### Announcement  <a id="announcement"></a>

* Create a new blog post on [icinga.com/blog](https://icinga.com/blog) including a featured image
* Create a release topic on [community.icinga.com](https://community.icinga.com)
* Release email to net-tech & team

### Project Management  <a id="project-management"></a>

* Add new minor version on [GitHub](https://github.com/Icinga/icinga2/milestones).
