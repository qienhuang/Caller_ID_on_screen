#!/usr/bin/env python
# -*- coding:utf-8 -*-

#
#
# Programming By Kevin Huang 03-2016
# kevin11206@gmail.com                                            
#

import serial
import time
import MySQLdb  
import json
import threading
import time
import urllib
import demjson
import logging
import socket
from logging.handlers import RotatingFileHandler
from os import listdir
from array import array

##### Definition of functions and classes
class Caller:
	def __init__(self):
		self.clearBuffer()

	def clearBuffer(self):
		self.date =''
		self.time =''
		self.nmbr =''
		self.name =''

		self.lastInvoiceDate = ''
		self.companyName = ''
		self.address = ''
		self.city = ''
		self.state = ''

def phoneLineThread(line_number, serial_port_path):
	time.sleep(int(line_number)*2)
	#Disable line3 for testing
	if(line_number == 3):
		print "Line3 has been disabled for testing!!!!"
		return
	try:
		ser = serial.Serial(serial_port_path, 9600, timeout = None)
	except:
		print "Unable to open serial port"
		app_log.info("Unable to open serial port")
	time.sleep(1)
	ser.write("\r\nATZ\r\n")
	time.sleep(1)
	ser.write("\r\nAT\x2bVCID\x3d1\r\n")
	time.sleep(1)

	caller = Caller()

	while True:
		if ser.isOpen() == False:
			ser.open()
		try:
			re = ser.readline()
			if re.rstrip() != '':
				logString =  "Line" + str(line_number) + ": " + re
				print logString
				app_log.info(logString)
			
			if re[:4] =='DATE':			
				caller.date = re[7:].rstrip()
				#print re
				
			if re[:4] =='TIME':
				caller.time = re[7:].rstrip()
				#print re
			
			if re[:4] =='NMBR':
				caller.nmbr = re[7:].rstrip()
				#print "NMBR",re

			if re[:4] =='NAME':
				caller.name = re[7:].rstrip()
				#print re  
				logString = ">> "+caller.nmbr+" "+caller.name+" Date:"+caller.date+" Time:"+caller.time
				print logString
				app_log.info(logString)
				sql_str = "INSERT INTO call_log (phone_line, date,time,nmbr,name) \
					values ('%s','%s','%s','%s','%s' )" % \
					(str(line_number), caller.date, caller.time, caller.nmbr, caller.name)
				#sql_str = "INSERT INTO call_log (phone_line, date,time,nmbr,name) values ('1','0000','0000','1234567890','unknow')"
				try:
					db = MySQLdb.connect(host='192.168.1.10', user='root', passwd='password',db='modem')
					cursor = db.cursor()
					cursor.execute(sql_str)
					db.commit()
					sql_str = "select last_invoice_date, company_name, address, city, state \
								from customers where telephone1='%s'  \
								or telephone2='%s' limit 1" % (caller.nmbr, caller.nmbr)
					cursor.execute(sql_str)
					row = cursor.fetchone()
					if row is not None:
						data =  { 
								'line':str(line_number), \
								'nmbr':caller.nmbr, \
								'name':caller.name, \
								'last_invoice_date': row[0], \
								'company_name': row[1], \
								'address': row[2], \
								'city': row[3], \
								'state': row[4] \
								} 
					else:
						data =  { 
								'line':str(line_number), \
								'nmbr':caller.nmbr, \
								'name':caller.name, \
								'last_invoice_date':'', \
								'company_name':'', \
								'address': '', \
								'city': '', \
								'state': '' \
								} 						

					json_str = demjson.encode(data)
					logString = "Json str: ",json_str
					print logString
					app_log.info(logString)

					db.close()
				except MySQLdb.Error, e:
					try:
						logString = "MySQL Error [%d]: %s" % (e.args[0], e.args[1])
						print logString
						app_log.info(logString)
					except IndexError:
						logString = "MySQL Error: %s" % str(e)
						print logString
						app_log.info(logString)
				logString = '>> Data is stored into database!'
				print logString
				app_log.info(logString)
				caller.clearBuffer()

				try:
					threading.Thread( target = senderThread, args =(json_str,)).start()
				except Exception, e:
					logString = "Start senderThread Error: ",e[0]
					print logString
					app_log.info(logString)				

		except:
			pass

def udpListenServer():
	print "UDP Server running..."
	listenSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	listenSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	listenSock.bind(('', udpServerPort))

	while True:
		try:
			data, addr=listenSock.recvfrom(1024)
			## verifying data package
			if data[0] == b'\x01' and data[1] == b'\xab' and data[1023] == b'\x0d':
				#print data[0].encode("hex")," ", data[1023].encode("hex")
				if mutex.acquire():
					global cid_receivers
					if addr[0] not in cid_receivers:
						cid_receivers.append(addr[0])
					mutex.release()
				logString = "Receiver IPs: "+str(cid_receivers)
				print logString
				app_log.info(logString)
				listenSock.sendto('Hello',(addr[0],updClientPort))
		except Exception,e:
			print "UDP Server error..", e[0]
			return
	return

def senderThread(json_data):
	if mutex.acquire():		
		for idx, cid_recv in enumerate(cid_receivers):
			try:
				threading.Thread(target= sendSocketThread, args=(json_data, cid_recv)).start()
			except Exception, e:
				print "senderThread Error:", e[0]
		mutex.release()
	return

def sendSocketThread(json_data, recv_addr):
	print "sendSocketThread starting: ", json_data ,recv_addr
	try:
		sendSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sendSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1)
		sendSock.connect((recv_addr, tcpClientPort))
		global pack_head
		mod = divmod(len(json_data), 1024)
		pack_head[2] = mod[0]+1 	#how many package will be needed		
		sendSock.sendall(pack_head)
		sendSock.sendall(json_data)
	except Exception, e:
		if mutex.acquire():
			global cid_receivers	
			for idx, elem in enumerate(cid_receivers):
				#print "elem:", elem, " recv_addr ",recv_addr
				if elem == recv_addr:
					del cid_receivers[idx]
					print "cid_receivers", cid_receivers
					print "%s not response, it has been deleted." % (recv_addr)
			mutex.release() 
		print "sendSocketThread Error: ",e[0]
	return

#//////////////// RUN FROM HERE /////////////////
##### Setup Logging
LOG_FILE = '/var/www/cidmon.log'
log_formatter = logging.Formatter('%(asctime)s %(levelname)s %(funcName)s(%(lineno)d) %(message)s')

handler = logging.handlers.RotatingFileHandler(LOG_FILE, maxBytes=5*1024*1024, backupCount =2)
handler.setFormatter(log_formatter)
handler.setLevel(logging.INFO)

app_log = logging.getLogger('Kevin')
app_log .setLevel(logging.INFO)
app_log .addHandler(handler)

# Socket Port numbers definition
tcpServerPort = 5005
tcpClientPort = 5005
udpServerPort = 5001
updClientPort = 5002

#--- Package head ------------
pack_head = bytearray(1024)		# Initialize array size
pack_head[0]=1		#1 = 0x01
pack_head[1]=171	#171=0xab
pack_head[1023]=13	#13 = 0x0d


mutex = threading.Lock()
cid_receivers = ["192.168.1.10","192.168.1.99","192.168.1.111"]

### Caller ID detecting thread
phoneLine = 1
for serialPort in sorted(listdir("/dev/serial/by-path/")):
	portPath = "/dev/serial/by-path/" + serialPort
	print portPath
	app_log.info(portPath)
	# Example:
	# /dev/serial/by-path/platform-3f980000.usb-usb-0:1.2:1.0
	try:
		threading.Thread( target = phoneLineThread, args =(phoneLine,portPath)).start()
		phoneLine = phoneLine + 1
	
	except:
		print "Error: unable to start thread!"
		app_log.info("Error: unable to start thread!")

### Upd server thread, for client IP register
try:
	threading.Thread( target = udpListenServer, args =()).start()
except Exception,e:
	print "Error: Fail to start UDP Server", e[0]

# Keep running
while 1:
	pass

