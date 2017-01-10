-- -----------------------------------------
-- upgrade path for Icinga 2.6.0
--
-- -----------------------------------------
-- Copyright (c) 2016 Icinga Development Team (https://www.icinga.com)
--
-- Please check http://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #10502 IDO: Support NO_ZERO_DATE and NO_ZERO_IN_DATE SQL modes
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_commenthistory
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN comment_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN expiration_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN deletion_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_comments
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN comment_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN expiration_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_conninfo
  MODIFY COLUMN connect_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN disconnect_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_checkin_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN data_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN data_end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_contactnotificationmethods
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_contactnotifications
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_contactstatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_host_notification timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_service_notification timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_customvariablestatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_dbversion
  MODIFY COLUMN create_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN modify_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_downtimehistory
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN scheduled_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN scheduled_end_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN actual_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN actual_end_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN trigger_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_eventhandlers
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_externalcommands
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_flappinghistory
  MODIFY COLUMN event_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN comment_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_hostchecks
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_hoststatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_check timestamp NULL DEFAULT NULL,
  MODIFY COLUMN next_check timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_state_change timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_hard_state_change timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_up timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_down timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_unreachable timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_notification timestamp NULL DEFAULT NULL,
  MODIFY COLUMN next_notification timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_logentries
  MODIFY COLUMN logentry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_notifications
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_processevents
  MODIFY COLUMN event_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_programstatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN program_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN program_end_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_command_check timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_log_rotation timestamp NULL DEFAULT NULL,
  MODIFY COLUMN disable_notif_expire_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_scheduleddowntime
  MODIFY COLUMN entry_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN scheduled_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN scheduled_end_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN actual_start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN trigger_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_servicechecks
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_servicestatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_check timestamp NULL DEFAULT NULL,
  MODIFY COLUMN next_check timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_state_change timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_hard_state_change timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_ok timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_warning timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_unknown timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_time_critical timestamp NULL DEFAULT NULL,
  MODIFY COLUMN last_notification timestamp NULL DEFAULT NULL,
  MODIFY COLUMN next_notification timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_statehistory
  MODIFY COLUMN state_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_systemcommands
  MODIFY COLUMN start_time timestamp NULL DEFAULT NULL,
  MODIFY COLUMN end_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_endpointstatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL;

ALTER TABLE icinga_zonestatus
  MODIFY COLUMN status_update_time timestamp NULL DEFAULT NULL;

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.2', NOW(), NOW())
ON DUPLICATE KEY UPDATE version='1.14.2', modify_time=NOW();
