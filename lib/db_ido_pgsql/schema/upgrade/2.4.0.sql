-- -----------------------------------------
-- upgrade path for Icinga 2.4.0
--
-- -----------------------------------------
-- Copyright (c) 2015 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #9027 Default timestamps lack time zone
-- -----------------------------------------

ALTER TABLE icinga_acknowledgements ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_acknowledgements ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_commenthistory ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_commenthistory ALTER COLUMN comment_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_commenthistory ALTER COLUMN expiration_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_commenthistory ALTER COLUMN deletion_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_comments ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_comments ALTER COLUMN comment_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_comments ALTER COLUMN expiration_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_conninfo ALTER COLUMN connect_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_conninfo ALTER COLUMN disconnect_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_conninfo ALTER COLUMN last_checkin_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_conninfo ALTER COLUMN data_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_conninfo ALTER COLUMN data_end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_contactnotificationmethods ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_contactnotificationmethods ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_contactnotifications ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_contactnotifications ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_contactstatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_contactstatus ALTER COLUMN last_host_notification SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_contactstatus ALTER COLUMN last_service_notification SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_customvariablestatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_dbversion ALTER COLUMN create_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_dbversion ALTER COLUMN modify_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_downtimehistory ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_downtimehistory ALTER COLUMN scheduled_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_downtimehistory ALTER COLUMN scheduled_end_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_downtimehistory ALTER COLUMN actual_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_downtimehistory ALTER COLUMN actual_end_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_downtimehistory ALTER COLUMN trigger_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_eventhandlers ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_eventhandlers ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_externalcommands ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_flappinghistory ALTER COLUMN event_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_flappinghistory ALTER COLUMN comment_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_hostchecks ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hostchecks ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_hoststatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_check SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN next_check SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_state_change SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_hard_state_change SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_time_up SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_time_down SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_time_unreachable SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN last_notification SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_hoststatus ALTER COLUMN next_notification SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_logentries ALTER COLUMN logentry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_logentries ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_notifications ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_notifications ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_processevents ALTER COLUMN event_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_programstatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_programstatus ALTER COLUMN program_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_programstatus ALTER COLUMN program_end_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_programstatus ALTER COLUMN last_command_check SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_programstatus ALTER COLUMN last_log_rotation SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_programstatus ALTER COLUMN disable_notif_expire_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_scheduleddowntime ALTER COLUMN entry_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN scheduled_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN scheduled_end_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN actual_start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_scheduleddowntime ALTER COLUMN trigger_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_servicechecks ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicechecks ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_servicestatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_check SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN next_check SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_state_change SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_hard_state_change SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_time_ok SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_time_warning SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_time_unknown SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_time_critical SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN last_notification SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_servicestatus ALTER COLUMN next_notification SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_statehistory ALTER COLUMN state_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_systemcommands ALTER COLUMN start_time SET DEFAULT '1970-01-01 00:00:00+00';
ALTER TABLE icinga_systemcommands ALTER COLUMN end_time SET DEFAULT '1970-01-01 00:00:00+00';

ALTER TABLE icinga_endpointstatus ALTER COLUMN status_update_time SET DEFAULT '1970-01-01 00:00:00+00';

-- -----------------------------------------
-- #9455 check_source data type
-- -----------------------------------------

ALTER TABLE icinga_statehistory ALTER COLUMN check_source TYPE TEXT;
ALTER TABLE icinga_statehistory ALTER COLUMN check_source SET default '';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.0');
