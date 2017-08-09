-- -----------------------------------------
-- upgrade path for Icinga 2.8.0
--
-- -----------------------------------------
-- Copyright (c) 2017 Icinga Development Team (https://www.icinga.com)
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- -----------------------------------------
-- #5458 IDO: Improve downtime removal/cancel
-- -----------------------------------------

CREATE INDEX idx_downtimehistory_remove ON icinga_downtimehistory (object_id, entry_time, scheduled_start_time, scheduled_end_time);
CREATE INDEX idx_scheduleddowntime_remove ON icinga_scheduleddowntime (object_id, entry_time, scheduled_start_time, scheduled_end_time);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.3', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.14.3', modify_time=NOW();
