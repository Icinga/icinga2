#!/bin/bash

set -e
set -o pipefail

if [ -z "$ICINGA2_HOSTS" ]; then
    ICINGA2_HOSTS=100
fi

cat <<<"const DummyHostsAmount = $ICINGA2_HOSTS" >/opt/icinga2/etc/icinga2/zones.d/master/01-dummys-amount.conf
