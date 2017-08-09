-- -----------------------------------------
-- upgrade path for Icinga 2.8.0
--
-- -----------------------------------------
-- Copyright (c) 2017 Icinga Development Team (https://www.icinga.com)
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #5458 IDO: Improve downtime removal/cancel
-- -----------------------------------------

CREATE INDEX idx_downtimehistory_remove ON icinga_downtimehistory (object_id, entry_time, scheduled_start_time, scheduled_end_time);
CREATE INDEX idx_scheduleddowntime_remove ON icinga_scheduleddowntime (object_id, entry_time, scheduled_start_time, scheduled_end_time);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.3');
