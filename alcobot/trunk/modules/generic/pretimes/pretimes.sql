--
-- Pre time table schema.
--

CREATE TABLE `pretimes` (
  `id`       int unsigned      NOT NULL auto_increment,
  `pretime`  int(10)           NOT NULL default '0', -- Time stamp of pre.
  `section`  varchar(20)       NOT NULL default '',  -- Section name.
  `release`  varchar(255)      NOT NULL default '',  -- Release name.
  `files`    smallint unsigned NOT NULL default '0', -- Number of files present.
  `kbytes`   int unsigned      NOT NULL default '0', -- Size in kilobytes.
  `disks`    tinyint unsigned  NOT NULL default '0', -- Number of disks (CDs/DVDs).
  `nuked`    tinyint(1)        NOT NULL default '0', -- Indicates if the release is nuked.
  `nuketime` int(10)           NOT NULL default '0', -- Time stamp of nuke.
  `reason`   varchar(255)      NOT NULL default '',  -- Reason for nuke.
  PRIMARY KEY (`id`),
  UNIQUE KEY  (`release`)
);
