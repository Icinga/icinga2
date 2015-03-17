#!/bin/sh
ICINGA2_BIN=@CMAKE_INSTALL_FULL_LIBDIR@/icinga2/sbin/icinga2

if test "x`uname -s`" = "xLinux"; then
  libedit_line=`ldd $ICINGA2_BIN 2>&1 | grep libedit`
  if test $? -eq 0; then
    libedit_path=`echo $libedit_line | cut -f3 -d' '`
    if test -n "$libedit_path"; then
      libdir=`dirname -- $libedit_path`
      for libreadline_path in $libdir/libreadline.so*; do
        break
      done
      if test -n "$libreadline_path"; then
        export LD_PRELOAD="$libreadline_path:$LD_PRELOAD"
      fi
    fi
  fi
fi

exec $ICINGA2_BIN $@
