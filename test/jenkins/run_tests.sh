#!/bin/sh

vagrant ssh-config > ssh_config
./run_tests.py $@ *.test
rm -f ssh_config
