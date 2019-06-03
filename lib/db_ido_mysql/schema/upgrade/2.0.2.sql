-- -----------------------------------------
-- upgrade path for Icinga 2.0.2
--
-- -----------------------------------------
-- Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

UPDATE icinga_objects SET name2 = NULL WHERE name2 = '';

ALTER TABLE `icinga_customvariables` MODIFY COLUMN `varname` varchar(255) character set latin1 collate latin1_general_cs default NULL;
ALTER TABLE `icinga_customvariablestatus` MODIFY COLUMN `varname` varchar(255) character set latin1 collate latin1_general_cs default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.6', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.6', modify_time=NOW();

