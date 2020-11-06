#!/bin/bash
set -exo pipefail

. .drone/setup.sh

cd raspbian-icinga2
icinga-build-test
