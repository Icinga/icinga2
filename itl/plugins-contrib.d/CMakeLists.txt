# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

install(
  FILES big-data.conf databases.conf hardware.conf icingacli.conf ipmi.conf logmanagement.conf metrics.conf network-components.conf network-services.conf operating-system.conf raid-controller.conf smart-attributes.conf smart-advanced.conf storage.conf systemd.conf virtualization.conf vmware.conf web.conf
  DESTINATION ${ICINGA2_INCLUDEDIR}/plugins-contrib.d
)
