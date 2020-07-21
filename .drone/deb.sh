#!/bin/bash
set -exo pipefail

. .drone/vars.sh

cd "$(mktemp -d)"
git clone https://git.icinga.com/packaging/deb-icinga2.git
cd deb-icinga2

icinga-build-deb-source
icinga-build-deb-binary
