

ALTER TABLE icinga_hostgroups ADD COLUMN notes TEXT default NULL;
ALTER TABLE icinga_hostgroups ADD COLUMN notes_url TEXT default NULL;
ALTER TABLE icinga_hostgroups ADD COLUMN action_url TEXT default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN notes TEXT default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN notes_url TEXT default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN action_url TEXT default NULL;
