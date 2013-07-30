#!/bin/sh

foo=$(shuf -i 0-3 -n 1)
echo "flapme: $foo"
exit $foo
