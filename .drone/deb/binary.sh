#!/bin/bash
set -exo pipefail

. .drone/setup.sh
. .drone/ccache.sh

cd deb-icinga2
icinga-build-deb-binary
ccache -s
