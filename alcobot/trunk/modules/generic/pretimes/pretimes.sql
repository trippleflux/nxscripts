#
# Pre time table schema (same as the one used by nxTools).
#

CREATE TABLE `pretimes` (
  `id`       int unsigned      NOT NULL auto_increment,
  `pretime`  int(10)           NOT NULL default '0',
  `section`  varchar(20)       NOT NULL default '',
  `release`  varchar(255)      NOT NULL default '',
  `files`    smallint unsigned NOT NULL default '0',
  `kbytes`   int unsigned      NOT NULL default '0',
  `disks`    tinyint unsigned  NOT NULL default '0',
  `nuked`    tinyint(1)        NOT NULL default '0',
  `nuketime` int(10)           NOT NULL default '0',
  `reason`   varchar(255)      NOT NULL default '',
  PRIMARY KEY (`id`),
  UNIQUE KEY  (`release`)
);
