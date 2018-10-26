-- -----------------------------------------
-- upgrade path for Icinga 2.11.0
--
-- -----------------------------------------
-- Copyright (c) 2018 Icinga Development Team (https://icinga.com/)
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE `icinga_commands` DROP INDEX `commands_i_id_idx`;
ALTER TABLE `icinga_comments` DROP INDEX `idx_comments_object_id`;
ALTER TABLE `icinga_comments` DROP INDEX `comments_i_id_idx`;
ALTER TABLE `icinga_configfiles` DROP INDEX `configfiles_i_id_idx`;
ALTER TABLE `icinga_contactgroups` DROP INDEX `contactgroups_i_id_idx`;
ALTER TABLE `icinga_contacts` DROP INDEX `contacts_i_id_idx`;
ALTER TABLE `icinga_customvariables` DROP INDEX `idx_customvariables_object_id`;
ALTER TABLE `icinga_eventhandlers` DROP INDEX `eventhandlers_i_id_idx`;
ALTER TABLE `icinga_hostdependencies` DROP INDEX `hostdependencies_i_id_idx`;
ALTER TABLE `icinga_hostescalations` DROP INDEX `hostesc_i_id_idx`;
ALTER TABLE `icinga_hostescalation_contacts` DROP INDEX `hostesc_contacts_i_id_idx`;
ALTER TABLE `icinga_hostgroups` DROP INDEX `hostgroups_i_id_idx`;
ALTER TABLE `icinga_hosts` DROP INDEX `host_object_id`;
ALTER TABLE `icinga_hosts` DROP INDEX `hosts_i_id_idx`;
ALTER TABLE `icinga_objects` DROP INDEX `objects_objtype_id_idx`;
ALTER TABLE `icinga_programstatus` DROP INDEX `programstatus_i_id_idx`;
ALTER TABLE `icinga_runtimevariables` DROP INDEX `runtimevariables_i_id_idx`;
ALTER TABLE `icinga_scheduleddowntime` DROP INDEX `scheduleddowntime_i_id_idx`;
ALTER TABLE `icinga_scheduleddowntime` DROP INDEX `idx_scheduleddowntime_object_id`;
ALTER TABLE `icinga_serviceescalations` DROP INDEX `serviceesc_i_id_idx`;
ALTER TABLE `icinga_serviceescalation_contacts` DROP INDEX `serviceesc_contacts_i_id_idx`;
ALTER TABLE `icinga_servicegroups` DROP INDEX `servicegroups_i_id_idx`;
ALTER TABLE `icinga_services` DROP INDEX `services_i_id_idx`;
ALTER TABLE `icinga_services` DROP INDEX `service_object_id`;
ALTER TABLE `icinga_systemcommands` DROP INDEX `systemcommands_i_id_idx`;
ALTER TABLE `icinga_timeperiods` DROP INDEX `timeperiods_i_id_idx`;

-- -----------------------------------------
-- set dbversion (same as 2.11.0)
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.15.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.15.0', modify_time=NOW();
