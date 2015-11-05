#!/bin/sh
ICINGA2_BIN=@CMAKE_INSTALL_FULL_LIBDIR@/icinga2/sbin/icinga2

if test "x`uname -s`" = "xLinux" -a "x$1" = "xconsole"; then
  libedit_line=`ldd $ICINGA2_BIN 2>&1 | grep libedit`
  if test $? -eq 0; then
    libedit_path=`echo $libedit_line | cut -f3 -d' '`
    if test -n "$libedit_path"; then
      libdir=`dirname -- $libedit_path`
      found=0
      for libreadline_path in `ls -1r -- $libdir/libreadline.so.* 2>/dev/null`; do
        found=1
        break
      done
      if test $found -eq 0; then
        libdir2=/`echo $libdir | cut -f3- -d/`
        for libreadline_path in `ls -1r -- $libdir2/libreadline.so.* 2>/dev/null`; do
          found=1
          break
        done
      fi
      if test $found -gt 0; then
        export LD_PRELOAD="$libreadline_path:$LD_PRELOAD"
      fi
    fi
  fi
fi

exec $ICINGA2_BIN "$@"
