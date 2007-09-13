--
-- nxMyDB - Table Schemas
--
-- Requires MySQL v5.0.3 or newer
--

CREATE TABLE io_group (
  name        varchar(65)  NOT NULL,
  description varchar(128) NOT NULL,
  slots       tinyblob     NOT NULL,
  users       int          NOT NULL default 0,
  vfsfile     varchar(260) NOT NULL,
  updated     int unsigned NOT NULL default 0,
  lockowner   varchar(36)           default NULL,
  locktime    int unsigned NOT NULL default 0,
  PRIMARY KEY (name)
);

CREATE TABLE io_user (
  name        varchar(65)  NOT NULL,
  description varchar(128) NOT NULL,
  flags       varchar(32)  NOT NULL,
  home        varchar(260) NOT NULL,
  limits      tinyblob     NOT NULL,
  password    tinyblob     NOT NULL,
  vfsfile     varchar(260) NOT NULL,
  credits     tinyblob     NOT NULL,
  ratio       tinyblob     NOT NULL,
  alldn       tinyblob     NOT NULL,
  allup       tinyblob     NOT NULL,
  daydn       tinyblob     NOT NULL,
  dayup       tinyblob     NOT NULL,
  monthdn     tinyblob     NOT NULL,
  monthup     tinyblob     NOT NULL,
  wkdn        tinyblob     NOT NULL,
  wkup        tinyblob     NOT NULL,
  updated     int unsigned NOT NULL default 0,
  lockowner   varchar(36)           default NULL,
  locktime    int unsigned NOT NULL default 0,
  PRIMARY KEY (name)
);

CREATE TABLE io_user_admins (
  uname       varchar(65) NOT NULL,
  gname       varchar(65) NOT NULL,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_user_groups (
  uname       varchar(65) NOT NULL,
  gname       varchar(65) NOT NULL,
  idx         tinyint     NOT NULL default 0,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_user_hosts (
  uname       varchar(65) NOT NULL,
  host        varchar(96) NOT NULL,
  PRIMARY KEY (uname,host)
);
