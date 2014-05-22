#!/bin/sh
#
# chkconfig: 35 90 12
# description: Icinga 2
#
### BEGIN INIT INFO
# Provides:          icinga2
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Should-Start:      mysql postgresql
# Should-Stop:       mysql postgresql
# Default-Start:     2 3 5
# Default-Stop:      0 1 6
# Short-Description: icinga2 host/service/network monitoring and management system
# Description:       Icinga 2 is a monitoring and management system for hosts, services and networks.
### END INIT INFO

DAEMON=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2
ICINGA2_CONFIG_FILE=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2/icinga2.conf
ICINGA2_STATE_DIR=@CMAKE_INSTALL_FULL_LOCALSTATEDIR@
ICINGA2_PID_FILE=$ICINGA2_STATE_DIR/run/icinga2/icinga2.pid
ICINGA2_ERROR_LOG=$ICINGA2_STATE_DIR/log/icinga2/error.log
ICINGA2_LOG=$ICINGA2_STATE_DIR/log/icinga2/icinga2.log
ICINGA2_USER=@ICINGA2_USER@
ICINGA2_GROUP=@ICINGA2_GROUP@
ICINGA2_COMMAND_USER=@ICINGA2_COMMAND_USER@
ICINGA2_COMMAND_GROUP=@ICINGA2_COMMAND_GROUP@

test -x $DAEMON || exit 0

if [ ! -e $ICINGA2_CONFIG_FILE ]; then
        echo "Config file '$ICINGA2_CONFIG_FILE' does not exist."
        exit 1
fi

# Get function from functions library
if [ -f /etc/rc.d/init.d/functions ]; then
        . /etc/rc.d/init.d/functions
elif [ -f /etc/init.d/functions ]; then
        . /etc/init.d/functions
fi

# Load extra environment variables
if [ -f /etc/sysconfig/icinga ]; then
        . /etc/sysconfig/icinga
fi
if [ -f /etc/default/icinga ]; then
        . /etc/default/icinga
fi

# Start Icinga 2
start() {
	mkdir -p $(dirname -- $ICINGA2_PID_FILE)
	chown $ICINGA2_USER:$ICINGA2_GROUP $(dirname -- $ICINGA2_PID_FILE)
	chown $ICINGA2_USER:$ICINGA2_GROUP $ICINGA2_PID_FILE

	mkdir -p $(dirname -- $ICINGA2_ERROR_LOG)
	chown $ICINGA2_USER:$ICINGA2_COMMAND_GROUP $(dirname -- $ICINGA2_ERROR_LOG)
	chmod 750 $(dirname -- $ICINGA2_ERROR_LOG)
	chown $ICINGA2_USER:$ICINGA2_COMMAND_GROUP $ICINGA2_ERROR_LOG $ICINGA2_LOG

	mkdir -p $ICINGA2_STATE_DIR/run/icinga2/cmd
	chown $ICINGA2_USER:$ICINGA2_COMMAND_GROUP $ICINGA2_STATE_DIR/run/icinga2/cmd
	chmod 2755 $ICINGA2_STATE_DIR/run/icinga2/cmd

	echo "Starting Icinga 2: "
	if ! $DAEMON -c $ICINGA2_CONFIG_FILE -d -e $ICINGA2_ERROR_LOG -u $ICINGA2_USER -g $ICINGA2_GROUP; then
		echo "Error starting Icinga."
		exit 1
	else
		echo "Done"
	fi
}

# Restart Icinga 2
stop() {
        printf "Stopping Icinga 2: "
        if [ ! -e $ICINGA2_PID_FILE ]; then
                echo "The PID file '$ICINGA2_PID_FILE' does not exist."
                if [ "x$1" = "xnofail" ]; then
			return
		else
			exit 1
		fi
        fi

	pid=`cat $ICINGA2_PID_FILE`
	
        if kill -INT $pid >/dev/null 2>&1; then
		for i in 1 2 3 4 5 6 7 8 9 10; do
			if ! kill -CHLD $pid >/dev/null 2>&1; then
				break
			fi
		
			printf '.'
			
			sleep 3
		done
	fi

        if kill -CHLD $pid >/dev/null 2>&1; then
                kill -KILL $pid
        fi

	echo "Done"
}

# Reload Icinga 2
reload() {
	printf "Reloading Icinga 2: "

	pid=`cat $ICINGA2_PID_FILE`
	if kill -HUP $pid >/dev/null 2>&1; then
		echo "Done"
	else
		echo "Error: Icinga not running"
		exit 3
	fi
}

# Check the Icinga 2 configuration
checkconfig() {
	printf "Checking configuration:"

        echo "Validating the configuration file:"
        if ! $DAEMON -c $ICINGA2_CONFIG_FILE -C -u $ICINGA2_USER -g $ICINGA2_GROUP; then
                if [ "x$1" = "x" ]; then
                        echo "Icinga 2 detected configuration errors."
                        exit 1
                else
                        echo "Not "$1"ing Icinga 2 due to configuration errors."
                        if [ "x$2" = "xfail" ]; then
                                exit 1
                        fi
                fi
        fi
}

# Print status for Icinga 2
status() {
	printf "Icinga 2 status: "

	pid=`cat $ICINGA2_PID_FILE`
	if kill -CHLD $pid >/dev/null 2>&1; then
		echo "Running"
	else
		echo "Not running"
		exit 3
	fi
}

### main logic ###
case "$1" in
  start)
	checkconfig start fail
        start
        ;;
  stop)
        stop
        ;;
  status)
        status
        ;;
  restart|condrestart)
	checkconfig restart fail
        stop nofail
        start
        ;;
  reload)
	reload
	;;
  checkconfig)
	checkconfig
	;;
  *)
        echo "Usage: $0 {start|stop|restart|reload|checkconfig|status}"
        exit 1
esac
exit 0
