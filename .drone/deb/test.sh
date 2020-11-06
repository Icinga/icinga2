#!/bin/bash
set -exo pipefail

. .drone/setup.sh

cd deb-icinga2
icinga-build-test
