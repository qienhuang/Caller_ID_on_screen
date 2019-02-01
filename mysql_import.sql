-- MySQL dump 10.13  Distrib 5.5.62, for debian-linux-gnu (armv8l)


--
-- Table structure for table `call_log`
--

DROP TABLE IF EXISTS `call_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `call_log` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `phone_line` varchar(25) DEFAULT NULL,
  `date` char(4) DEFAULT NULL,
  `time` char(4) DEFAULT NULL,
  `nmbr` varchar(25) DEFAULT NULL,
  `name` varchar(25) DEFAULT NULL,
  `customer_record` int(11) DEFAULT NULL,
  `time_stamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=13548 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `customers`
--

DROP TABLE IF EXISTS `customers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `customers` (
  `customer_record` int(11) NOT NULL,
  `last_invoice_date` varchar(25) DEFAULT NULL,
  `company_name` varchar(50) DEFAULT NULL,
  `address` varchar(50) DEFAULT NULL,
  `city` varchar(25) DEFAULT NULL,
  `state` varchar(25) DEFAULT NULL,
  `telephone1` varchar(25) DEFAULT NULL,
  `telephone2` varchar(25) DEFAULT NULL,
  `id` int(11) NOT NULL,
  PRIMARY KEY (`customer_record`),
  UNIQUE KEY `customer_record` (`customer_record`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `session_log`
--

DROP TABLE IF EXISTS `session_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `session_log` (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `ip` char(20) DEFAULT NULL,
  `name` char(50) DEFAULT NULL,
  `time_stamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `url` char(50) DEFAULT NULL,
  `Country` char(50) DEFAULT NULL,
  `CountryCode` char(10) DEFAULT NULL,
  `Region` char(50) DEFAULT NULL,
  `RegionName` char(50) DEFAULT NULL,
  `org` char(50) DEFAULT NULL,
  `isp` char(50) DEFAULT NULL,
  `lat` char(50) DEFAULT NULL,
  `lon` char(50) DEFAULT NULL,
  `asName` char(50) DEFAULT NULL,
  `zip` char(20) DEFAULT NULL,
  `city` char(50) DEFAULT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB AUTO_INCREMENT=15345 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;


-- Dump completed on 2019-01-28 22:11:39
