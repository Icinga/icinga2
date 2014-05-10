

-- -----------------------------------------
-- #6094
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD COLUMN is_reachable INTEGER  default 0;
ALTER TABLE icinga_servicestatus ADD COLUMN is_reachable INTEGER  default 0;

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.3');
