CREATE TABLE groups (
  Name        varchar(65)  NOT NULL,
  Description varchar(129) NOT NULL,
  SlotsRatio  int          NOT NULL default 0, -- Slots[0]
  SlotsLeech  int          NOT NULL default 0, -- Slots[1]
  Users       int          NOT NULL default 0,
  VfsFile     varchar(255) NOT NULL,
  PRIMARY KEY (Name)
);

CREATE TABLE users (
  Name        varchar(65)  NOT NULL,
  Tagline     varchar(129) NOT NULL,
  VfsFile     varchar(255) NOT NULL,
  Home        varchar(255) NOT NULL,
  Flags       varchar(33)  NOT NULL,
  Limits      tinyblob     NOT NULL,
  Password    tinyblob     NOT NULL,
  Ratio       tinyblob     NOT NULL,
  Credits     tinyblob     NOT NULL,
  DayUp       tinyblob     NOT NULL,
  DayDn       tinyblob     NOT NULL,
  WkUp        tinyblob     NOT NULL,
  WkDn        tinyblob     NOT NULL,
  MonthUp     tinyblob     NOT NULL,
  MonthDn     tinyblob     NOT NULL,
  AllUp       tinyblob     NOT NULL,
  AllDn       tinyblob     NOT NULL,
  PRIMARY KEY (Name)
);

CREATE TABLE users_admingroups (
  UserName    varchar(65) NOT NULL,
  GroupName   varchar(65) NOT NULL,
  PRIMARY KEY (UserName,GroupName)
);

CREATE TABLE users_groups (
  UserName    varchar(65) NOT NULL,
  GroupName   varchar(65) NOT NULL,
  PRIMARY KEY (UserName,GroupName)
);

CREATE TABLE users_hosts (
  UserName    varchar(65) NOT NULL,
  HostMask    varchar(97) NOT NULL,
  PRIMARY KEY (UserName,HostMask)
);
