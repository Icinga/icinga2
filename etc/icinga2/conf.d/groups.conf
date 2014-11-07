/**
 * Host group examples.
 */

object HostGroup "linux-servers" {
  display_name = "Linux Servers"

  assign where host.vars.os == "Linux"
}

object HostGroup "windows-servers" {
  display_name = "Windows Servers"

  assign where host.vars.os == "Windows"
}

/**
 * Service group examples.
 */

object ServiceGroup "ping" {
  display_name = "Ping Checks"

  assign where match("ping*", service.name)
}

object ServiceGroup "http" {
  display_name = "HTTP Checks"

  assign where match("http*", service.check_command)
}

object ServiceGroup "disk" {
  display_name = "Disk Checks"

  assign where match("disk*", service.check_command)
}
