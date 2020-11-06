#!/bin/bash
set -exo pipefail

. .drone/setup.sh
. .drone/ccache.sh

git clone https://git.icinga.com/packaging/raspbian-icinga2.git
cd raspbian-icinga2

ln -vs ../ccache .
ICINGA_BUILD_DEB_DEFAULT_ARCH=armhf icinga-build-package
ccache -s
