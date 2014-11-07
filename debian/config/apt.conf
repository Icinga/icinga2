apply Service "apt" {
  import "generic-service"

  check_command = "apt"

  assign where host.name == NodeName
}
