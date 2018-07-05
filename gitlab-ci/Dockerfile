FROM centos:7
RUN yum install -y epel-release
RUN yum install -y bash boost148-program-options boost148-regex boost148-system boost148-thread coreutils glibc libedit libgcc libstdc++ ncurses-libs openssl-libs shadow-utils yajl
RUN yum install -y fortune-mod mailx nagios-plugins-all supervisor which
COPY destdir /opt/icinga2
COPY gitlab-ci/mk_i2config.sh /mk_i2config.sh
COPY gitlab-ci/icinga2/*dummys*.conf /opt/icinga2/etc/icinga2/zones.d/master/
COPY gitlab-ci/supervisord.conf /etc/supervisor/conf.d/
RUN chown -R nagios:nagios /opt/icinga2
RUN /opt/icinga2/lib/icinga2/prepare-dirs /opt/icinga2/etc/sysconfig/icinga2
RUN /opt/icinga2/sbin/icinga2 node setup --master --cn docker-master
RUN perl -pi -e 's~".*?"~"icinga"~ if m~^\s*password\b~' /opt/icinga2/etc/icinga2/conf.d/api-users.conf
RUN /opt/icinga2/sbin/icinga2 feature enable redis
CMD /usr/bin/supervisord -n -c /etc/supervisor/conf.d/supervisord.conf
