-- -----------------------------------------
-- upgrade path for Icinga 2.8.1 (fix for fresh 2.8.0 installation only)
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
-- set dbversion (same as 2.8.0)
-- -----------------------------------------

SELECT updatedbversion('1.14.3');
