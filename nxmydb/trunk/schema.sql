--
-- nxMyDB - Table Schemas
--
-- Requires MySQL v5.0.19 or newer
--

CREATE PROCEDURE io_user_lock(IN pName VARCHAR(65), IN pExpire INT, IN pTimeout INT, IN pOwner VARCHAR(36))
BEGIN
proc:BEGIN
  DECLARE elapsed FLOAT UNSIGNED DEFAULT 0;
  DECLARE sleep   FLOAT UNSIGNED DEFAULT 0.2;

  WHILE elapsed < pTimeout DO
    UPDATE io_user SET lockowner=pOwner, locktime=UNIX_TIMESTAMP()
      WHERE name=pName AND (lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > pExpire);

    IF ROW_COUNT() > 0 THEN
      LEAVE proc;
    END IF;

    SET elapsed = elapsed + sleep;
    DO SLEEP(sleep);
  END WHILE;
END;
END;

CREATE PROCEDURE io_group_lock(IN pName VARCHAR(65), IN pExpire INT, IN pTimeout INT, IN pOwner VARCHAR(36))
BEGIN
proc:BEGIN
  DECLARE elapsed FLOAT UNSIGNED DEFAULT 0;
  DECLARE sleep   FLOAT UNSIGNED DEFAULT 0.2;

  WHILE elapsed < pTimeout DO
    UPDATE io_group SET lockowner=pOwner, locktime=UNIX_TIMESTAMP()
      WHERE name=pName AND (lockowner IS NULL OR (UNIX_TIMESTAMP() - locktime) > pExpire);

    IF ROW_COUNT() > 0 THEN
      LEAVE proc;
    END IF;

    SET elapsed = elapsed + sleep;
    DO SLEEP(sleep);
  END WHILE;
END;
END;

CREATE TABLE io_group (
  name        VARCHAR(65)  NOT NULL,
  description VARCHAR(128) NOT NULL,
  slots       TINYBLOB     NOT NULL,
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
  limits      TINYBLOB     NOT NULL,
  password    TINYBLOB     NOT NULL,
  vfsfile     VARCHAR(260) NOT NULL,
  credits     TINYBLOB     NOT NULL,
  ratio       TINYBLOB     NOT NULL,
  alldn       TINYBLOB     NOT NULL,
  allup       TINYBLOB     NOT NULL,
  daydn       TINYBLOB     NOT NULL,
  dayup       TINYBLOB     NOT NULL,
  monthdn     TINYBLOB     NOT NULL,
  monthup     TINYBLOB     NOT NULL,
  wkdn        TINYBLOB     NOT NULL,
  wkup        TINYBLOB     NOT NULL,
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
  host        VARCHAR(96) NOT NULL,
  PRIMARY KEY (uname,host)
);
