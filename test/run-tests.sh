#!/bin/sh
exec ./icinga2_test --log_format=XML --log_sink=results.xml --log_level=all --report_level=no
