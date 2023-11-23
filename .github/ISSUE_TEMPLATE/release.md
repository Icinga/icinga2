---
name: '[INTERNAL] Release'
about: Release a version
title: 'Release Version v$version'
labels: ''
assignees: ''

---

# Release Workflow

- [ ] Update `ICINGA2_VERSION`
- [ ] Update bundled Windows dependencies
- [ ] Harden global TLS defaults (consult https://ssl-config.mozilla.org)
- [ ] Update `CHANGELOG.md`
- [ ] Create and push a signed tag for the version
- [ ] Build and release DEB and RPM packages
- [ ] Build and release Windows packages
- [ ] Create release on GitHub
- [ ] Update public docs
- [ ] Announce release

## Update Bundled Windows Dependencies

### Update packages.icinga.com

Add the latest Boost and OpenSSL versions to
https://packages.icinga.com/windows/dependencies/, e.g.:

* https://master.dl.sourceforge.net/project/boost/boost-binaries/1.82.0/boost_1_82_0-msvc-14.2-64.exe
* https://master.dl.sourceforge.net/project/boost/boost-binaries/1.82.0/boost_1_82_0-msvc-14.2-32.exe
* https://slproweb.com/download/Win64OpenSSL-3_0_9.exe
* https://slproweb.com/download/Win32OpenSSL-3_0_9.exe

### Update Build Server, CI/CD and Documentation

* [doc/21-development.md](doc/21-development.md)
* [doc/win-dev.ps1](doc/win-dev.ps1) (also affects CI/CD)
* [tools/win32/configure.ps1](tools/win32/configure.ps1)
* [tools/win32/configure-dev.ps1](tools/win32/configure-dev.ps1)

### Re-provision Build Server

Even if there aren't any new releases of dependencies with versions
hardcoded in the repos and files listed above (Boost, OpenSSL).
There may be new build versions of other dependencies (VS, MSVC).
Our GitHub actions (tests) use the latest ones automatically,
but the GitLab runner (release packages) doesn't.
