-- -----------------------------------------
-- upgrade path for Icinga 2.13.0
--
-- -----------------------------------------
-- Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- ----------------------------------------
-- #7472 Support hosts with >128 characters
-- ----------------------------------------

ALTER TABLE icinga_objects
  MODIFY COLUMN name1 varchar(255) character set latin1 collate latin1_general_cs default '',
  MODIFY COLUMN name2 varchar(255) character set latin1 collate latin1_general_cs default NULL;

-- -------------
-- set dbversion
-- -------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.15.1', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.15.1', modify_time=NOW();
