CREATE DATABASE `auris` /*!40100 DEFAULT CHARACTER SET latin1 */;


DROP TABLE IF EXISTS `auris`.`device`;
CREATE TABLE  `auris`.`device`
(
  	`serialnumber` varchar(45) NOT NULL,

	`manufacturer` varchar(45) NOT NULL,

	`model` varchar(45) NOT NULL,

	PRIMARY KEY  (`serialnumber`)

) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `auris`.`object`;
CREATE TABLE  `auris`.`object`
(
	`type` varchar(45) NOT NULL,

	`version` varchar(10) NOT NULL,

	`name` varchar(45) NOT NULL,

	`json` varchar(9999) NOT NULL,
	`id` int(10) unsigned NOT NULL auto_increment,

	PRIMARY KEY  USING BTREE (`id`),

	KEY `descriptor` (`type`,`name`)

) ENGINE=InnoDB AUTO_INCREMENT=34 DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `auris`.`triggerr`;
CREATE TABLE  `auris`.`triggerr`
(
	`id` int(10) unsigned NOT NULL auto_increment,

	`type` varchar(45) NOT NULL,

	`name` varchar(45) NOT NULL,

	`version` varchar(10) NOT NULL,

	`event` varchar(45) NOT NULL,

	`value` varchar(45) NOT NULL,

	PRIMARY KEY  (`id`),

	UNIQUE KEY `onlyone` (`type`,`name`,`version`,`event`,`value`)

) ENGINE=InnoDB AUTO_INCREMENT=28 DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `auris`.`download`;
CREATE TABLE  `auris`.`download`
(
	`id` int(10) unsigned NOT NULL auto_increment,

	`type` varchar(45) NOT NULL,

	`name` varchar(45) NOT NULL,

	`version` varchar(10) NOT NULL,

	`trigger_id` int(10) unsigned NOT NULL,

	PRIMARY KEY  (`id`),
  UNIQUE KEY `onlyOne` USING BTREE (`type`,`name`,`version`,`trigger_id`),

	KEY `FK_download_1` (`trigger_id`),

	CONSTRAINT `FK_download_1` FOREIGN KEY (`trigger_id`) REFERENCES `triggerr` (`id`) ON DELETE CASCADE ON UPDATE CASCADE

) ENGINE=InnoDB AUTO_INCREMENT=187 DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `auris`.`downloaded`;
CREATE TABLE  `auris`.`downloaded`
(
  	`serialnumber` varchar(45) NOT NULL,

	`id` int(10) unsigned NOT NULL,

	PRIMARY KEY  USING BTREE (`serialnumber`,`id`),

	KEY `FK_object_id` (`id`),

	CONSTRAINT `FK_device_serialnumber`
	FOREIGN KEY (`serialnumber`) REFERENCES `device` (`serialnumber`) ON DELETE CASCADE ON UPDATE CASCADE,

	CONSTRAINT `FK_object_id`
	FOREIGN KEY (`id`) REFERENCES `object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE

) ENGINE=InnoDB DEFAULT CHARSET=latin1;

