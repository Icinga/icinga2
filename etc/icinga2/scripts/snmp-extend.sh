#!/bin/sh
SNMPGET=$(which snmpget)
test -x $SNMPGET || exit 3

if [ -z "$3" ]; then
  echo "Syntax: $0 <host> <community> <plugin>"
  exit 3
fi

HOST=$1
COMMUNITY=$2
PLUGIN=$3

RESULT=$(snmpget -v2c -c $COMMUNITY -OQv $HOST .1.3.6.1.4.1.8072.1.3.2.3.1.2.\"$PLUGIN\" 2>&1 | sed -e 's/^"//'  -e 's/"$//')

if [ $? -ne 0 ]; then
  echo $RESULT
  exit 3
fi

STATUS=$(echo $RESULT | cut -f1 -d' ')
OUTPUT=$(echo $RESULT | cut -f2- -d' ')

echo $OUTPUT
exit $STATUS
