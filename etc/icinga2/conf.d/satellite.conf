/*
 * Host and Service templates for the Agent Setup.
 */


/**
 * Provides settings for satellite hosts managed by 'icinga2 repository'.
 * Define your global attributes here, for example custom
 * attributes used for notifications, etc.
 */
template Host "satellite-host" {
  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
  }
}

/**
 * Provides settings for satellite services managed by 'icinga2 repository'.
 * Define your global satellite attributes here, for example custom
 * attributes used for notifications, etc.
 */
template Service "satellite-service" {
  vars.notification["mail"] = {
    groups = [ "icingaadmins" ]
  }
}


apply Dependency "satellite-host" to Host {
  parent_host_name = host.zone

  assign where host.zone != "" && "satellite-host" in host.templates
}
