--
-- Pre-times tables (MySQL syntax).
--

CREATE TABLE `pretimes` (
  `id`        int(10) unsigned     NOT NULL auto_increment,
  `pretime`   int(11)              NOT NULL default '0',  -- Time stamp of pre.
  `section`   varchar(25)          default NULL,          -- Section name.
  `release`   varchar(255)         NOT NULL default '',   -- Release name.
  `files`     int(10) unsigned     NOT NULL default '0',  -- Number of files present.
  `kbytes`    int(10) unsigned     NOT NULL default '0',  -- Size in kilobytes.
  `disks`     smallint(5) unsigned NOT NULL default '0',  -- Number of disks (CDs/DVDs).
  `nuked`     smallint(1)          NOT NULL default '0',  -- Indicates if the release is nuked.
  `nuketime`  int(11)              NOT NULL default '0',  -- Time stamp of nuke.
  `reason`    varchar(255)         default NULL,          -- Reason for nuke.
  PRIMARY KEY (`id`),
  UNIQUE KEY  (`release`)
);
