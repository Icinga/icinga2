

-- -----------------------------------------
-- #6094
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD COLUMN is_reachable smallint(6) DEFAULT NULL;
ALTER TABLE icinga_servicestatus ADD COLUMN is_reachable smallint(6) DEFAULT NULL;
