#!/bin/bash
set -exo pipefail

. .drone/vars.sh

cd "$(mktemp -d)"
git clone https://git.icinga.com/packaging/rpm-icinga2.git
cd rpm-icinga2

icinga-build-package
