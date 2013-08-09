#!/bin/sh

echoerr() { echo "$@" 1>&2; }

foo=$(shuf -i 0-3 -n 1)
echo "flapme: $foo"
echoerr "stderr test"
exit $foo
