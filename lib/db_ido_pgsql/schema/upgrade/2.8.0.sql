-- -----------------------------------------
-- upgrade path for Icinga 2.8.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE icinga_downtimehistory DROP CONSTRAINT IF EXISTS UQ_downtimehistory;
ALTER TABLE icinga_scheduleddowntime DROP CONSTRAINT IF EXISTS UQ_scheduleddowntime;
ALTER TABLE icinga_commenthistory DROP CONSTRAINT IF EXISTS UQ_commenthistory;
ALTER TABLE icinga_comments DROP CONSTRAINT IF EXISTS UQ_comments;

-- -----------------------------------------
-- #5458 IDO: Improve downtime removal/cancel
-- -----------------------------------------

CREATE INDEX idx_downtimehistory_remove ON icinga_downtimehistory (object_id, entry_time, scheduled_start_time, scheduled_end_time);
CREATE INDEX idx_scheduleddowntime_remove ON icinga_scheduleddowntime (object_id, entry_time, scheduled_start_time, scheduled_end_time);

-- -----------------------------------------
-- #5492
-- -----------------------------------------

CREATE INDEX idx_commenthistory_remove ON icinga_commenthistory (object_id, entry_time);
CREATE INDEX idx_comments_remove ON icinga_comments (object_id, entry_time);
-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.3');
