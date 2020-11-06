#!/bin/bash
set -exo pipefail

. .drone/setup.sh
. .drone/ccache.sh

git clone https://git.icinga.com/packaging/rpm-icinga2.git
cd rpm-icinga2

ln -vs ../ccache .
icinga-build-package
ccache -s
