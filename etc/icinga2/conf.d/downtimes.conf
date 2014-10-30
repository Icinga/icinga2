/**
 * The example downtime apply rule.
 */

apply ScheduledDowntime "backup-downtime" to Service {
  author = "icingaadmin"
  comment = "Scheduled downtime for backup"

  ranges = {
    monday = service.vars.backup_downtime
    tuesday = service.vars.backup_downtime
    wednesday = service.vars.backup_downtime
    thursday = service.vars.backup_downtime
    friday = service.vars.backup_downtime
    saturday = service.vars.backup_downtime
    sunday = service.vars.backup_downtime
  }

  assign where service.vars.backup_downtime != ""
}
