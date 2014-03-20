
ALTER TABLE icinga_servicestatus ADD COLUMN check_source_object_id bigint default NULL;
ALTER TABLE icinga_hoststatus ADD COLUMN check_source_object_id bigint default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN check_source_object_id bigint default NULL;

