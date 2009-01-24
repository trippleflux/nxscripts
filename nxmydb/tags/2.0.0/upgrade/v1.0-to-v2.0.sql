--
-- nxMyDB - Upgrade from v1.0 to v2.0
--
-- Instructions:
--
--  1. Back up "ioftpd" database!!!
--
--  2. Run the upgrade script:
--     mysql -u root -p -h 192.168.1.1 -D ioftpd --delimiter=$ < v1.0-to-v2.0.sql
--

--
-- Rename existing tables
--

RENAME TABLE io_user TO io_user_v1;

RENAME TABLE io_user_hosts TO io_user_hosts_v1;

--
-- Create new tables
--

CREATE TABLE io_user (
  name        VARCHAR(65)  NOT NULL,
  description VARCHAR(128) NOT NULL,
  flags       VARCHAR(32)  NOT NULL,
  home        VARCHAR(260) NOT NULL,
  limits      TINYBLOB     NOT NULL, --  5 x INT  = 20 bytes
  password    TINYBLOB     NOT NULL, -- 20 x CHAR = 20 bytes
  vfsfile     VARCHAR(260) NOT NULL,
  credits     TINYBLOB     NOT NULL, -- 25 x INT64 = 200 bytes
  ratio       TINYBLOB     NOT NULL, -- 25 x INT   = 100 bytes
  alldn       BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  allup       BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  daydn       BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  dayup       BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  monthdn     BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  monthup     BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  wkdn        BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  wkup        BLOB         NOT NULL, -- 3 x 25 x INT64 = 600 bytes
  creator     VARCHAR(65)  NOT NULL,
  createdon   BIGINT       NOT NULL,
  logoncount  INT          NOT NULL,
  logonlast   BIGINT       NOT NULL,
  logonhost   VARCHAR(97)  NOT NULL,
  maxups      INT          NOT NULL,
  maxdowns    INT          NOT NULL,
  maxlogins   INT          NOT NULL,
  expiresat   BIGINT       NOT NULL,
  deletedon   BIGINT       NOT NULL,
  deletedby   VARCHAR(65)  NOT NULL,
  deletedmsg  VARCHAR(65)  NOT NULL,
  theme       INT          NOT NULL,
  opaque      VARCHAR(257) NOT NULL,
  updated     INT UNSIGNED NOT NULL DEFAULT 0,
  lockowner   VARCHAR(36)           DEFAULT NULL,
  locktime    INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (name)
);

CREATE TABLE io_user_hosts (
  uname       VARCHAR(65) NOT NULL,
  host        VARCHAR(97) NOT NULL,
  PRIMARY KEY (uname,host)
);

--
-- Import existing data
--

INSERT INTO io_user (name,description,flags,home,limits,password,vfsfile,credits,
                    ratio,alldn,allup,daydn,dayup,monthdn,monthup,wkdn,wkup,
                    creator,createdon,logoncount,logonlast,logonhost,maxups,maxdowns,
                    maxlogins,expiresat,deletedon,deletedby,deletedmsg,theme,opaque)
  SELECT io_user_v1.name,           -- name
         io_user_v1.description,    -- description
         io_user_v1.flags,          -- flags
         io_user_v1.home,           -- home
         io_user_v1.limits,         -- limits
         io_user_v1.password,       -- password
         io_user_v1.vfsfile,        -- vfsfile
         io_user_v1.ratio,          -- credits = v1.ratio
         io_user_v1.credits,        -- ratio   = v1.credits
         io_user_v1.dayup,          -- alldn   = v1.dayup
         io_user_v1.daydn,          -- allup   = v1.daydn
         io_user_v1.wkup,           -- daydn   = v1.wkup
         io_user_v1.wkdn,           -- dayup   = v1.wkdn
         io_user_v1.monthup,        -- monthdn = v1.monthup
         io_user_v1.monthdn,        -- monthup = v1.monthdn
         io_user_v1.allup,          -- wkdn    = v1.allup
         io_user_v1.alldn,          -- wkup    = v1.alldn
         '',                        -- creator
         0,                         -- createdon
         0,                         -- logoncount
         0,                         -- logonlast
         '',                        -- logonhost
         0,                         -- maxups
         0,                         -- maxdowns
         0,                         -- maxlogins
         0,                         -- expiresat
         0,                         -- deletedon
         '',                        -- deletedby
         '',                        -- deletedmsg
         0,                         -- theme
         ''                         -- opaque
  FROM   io_user_v1;

INSERT INTO io_user_hosts (uname,host)
  SELECT io_user_hosts_v1.uname,    -- uname
         io_user_hosts_v1.host      -- host
  FROM   io_user_hosts_v1;

--
-- Drop old tables
--

DROP TABLE io_user_v1;

DROP TABLE io_user_hosts_v1;
