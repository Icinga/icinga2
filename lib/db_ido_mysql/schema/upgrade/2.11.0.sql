-- -----------------------------------------
-- upgrade path for Icinga 2.11.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2019 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- --------------------------------------------------------
-- Helper functions and procedures for DROP INDEX IF EXISTS
-- --------------------------------------------------------

DELIMITER //
DROP FUNCTION IF EXISTS ido_index_exists //
CREATE FUNCTION ido_index_exists(
  f_table_name varchar(64),
  f_index_name varchar(64)
)
  RETURNS BOOL
  DETERMINISTIC
  READS SQL DATA
  BEGIN
    DECLARE index_exists BOOL DEFAULT FALSE;
    SELECT EXISTS (
        SELECT 1
        FROM information_schema.statistics
        WHERE table_schema = SCHEMA()
              AND table_name = f_table_name
              AND index_name = f_index_name
    ) INTO index_exists;
    RETURN index_exists;
  END //

DROP PROCEDURE IF EXISTS ido_drop_index_if_exists //
CREATE PROCEDURE ido_drop_index_if_exists (
  IN p_table_name varchar(64),
  IN p_index_name varchar(64)
)
  DETERMINISTIC
  MODIFIES SQL DATA
  BEGIN
    IF ido_index_exists(p_table_name, p_index_name)
    THEN
      SET @ido_drop_index_sql = CONCAT('ALTER TABLE `', SCHEMA(), '`.`', p_table_name, '` DROP INDEX `', p_index_name, '`');
      PREPARE stmt FROM @ido_drop_index_sql;
      EXECUTE stmt;
      DEALLOCATE PREPARE stmt;
      SET @ido_drop_index_sql = NULL;
    END IF;
  END //
DELIMITER ;

CALL ido_drop_index_if_exists('icinga_commands', 'commands_i_id_idx');
CALL ido_drop_index_if_exists('icinga_comments', 'idx_comments_object_id');
CALL ido_drop_index_if_exists('icinga_comments', 'comments_i_id_idx');
CALL ido_drop_index_if_exists('icinga_configfiles', 'configfiles_i_id_idx');
CALL ido_drop_index_if_exists('icinga_contactgroups', 'contactgroups_i_id_idx');
CALL ido_drop_index_if_exists('icinga_contacts', 'contacts_i_id_idx');
CALL ido_drop_index_if_exists('icinga_customvariables', 'idx_customvariables_object_id');
CALL ido_drop_index_if_exists('icinga_eventhandlers', 'eventhandlers_i_id_idx');
CALL ido_drop_index_if_exists('icinga_hostdependencies', 'hostdependencies_i_id_idx');
CALL ido_drop_index_if_exists('icinga_hostescalations', 'hostesc_i_id_idx');
CALL ido_drop_index_if_exists('icinga_hostescalation_contacts', 'hostesc_contacts_i_id_idx');
CALL ido_drop_index_if_exists('icinga_hostgroups', 'hostgroups_i_id_idx');
CALL ido_drop_index_if_exists('icinga_hosts', 'host_object_id');
CALL ido_drop_index_if_exists('icinga_hosts', 'hosts_i_id_idx');
CALL ido_drop_index_if_exists('icinga_objects', 'objects_objtype_id_idx');
CALL ido_drop_index_if_exists('icinga_programstatus', 'programstatus_i_id_idx');
CALL ido_drop_index_if_exists('icinga_runtimevariables', 'runtimevariables_i_id_idx');
CALL ido_drop_index_if_exists('icinga_scheduleddowntime', 'scheduleddowntime_i_id_idx');
CALL ido_drop_index_if_exists('icinga_scheduleddowntime', 'idx_scheduleddowntime_object_id');
CALL ido_drop_index_if_exists('icinga_serviceescalations', 'serviceesc_i_id_idx');
CALL ido_drop_index_if_exists('icinga_serviceescalation_contacts', 'serviceesc_contacts_i_id_idx');
CALL ido_drop_index_if_exists('icinga_servicegroups', 'servicegroups_i_id_idx');
CALL ido_drop_index_if_exists('icinga_services', 'services_i_id_idx');
CALL ido_drop_index_if_exists('icinga_services', 'service_object_id');
CALL ido_drop_index_if_exists('icinga_systemcommands', 'systemcommands_i_id_idx');
CALL ido_drop_index_if_exists('icinga_timeperiods', 'timeperiods_i_id_idx');

DROP FUNCTION ido_index_exists;
DROP PROCEDURE ido_drop_index_if_exists;

-- -----------------------------------------
-- set dbversion (same as 2.11.0)
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.15.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.15.0', modify_time=NOW();
