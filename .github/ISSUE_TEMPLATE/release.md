---
name: '[INTERNAL] Release'
about: Release a version
title: 'Release Version v$version'
labels: ''
assignees: ''

---

# Release Workflow

- [ ] Check that the `.mailmap` and `AUTHORS` files are up to date
- [ ] Update `ICINGA2_VERSION`
- [ ] Update bundled Windows dependencies
- [ ] Update `CHANGELOG.md`
- [ ] Create and push a signed tag for the version
- [ ] Build and release DEB and RPM packages
- [ ] Build and release Windows packages
- [ ] Create release on GitHub
- [ ] Update public docs
- [ ] Announce release
