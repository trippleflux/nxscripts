CREATE TABLE io_groups (
  name        varchar(65)  NOT NULL,
  description varchar(129) NOT NULL,
  slots       tinyblob     NOT NULL,
  users       int          NOT NULL default 0,
  vfsfile     varchar(255) NOT NULL,
  updated     int unsigned NOT NULL default 0, -- Time stamp of last update
  PRIMARY KEY (name)
);

CREATE TABLE io_users (
  name        varchar(65)  NOT NULL,
  description varchar(129) NOT NULL,
  flags       varchar(33)  NOT NULL,
  home        varchar(255) NOT NULL,
  limits      tinyblob     NOT NULL,
  password    tinyblob     NOT NULL,
  vfsfile     varchar(255) NOT NULL,
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
  updated     int unsigned NOT NULL default 0, -- Time stamp of last update
  PRIMARY KEY (name)
);

CREATE TABLE io_useradmins (
  uname       varchar(65) NOT NULL,
  gname       varchar(65) NOT NULL,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_usergroups (
  uname       varchar(65) NOT NULL,
  gname       varchar(65) NOT NULL,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_userhosts (
  name        varchar(65) NOT NULL,
  host        varchar(97) NOT NULL,
  PRIMARY KEY (name,host)
);
