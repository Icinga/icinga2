#!/bin/bash
set -exo pipefail

. .drone/setup.sh

cd rpm-icinga2
icinga-build-test
