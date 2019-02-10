# Caller ID On Screen

## An application for telephone caller ID display on screen. Implemented in Raspberry Pi, a low-cost and low-power single board computer.

This application was created for my customer support team to response quickly and efficiently for incoming service calls.

![diagram](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/img/diagram.png)

### Components:

[cidmon.go](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/cidmon.go)  # The server App runs on Raspberry Pi, coding in Golang

Features:

  - Supports multiple phone lines
  - Mornitoring USB modem hot plug/unplug for phone line adding or removing
  - Matching phone number from accounting database, returns customer's address and last purchase date
  
![server_running](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/img/server_running.png)

Client     # The client program runs on Windows PC, implemented on [Visual C++](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/client_cpp/cider/cider/ciderDlg.cpp) and [C#/Winforms](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/client_cs/cider/cider/Form1.cs)

Features:
  - No configure required, automatically register on server
  - Receives caller information and view call log

The caller ID popup program on Windows PC:

![main_form](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/img/main_form.png)

The expanded window to view call logs:

![main_form_expanded](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/img/main_form_expanded.png)

The call log page from the company web portal(Java/Spring/Tomcat):

![call log page](https://raw.githubusercontent.com/qienhuang/Caller_ID_on_screen/master/img/web_calllog.png)

### Deployment:

On raspberry Pi/Debian:

#If you haven't the mysql installed:
```
sudo apt install mysql-server mysql-client
```

#Create database 'modem' and import tables
```
sudo mysql -uroot -p -e "create database modem"
sudo mysql -uroot -p  modem < mysql_import.sql
```
#To fix "Ping MySQL host error:  Error 1698: Access denied for user 'root'@'localhost'"
```
sudo mysql -uroot -p < change_root_plugin.sql
```
#If your mysql root user password isn't blank, please update it from the line:

#sql.Open("mysql", "root:your_password@tcp(localhost:3306)/modem?charset=utf8")
```
nano cidmon.go
```
#Build
```
go get github.com/go-sql-driver/mysql
go get github.com/mattn/go-sqlite3
go get github.com/tarm/serial
go build
```
#run
```
sudo ./cidmon
```

### #Or if you like to use the pre-built app
```
cd /home/pi/Downloads
wget https://github.com/qienhuang/Caller_ID_on_screen/raw/master/bin/cidmon
wget https://github.com/qienhuang/Caller_ID_on_screen/raw/master/cidmon.service
```

#Run App as a service:
```
sudo mkdir -p /usr/local/cidmon && cp cidmon /usr/local/cidmon
sudo cp cidmon.service /etc/systemd/system
sudo systemctl enable cidmon.service
sudo systemctl start cidmon.service
```

On Windows stations:

Download the pre-built client from [here](https://github.com/qienhuang/Caller_ID_on_screen/raw/master/bin/cider.exe), then create a shortcut for cider.exe and add it to startup folder.

Plan for improvement:
 - MQTT with TLS secured messaging

Others:

- [archived/cidmon.c](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/archived/cidmon.c)  -The previous version of server app that written in c. Implemented serial port communication, multi-threads, tcp/udp socket, MySQL database Read/Write...
- [archived/cidmon.py](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/archived/cidmon.py)  -The previous version of server app that written in Python.
- [img/diagram.ai](https://github.com/qienhuang/Caller_ID_on_screen/blob/master/img/diagram.ai)    -The vector source file of diagram drawing(isometric view)
