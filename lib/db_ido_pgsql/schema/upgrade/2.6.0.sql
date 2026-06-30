-- -----------------------------------------
-- upgrade path for Icinga 2.6.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #13221 IDO: PostgreSQL: Don't use timestamp with timezone for unix timestamp columns
-- -----------------------------------------

DROP FUNCTION IF EXISTS from_unixtime(bigint);
CREATE FUNCTION from_unixtime(bigint) RETURNS timestamp AS $$
  SELECT to_timestamp($1) AT TIME ZONE 'UTC' AS result
$$ LANGUAGE sql;

DROP FUNCTION IF EXISTS unix_timestamp(timestamp WITH TIME ZONE);
CREATE OR REPLACE FUNCTION unix_timestamp(timestamp) RETURNS bigint AS '
  SELECT CAST(EXTRACT(EPOCH FROM $1) AS bigint) AS result;
' LANGUAGE sql;

ALTER TABLE icinga_acknowledgements
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_commenthistory
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp,
  ALTER COLUMN comment_time DROP DEFAULT, ALTER COLUMN comment_time TYPE timestamp,
  ALTER COLUMN expiration_time DROP DEFAULT, ALTER COLUMN expiration_time TYPE timestamp,
  ALTER COLUMN deletion_time DROP DEFAULT, ALTER COLUMN deletion_time TYPE timestamp;

ALTER TABLE icinga_comments
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp,
  ALTER COLUMN comment_time DROP DEFAULT, ALTER COLUMN comment_time TYPE timestamp,
  ALTER COLUMN expiration_time DROP DEFAULT, ALTER COLUMN expiration_time TYPE timestamp;

ALTER TABLE icinga_conninfo
  ALTER COLUMN connect_time DROP DEFAULT, ALTER COLUMN connect_time TYPE timestamp,
  ALTER COLUMN disconnect_time DROP DEFAULT, ALTER COLUMN disconnect_time TYPE timestamp,
  ALTER COLUMN last_checkin_time DROP DEFAULT, ALTER COLUMN last_checkin_time TYPE timestamp,
  ALTER COLUMN data_start_time DROP DEFAULT, ALTER COLUMN data_start_time TYPE timestamp,
  ALTER COLUMN data_end_time DROP DEFAULT, ALTER COLUMN data_end_time TYPE timestamp;

ALTER TABLE icinga_contactnotificationmethods
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_contactnotifications
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_contactstatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp,
  ALTER COLUMN last_host_notification DROP DEFAULT, ALTER COLUMN last_host_notification TYPE timestamp,
  ALTER COLUMN last_service_notification DROP DEFAULT, ALTER COLUMN last_service_notification TYPE timestamp;

ALTER TABLE icinga_customvariablestatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp;

ALTER TABLE icinga_dbversion
  ALTER COLUMN create_time DROP DEFAULT, ALTER COLUMN create_time TYPE timestamp,
  ALTER COLUMN modify_time DROP DEFAULT, ALTER COLUMN modify_time TYPE timestamp;

ALTER TABLE icinga_downtimehistory
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp,
  ALTER COLUMN scheduled_start_time DROP DEFAULT, ALTER COLUMN scheduled_start_time TYPE timestamp,
  ALTER COLUMN scheduled_end_time DROP DEFAULT, ALTER COLUMN scheduled_end_time TYPE timestamp,
  ALTER COLUMN actual_start_time DROP DEFAULT, ALTER COLUMN actual_start_time TYPE timestamp,
  ALTER COLUMN actual_end_time DROP DEFAULT, ALTER COLUMN actual_end_time TYPE timestamp,
  ALTER COLUMN trigger_time DROP DEFAULT, ALTER COLUMN trigger_time TYPE timestamp;

ALTER TABLE icinga_eventhandlers
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_externalcommands
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp;

ALTER TABLE icinga_flappinghistory
  ALTER COLUMN event_time DROP DEFAULT, ALTER COLUMN event_time TYPE timestamp,
  ALTER COLUMN comment_time DROP DEFAULT, ALTER COLUMN comment_time TYPE timestamp;

ALTER TABLE icinga_hostchecks
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_hoststatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp,
  ALTER COLUMN last_check DROP DEFAULT, ALTER COLUMN last_check TYPE timestamp,
  ALTER COLUMN next_check DROP DEFAULT, ALTER COLUMN next_check TYPE timestamp,
  ALTER COLUMN last_state_change DROP DEFAULT, ALTER COLUMN last_state_change TYPE timestamp,
  ALTER COLUMN last_hard_state_change DROP DEFAULT, ALTER COLUMN last_hard_state_change TYPE timestamp,
  ALTER COLUMN last_time_up DROP DEFAULT, ALTER COLUMN last_time_up TYPE timestamp,
  ALTER COLUMN last_time_down DROP DEFAULT, ALTER COLUMN last_time_down TYPE timestamp,
  ALTER COLUMN last_time_unreachable DROP DEFAULT, ALTER COLUMN last_time_unreachable TYPE timestamp,
  ALTER COLUMN last_notification DROP DEFAULT, ALTER COLUMN last_notification TYPE timestamp,
  ALTER COLUMN next_notification DROP DEFAULT, ALTER COLUMN next_notification TYPE timestamp;

ALTER TABLE icinga_logentries
  ALTER COLUMN logentry_time DROP DEFAULT, ALTER COLUMN logentry_time TYPE timestamp,
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp;

ALTER TABLE icinga_notifications
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_processevents
  ALTER COLUMN event_time DROP DEFAULT, ALTER COLUMN event_time TYPE timestamp;

ALTER TABLE icinga_programstatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp,
  ALTER COLUMN program_start_time DROP DEFAULT, ALTER COLUMN program_start_time TYPE timestamp,
  ALTER COLUMN program_end_time DROP DEFAULT, ALTER COLUMN program_end_time TYPE timestamp,
  ALTER COLUMN last_command_check DROP DEFAULT, ALTER COLUMN last_command_check TYPE timestamp,
  ALTER COLUMN last_log_rotation DROP DEFAULT, ALTER COLUMN last_log_rotation TYPE timestamp,
  ALTER COLUMN disable_notif_expire_time DROP DEFAULT, ALTER COLUMN disable_notif_expire_time TYPE timestamp;

ALTER TABLE icinga_scheduleddowntime
  ALTER COLUMN entry_time DROP DEFAULT, ALTER COLUMN entry_time TYPE timestamp,
  ALTER COLUMN scheduled_start_time DROP DEFAULT, ALTER COLUMN scheduled_start_time TYPE timestamp,
  ALTER COLUMN scheduled_end_time DROP DEFAULT, ALTER COLUMN scheduled_end_time TYPE timestamp,
  ALTER COLUMN actual_start_time DROP DEFAULT, ALTER COLUMN actual_start_time TYPE timestamp,
  ALTER COLUMN trigger_time DROP DEFAULT, ALTER COLUMN trigger_time TYPE timestamp;

ALTER TABLE icinga_servicechecks
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_servicestatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp,
  ALTER COLUMN last_check DROP DEFAULT, ALTER COLUMN last_check TYPE timestamp,
  ALTER COLUMN next_check DROP DEFAULT, ALTER COLUMN next_check TYPE timestamp,
  ALTER COLUMN last_state_change DROP DEFAULT, ALTER COLUMN last_state_change TYPE timestamp,
  ALTER COLUMN last_hard_state_change DROP DEFAULT, ALTER COLUMN last_hard_state_change TYPE timestamp,
  ALTER COLUMN last_time_ok DROP DEFAULT, ALTER COLUMN last_time_ok TYPE timestamp,
  ALTER COLUMN last_time_warning DROP DEFAULT, ALTER COLUMN last_time_warning TYPE timestamp,
  ALTER COLUMN last_time_unknown DROP DEFAULT, ALTER COLUMN last_time_unknown TYPE timestamp,
  ALTER COLUMN last_time_critical DROP DEFAULT, ALTER COLUMN last_time_critical TYPE timestamp,
  ALTER COLUMN last_notification DROP DEFAULT, ALTER COLUMN last_notification TYPE timestamp,
  ALTER COLUMN next_notification DROP DEFAULT, ALTER COLUMN next_notification TYPE timestamp;

ALTER TABLE icinga_statehistory
  ALTER COLUMN state_time DROP DEFAULT, ALTER COLUMN state_time TYPE timestamp;

ALTER TABLE icinga_systemcommands
  ALTER COLUMN start_time DROP DEFAULT, ALTER COLUMN start_time TYPE timestamp,
  ALTER COLUMN end_time DROP DEFAULT, ALTER COLUMN end_time TYPE timestamp;

ALTER TABLE icinga_endpointstatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp;

ALTER TABLE icinga_zonestatus
  ALTER COLUMN status_update_time DROP DEFAULT, ALTER COLUMN status_update_time TYPE timestamp;

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.2');
