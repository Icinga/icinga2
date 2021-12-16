-- -----------------------------------------
-- upgrade path for Icinga 2.13.3
--
-- -----------------------------------------
-- Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- -------------
-- set dbversion
-- -------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.15.1', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.15.1', modify_time=NOW();
