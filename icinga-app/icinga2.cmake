#!/bin/sh
ICINGA2_BIN=@CMAKE_INSTALL_FULL_LIBDIR@/icinga2/sbin/icinga2

exec $ICINGA2_BIN "$@"
