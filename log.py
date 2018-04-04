#!/usr/bin/python

import shutil, time, serial, smtplib, socket, datetime
import pyscreenshot as ImageGrab
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText

ser = serial.Serial(
	port='/dev/ttyUSB0',\
	baudrate=9600,\
	parity=serial.PARITY_NONE,\
	stopbits=serial.STOPBITS_ONE,\
	bytesize=serial.EIGHTBITS,\
	timeout=1)


flag = 0
while True:
	ser.write(b'STA\r')
	time.sleep(0.25)
	status = ser.readall().split()
	i = status.index("ALMANAC")
	#print status
	if status[i+2] != "Locked" and flag == 0:
		log = str(datetime.datetime.utcnow()) + " " + status[i+2]
		print log
		flag = 1
		ImageGrab.grab_to_file(log.split('.')[0]+".png")
		shutil.copyfile("test.nav", log.split('.')[0]+".nav")
		try:
			server = smtplib.SMTP('smtp.gmail.com', 587)
			server.starttls()
			server.login("juwhan1019@gmail.com", "sdpeukiklmvknptd")
			server.sendmail("juwhan1019@gmail.com", "juwhan1019@gmail.com", status[i+2])
			server.quit()
		except Exception:
			server = None
	elif status[i+2] == "Locked" and flag == 1:
		log = str(datetime.datetime.utcnow()) + " " + status[i+2]
		print log
		flag = 0
		try:
			server = smtplib.SMTP('smtp.gmail.com', 587)
			server.starttls()
			server.login("juwhan1019@gmail.com", "sdpeukiklmvknptd")
			server.sendmail("juwhan1019@gmail.com", "juwhan1019@gmail.com", status[i+2])
			server.quit()
		except Exception:
			server = None
