-- -----------------------------------------
-- upgrade path for Icinga 2.3.0
--
-- -----------------------------------------
-- Copyright (c) 2015 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.14.0', modify_time=NOW();
