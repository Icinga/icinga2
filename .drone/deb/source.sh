#!/bin/bash
set -exo pipefail

. .drone/setup.sh

git clone https://git.icinga.com/packaging/deb-icinga2.git

cd deb-icinga2
ln -vs ../ccache .
icinga-build-deb-source
