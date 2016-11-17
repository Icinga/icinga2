-- -----------------------------------------
-- upgrade path for Icinga 2.6.0
--
-- -----------------------------------------
-- Copyright (c) 2016 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.2', NOW(), NOW())
ON DUPLICATE KEY UPDATE version='1.14.2', modify_time=NOW();
