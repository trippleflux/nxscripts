--
-- Invite table schemas.
--

CREATE TABLE `invite_users` (
  `ftp_user` varchar(100) NOT NULL default '',  -- FTP user name.
  `irc_user` varchar(100) default NULL,         -- IRC user name.
  `online`   smallint(1)  NOT NULL default '0', -- Indicates if the user is on IRC.
  `password` varchar(100) default NULL,         -- Password hash (PKCS #5 v2 based).
  `time`     int(11)      NOT NULL default '0', -- Time stamp of last activity.
  PRIMARY KEY (`ftp_user`)
);

CREATE TABLE `invite_hosts` (
  `ftp_user` varchar(100) NOT NULL default '', -- FTP user name.
  `hostmask` varchar(255) NOT NULL default '', -- IRC host mask (ident@host).
  PRIMARY KEY (`ftp_user`,`hostmask`)
);
