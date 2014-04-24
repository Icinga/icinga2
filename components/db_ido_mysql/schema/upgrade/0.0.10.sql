

ALTER TABLE icinga_hostgroups ADD COLUMN notes TEXT character set latin1  default NULL;
ALTER TABLE icinga_hostgroups ADD COLUMN notes_url TEXT character set latin1  default NULL;
ALTER TABLE icinga_hostgroups ADD COLUMN action_url TEXT character set latin1  default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN notes TEXT character set latin1  default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN notes_url TEXT character set latin1  default NULL;
ALTER TABLE icinga_servicegroups ADD COLUMN action_url TEXT character set latin1  default NULL;
