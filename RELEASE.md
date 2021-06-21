# Release Workflow <a id="release-workflow"></a>

#### Table of Content

- [1. Preparations](#preparations)
  - [1.1. Issues](#issues)
  - [1.2. Backport Commits](#backport-commits)
  - [1.3. Windows Dependencies](#windows-dependencies)
- [2. Version](#version)
- [3. Changelog](#changelog)
- [4. Git Tag](#git-tag)
- [5. Package Builds](#package-builds)
  - [5.1. RPM Packages](#rpm-packages)
  - [5.2. DEB Packages](#deb-packages)
- [6. Build Server](#build-infrastructure)
- [7. Release Tests](#release-tests)
- [8. GitHub Release](#github-release)
- [9. Docker](#docker)
- [10. Post Release](#post-release)
  - [10.1. Online Documentation](#online-documentation)
  - [10.2. Announcement](#announcement)
  - [10.3. Project Management](#project-management)

## Preparations <a id="preparations"></a>

Specify the release version.

```bash
VERSION=2.11.0
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

### Windows Dependencies <a id="windows-dependencies"></a>

In contrast to Linux, the Windows dependencies (at least Boost and OpenSSL)
are neither updated independently of Icinga itself by instance administrators,
nor their latest versions available for the OS are fetched at package build time.

To ensure the upcoming Icinga release ships the latest (i.e. most secure) dependencies on Windows:

1. Update their versions in [doc/win-dev.ps1](doc/win-dev.ps1) if any new ones are available
2. Let the GitHub actions verify the new versions (step one affects our GitHub actions)
3. Update also https://git.icinga.com/infra/ansible-windows-build and https://git.icinga.com/packaging/windows-icinga2


## Version <a id="version"></a>

Update the version:

```bash
perl -pi -e "s/Version: .*/Version: $VERSION/g" ICINGA2_VERSION
```

## Changelog <a id="changelog"></a>

Choose the most important issues and summarize them in multiple groups/paragraphs. Provide links to the mentioned
issues/PRs. At the start include a link to the milestone's closed issues.


## Git Tag  <a id="git-tag"></a>

```bash
git commit -v -a -m "Release version $VERSION"
```

Create a signed tag (tags/v<VERSION>) on the `master` branch (for major
releases) or the `support` branch (for minor releases).

```bash
git tag -s -m "Version $VERSION" v$VERSION
```

Push the tag:

```bash
git push origin v$VERSION
```

**For major releases:** Create a new `support` branch:

```bash
git checkout master
git push

git checkout -b support/2.12
git push -u origin support/2.12
```


## Package Builds  <a id="package-builds"></a>

```bash
mkdir $HOME/dev/icinga/packaging
cd $HOME/dev/icinga/packaging
```

### RPM Packages  <a id="rpm-packages"></a>

```bash
git clone git@git.icinga.com:packaging/rpm-icinga2.git && cd rpm-icinga2
```

### DEB Packages <a id="deb-packages"></a>

```bash
git clone git@git.icinga.com:packaging/deb-icinga2.git && cd deb-icinga2
```

#### Raspbian Packages

```bash
git clone git@git.icinga.com:packaging/raspbian-icinga2.git && cd raspbian-icinga2
```

### Windows Packages

```bash
git clone git@git.icinga.com:packaging/windows-icinga2.git && cd windows-icinga2
```


### Branch Workflow

For each support branch in this repo (e.g. support/2.12), there exists a corresponding branch in the packaging repos
(e.g. 2.12). Each package revision is a tagged commit on these branches. When doing a major release, create the new
branch, otherweise switch to the existing one.


### Switch Build Type

Ensure that `ICINGA_BUILD_TYPE` is set to `release` in `.gitlab-ci.yml`. This should only be necessary after creating a
new branch.

```yaml
variables:
  ...
  ICINGA_BUILD_TYPE: release
  ...
```

Commit the change.

```bash
git commit -av -m "Switch build type for 2.13"
```

#### RPM Release Preparations

Set the `Version`, `revision` and `%changelog` inside the spec file:

```
perl -pi -e "s/Version:.*/Version:        $VERSION/g" icinga2.spec

vim icinga2.spec

%changelog
* Thu Sep 19 2019 Michael Friedrich <michael.friedrich@icinga.com> 2.11.0-1
- Update to 2.11.0
```

#### DEB and Raspbian Release Preparations

Update file `debian/changelog` and add at the beginning:

```
icinga2 (2.11.0-1) icinga; urgency=medium

  * Release 2.11.0

 -- Michael Friedrich <michael.friedrich@icinga.com>  Thu, 19 Sep 2019 10:50:31 +0200
```


#### Windows Release Preparations

Update the file `.gitlab-ci.yml`:

```
perl -pi -e "s/^  UPSTREAM_GIT_BRANCH: .*/  UPSTREAM_GIT_BRANCH: v$VERSION/g" .gitlab-ci.yml
perl -pi -e "s/^  ICINGA_FORCE_VERSION: .*/  ICINGA_FORCE_VERSION: v$VERSION/g" .gitlab-ci.yml
```


### Release Commit

Commit the changes and push the branch.

```bash
git commit -av -m "Release $VERSION-1"
git push origin 2.11
```

GitLab will now build snapshot packages based on the tag `v2.11.0` of Icinga 2.

### Package Tests

In order to test the created packages you can download a job's artifacts:

Visit [git.icinga.com](https://git.icinga.com/packaging/rpm-icinga2)
and navigate to the respective pipeline under `CI / CD -> Pipelines`.

There click on the job you want to download packages from.

The job's output appears. On the right-hand sidebar you can browse its artifacts.

Once there, navigate to `build/RPMS/noarch` where you'll find the packages.

### Release Packages

To build release packages and upload them to [packages.icinga.com](https://packages.icinga.com)
tag the release commit and push it.

RPM/DEB/Raspbian:

```bash
git tag -s $VERSION-1 -m "Release v$VERSION-1"
git push origin $VERSION-1
```

Windows:

```bash
git tag -s $VERSION -m "Release v$VERSION"
git push origin $VERSION
```


Now cherry pick the release commit to `master` so that the changes are transferred back to it.

**Attention**: Only the release commit. *NOT* the one switching the build type!


## Build Infrastructure <a id="build-infrastructure"></a>

https://git.icinga.com/packaging/rpm-icinga2/pipelines
https://git.icinga.com/packaging/deb-icinga2/pipelines
https://git.icinga.com/packaging/windows-icinga2/pipelines
https://git.icinga.com/packaging/raspbian-icinga2/pipelines

* Verify package build changes for this version.
* Test the snapshot packages for all distributions beforehand.

Once the release repository tags are pushed, release builds
are triggered and automatically published to packages.icinga.com

## Release Tests  <a id="release-tests"></a>

* Test DB IDO with MySQL and PostgreSQL.
* Provision the vagrant boxes and test the release packages.
* Test the [setup wizard](https://packages.icinga.com/windows/) inside a Windows VM.
* Start a new docker container and install/run icinga2.

### CentOS

```bash
docker run -ti centos:7 bash

yum -y install https://packages.icinga.com/epel/icinga-rpm-release-7-latest.noarch.rpm
yum -y install epel-release
yum -y install icinga2
icinga2 daemon -C
```

### Ubuntu

```bash
docker run -ti ubuntu:bionic bash

apt-get update
apt-get -y install apt-transport-https wget gnupg

wget -O - https://packages.icinga.com/icinga.key | apt-key add -

. /etc/os-release; if [ ! -z ${UBUNTU_CODENAME+x} ]; then DIST="${UBUNTU_CODENAME}"; else DIST="$(lsb_release -c| awk '{print $2}')"; fi; \
 echo "deb https://packages.icinga.com/ubuntu icinga-${DIST} main" > \
 /etc/apt/sources.list.d/${DIST}-icinga.list
 echo "deb-src https://packages.icinga.com/ubuntu icinga-${DIST} main" >> \
 /etc/apt/sources.list.d/${DIST}-icinga.list

apt-get update

apt-get -y install icinga2
icinga2 daemon -C
```


## GitHub Release  <a id="github-release"></a>

Create a new release for the newly created Git tag: https://github.com/Icinga/icinga2/releases

> Hint: Choose [tags](https://github.com/Icinga/icinga2/tags), pick one to edit and
> make this a release. You can also create a draft release.

The release body should contain a short changelog, with links
into the roadmap, changelog and blogpost.


## Docker  <a id="docker"></a>

> Only for final versions (not for RCs).

Once the release has been published on GitHub, wait for its
[GitHub actions](https://github.com/Icinga/icinga2/actions) to complete.

```bash
VERSION=2.12.1

TAGS=(2.12)
#TAGS=(2.12 2 latest)

docker pull icinga/icinga2:$VERSION

for t in "${TAGS[@]}"; do
  docker tag icinga/icinga2:$VERSION icinga/icinga2:$t
done

for t in "${TAGS[@]}"; do
  docker push icinga/icinga2:$t
done
```


## Post Release  <a id="post-release"></a>

### Online Documentation  <a id="online-documentation"></a>

> Only required for major releases.

Navigate to `puppet-customer/icinga.git` and do the following steps:

#### Testing

```bash
git checkout testing && git pull
vim files/var/www/docs/config/icinga2-latest.yml

git commit -av -m "icinga-web: Update docs for Icinga 2"

git push
```

SSH into the webserver and do a manual Puppet dry run with the testing environment.

```bash
puppet agent -t --environment testing --noop
```

Once succeeded, continue with production deployment.

#### Production

```bash
git checkout master && git pull
git merge testing
git push
```

SSH into the webserver and do a manual Puppet run from the production environment (default).

```bash
puppet agent -t
```

#### Manual Generation

SSH into the webserver or ask @bobapple.

```bash
cd /usr/local/icinga-docs-tools && ./build-docs.rb -c /var/www/docs/config/icinga2-latest.yml
```

### Announcement  <a id="announcement"></a>

* Create a new blog post on [icinga.com/blog](https://icinga.com/blog) including a featured image
* Create a release topic on [community.icinga.com](https://community.icinga.com)
* Release email to net-tech & team

### Project Management  <a id="project-management"></a>

* Add new minor version on [GitHub](https://github.com/Icinga/icinga2/milestones).
