-- -----------------------------------------
-- upgrade path for Icinga 2.8.1 (fix for fresh 2.8.0 installation only)
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
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

CALL ido_drop_index_if_exists('icinga_downtimehistory', 'instance_id');
CALL ido_drop_index_if_exists('icinga_scheduleddowntime', 'instance_id');
CALL ido_drop_index_if_exists('icinga_commenthistory', 'instance_id');
CALL ido_drop_index_if_exists('icinga_comments', 'instance_id');

DROP FUNCTION ido_index_exists;
DROP PROCEDURE ido_drop_index_if_exists;

-- -----------------------------------------
-- set dbversion (same as 2.8.0)
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.3', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.14.3', modify_time=NOW();
