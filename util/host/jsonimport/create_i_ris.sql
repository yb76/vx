CREATE DATABASE `i-ris` /*!40100 DEFAULT CHARACTER SET latin1 */;


DROP TABLE IF EXISTS `i-ris`.`device`;
CREATE TABLE  `i-ris`.`device`
(
  	`serialnumber` varchar(45) NOT NULL,

	`manufacturer` varchar(45) NOT NULL,

	`model` varchar(45) NOT NULL,

	PRIMARY KEY  (`serialnumber`)

) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `i-ris`.`object`;
CREATE TABLE  `i-ris`.`object`
(
	`type` varchar(45) NOT NULL,

	`version` varchar(10) NOT NULL,

	`name` varchar(45) NOT NULL,

	`json` varchar(9999) NOT NULL,
	`id` int(10) unsigned NOT NULL auto_increment,

	PRIMARY KEY  USING BTREE (`id`),

	KEY `descriptor` (`type`,`name`)

) ENGINE=InnoDB AUTO_INCREMENT=34 DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `i-ris`.`triggerr`;
CREATE TABLE  `i-ris`.`triggerr`
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

DROP TABLE IF EXISTS `i-ris`.`download`;
CREATE TABLE  `i-ris`.`download`
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

DROP TABLE IF EXISTS `i-ris`.`downloaded`;
CREATE TABLE  `i-ris`.`downloaded`
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



























DROP TABLE IF EXISTS device;
CREATE TABLE  device
(
  	serialnumber varchar(45) NOT NULL,

	manufacturer varchar(45) NOT NULL,

	model varchar(45) NOT NULL,

	PRIMARY KEY  (serialnumber)

);

DROP TABLE IF EXISTS object;
CREATE TABLE  object
(
	type varchar(45) NOT NULL,

	version varchar(10) NOT NULL,

	name varchar(45) NOT NULL,

	json varchar(9999) NOT NULL,
	id serial NOT NULL,

	PRIMARY KEY  (id),

	UNIQUE (type,name)

);

DROP TABLE IF EXISTS triggerr;
CREATE TABLE  triggerr
(
	id serial NOT NULL,

	type varchar(45) NOT NULL,

	name varchar(45) NOT NULL,

	version varchar(10) NOT NULL,

	event varchar(45) NOT NULL,

	value varchar(45) NOT NULL,

	PRIMARY KEY  (id),

	UNIQUE (type,name,version,event,value)

);

DROP TABLE IF EXISTS download;
CREATE TABLE  download
(
	id serial NOT NULL,

	type varchar(45) NOT NULL,

	name varchar(45) NOT NULL,

	version varchar(10) NOT NULL,

	trigger_id integer NOT NULL,

	PRIMARY KEY  (id),

	UNIQUE (type,name,version,trigger_id)
);

DROP TABLE IF EXISTS downloaded;
CREATE TABLE  downloaded
(
  	serialnumber varchar(45) NOT NULL,

	id serial NOT NULL,

	PRIMARY KEY (serialnumber,id)
);

