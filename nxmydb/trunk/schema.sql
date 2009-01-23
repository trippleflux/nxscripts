--
-- nxMyDB - Table Schemas
--
-- Instructions:
--
--  1. Create a new database:
--     mysql -u root -p -h 192.168.1.1 -e "CREATE DATABASE ioftpd"
--
--  2. Create table schemas in the new database:
--     mysql -u root -p -h 192.168.1.1 -D ioftpd --delimiter=$ < schema.sql
--
--  3. Create a MySQL user to access the "ioftpd" database.
--

--
-- Procedures
--

CREATE PROCEDURE io_user_lock(IN pName VARCHAR(65), IN pExpire INT, IN pTimeout INT, IN pOwner VARCHAR(36))
BEGIN
  DECLARE elapsed FLOAT UNSIGNED DEFAULT 0;
  DECLARE sleep   FLOAT UNSIGNED DEFAULT 0.2;

  SPIN:BEGIN
    WHILE elapsed < pTimeout DO
      UPDATE io_user SET lockowner=pOwner, locktime=UNIX_TIMESTAMP()
        WHERE name=pName AND (lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > pExpire);

      IF ROW_COUNT() > 0 THEN
        LEAVE SPIN;
      END IF;

      SET elapsed = elapsed + sleep;
      DO SLEEP(sleep);
    END WHILE;
  END SPIN;
END;

CREATE PROCEDURE io_group_lock(IN pName VARCHAR(65), IN pExpire INT, IN pTimeout INT, IN pOwner VARCHAR(36))
BEGIN
  DECLARE elapsed FLOAT UNSIGNED DEFAULT 0;
  DECLARE sleep   FLOAT UNSIGNED DEFAULT 0.2;

  SPIN:BEGIN
    WHILE elapsed < pTimeout DO
      UPDATE io_group SET lockowner=pOwner, locktime=UNIX_TIMESTAMP()
        WHERE name=pName AND (lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > pExpire);

      IF ROW_COUNT() > 0 THEN
        LEAVE SPIN;
      END IF;

      SET elapsed = elapsed + sleep;
      DO SLEEP(sleep);
    END WHILE;
  END SPIN;
END;

--
-- Tables
--

CREATE TABLE io_group (
  name        VARCHAR(65)  NOT NULL,
  description VARCHAR(128) NOT NULL,
  slots       TINYBLOB     NOT NULL, -- 2 x INT = 8 bytes
  users       INT          NOT NULL,
  vfsfile     VARCHAR(260) NOT NULL,
  updated     INT UNSIGNED NOT NULL DEFAULT 0,
  lockowner   VARCHAR(36)           DEFAULT NULL,
  locktime    INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (name)
);

CREATE TABLE io_group_changes (
  id          BIGINT UNSIGNED  NOT NULL AUTO_INCREMENT,
  time        INT UNSIGNED     NOT NULL,
  type        TINYINT UNSIGNED NOT NULL,
  name        VARCHAR(65)      NOT NULL,
  info        VARCHAR(255)     DEFAULT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE io_user (
  name        VARCHAR(65)  NOT NULL,
  description VARCHAR(128) NOT NULL,
  flags       VARCHAR(32)  NOT NULL,
  home        VARCHAR(260) NOT NULL,
  limits      TINYBLOB     NOT NULL, --  5 x INT  = 20 bytes
  password    TINYBLOB     NOT NULL, -- 20 x BYTE = 20 bytes
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

CREATE TABLE io_user_changes (
  id          BIGINT UNSIGNED  NOT NULL AUTO_INCREMENT,
  time        INT UNSIGNED     NOT NULL,
  type        TINYINT UNSIGNED NOT NULL,
  name        VARCHAR(65)      NOT NULL,
  info        VARCHAR(255)     DEFAULT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE io_user_admins (
  uname       VARCHAR(65) NOT NULL,
  gname       VARCHAR(65) NOT NULL,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_user_groups (
  uname       VARCHAR(65) NOT NULL,
  gname       VARCHAR(65) NOT NULL,
  idx         TINYINT     NOT NULL DEFAULT 0,
  PRIMARY KEY (uname,gname)
);

CREATE TABLE io_user_hosts (
  uname       VARCHAR(65) NOT NULL,
  host        VARCHAR(97) NOT NULL,
  PRIMARY KEY (uname,host)
);
