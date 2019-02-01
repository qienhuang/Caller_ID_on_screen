/*

Programming by Kevin Huang
2016-2018
kevin11206@gmail.com
Caller ID monitoring Application
Platform: Raspberry Pi/Debian

*/

package main

/*
      main()
        |
  initializeMySQL()
  initializeSQLite()
   	|
	| ______________________________________________________________________________
	|                                              |                                |
        V                                              V                                V
  initializeModems():                        udpRegisterServerThread():       queryListenServerThread():
  enumerate serial port                                |                                |
  send ATZ to initialize                               V                                V
  send AT+VCID=1 to enable caller ID         udpRegisterResponseThread():        queryResponseThread():
   if received OK                                                                   parse CMD:
        |                                                                          GET_CALL_LOG
        V
 go callMonitorThread()
    parse caller id
        |
        V
 go callTcpDispatcherThread()
	build json
    for each clinets
        |
        V
 go callTcpSendThread()
    send package head
    send json data
*/

import (
	"database/sql"
	"encoding/binary"
	"encoding/json"
	_ "fmt" // "_" ignore unused imported-package
	"io/ioutil"
	"log"
	"net"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	_ "github.com/go-sql-driver/mysql"
	_ "github.com/mattn/go-sqlite3"
	"github.com/tarm/serial"
)

//Network port assignment
const (
	UDP_LISTEN_PORT       = "5001"
	UDP_CLIENT_PORT       = "5002"
	TCP_LISTEN_PORT       = "5005"
	TCP_CLIENT_PORT       = "5005"
	CHECK_MODEMS_INTERVAL = 120 //  check ports interval in seconds
	ENABLE_MYSQL          = true
	EMABLE_SQLITE         = true

	// Custom protocol
	START_OF_HEADER     = 0x01 //byte[0]    - Start of Header
	ACK                 = 0xab //byte[1]    - acknowledgment character
	END_OF_TRANSMISSION = 0x04 //byte[1022] - End of Transmission
	CARRIAGE_RETURN     = 0x0d //byte[1023] - Carriage Return  (A package end, return to next one)
)

//The TCP Client
type TCPClient struct {
	Ip string
}

//For caller id parsing
type CallerID struct {
	LINE int
	DATE string
	TIME string
	NMBR string
	NAME string
}

//For modem enumerate
type Modem struct {
	Id       int
	Path     string
	LineNum  int
	PhoneNum int
	Handle   *serial.Port
}

//For database row scan
type Customer struct {
	Customer_record   sql.NullInt64
	Last_invoice_date sql.NullString
	Company_name      sql.NullString
	Address           sql.NullString
	City              sql.NullString
	State             sql.NullString
}

// For database Call_Log row scan
type CallLogRow struct {
	Id                sql.NullInt64
	Phone_line        sql.NullString
	Date              sql.NullString
	Time              sql.NullString
	Nmbr              sql.NullString
	Name              sql.NullString
	Customer_record   sql.NullInt64
	Last_invoice_date sql.NullString
	Company_name      sql.NullString
	Address           sql.NullString
	City              sql.NullString
	State             sql.NullString
	Custom_col1       sql.NullString
	Custom_col2       sql.NullString
	Custom_col3       sql.NullString
	Custom_col4       sql.NullString
	Custom_col5       sql.NullString
	Time_stamp        time.Time
}

//Global variables
var (
	db          *sql.DB
	sqliteDB    *sql.DB
	dbPingOk    bool
	serialPorts []string    // Paths
	modems      []Modem     // Modems enumerate
	tcpClinets  []TCPClient // Saving Ip adress
	//tcpClinets []TCPClient = []TCPClient{TCPClient{Ip: "192.168.1.10"}} // Saving Ip adress
	blockedIPs []string = []string{} // []string{"192.168.1.20", "192.168.1.99"}
	//blockedIPs   []string   = []string{"192.168.1.20", "192.168.1.99"}
	mutexClients sync.Mutex // lock for add and delete Ip
	mutexModems  sync.Mutex // lock for add/delete modems

)

//Json for tcpClinets and Call_Log requsts
type CallerJson struct {
	//{"address":"","city":"","company_name":"","last_invoice_date":"","line":"1","name":"KEVIN HUANG","nmbr":"13470000000","state":""}
	//use json tag to specifies the key name
	Id                string    `json:"id"`
	Line              string    `json:"line"`
	Date              string    `json:"date"`
	Time              string    `json:"time"`
	Nmbr              string    `json:"nmbr"`
	Name              string    `json:"name"`
	Customer_record   string    `json:"customer_record"`
	Last_invoice_date string    `json:"last_invoice_date"`
	Company_name      string    `json:"company_name"`
	Address           string    `json:"address"`
	City              string    `json:"city"`
	State             string    `json:"state"`
	Custom_col1       string    `json:"custom_col1"`
	Custom_col2       string    `json:"custom_col2"`
	Custom_col3       string    `json:"custom_col3"`
	Custom_col4       string    `json:"custom_col4"`
	Custom_col5       string    `json:"custom_col5"`
	Time_stamp        time.Time `json:"time_stamp"`
}

//General function
func checkError(err error, args ...string) {
	/*	Example:
		checkError(err)
		checkError(err, "Fatal")
		checkError(err, "Fatal", "Read file error")
		checkError(err, "Read file error")
	*/
	if err == nil { //No error
		return
	}
	tag := ""
	logStyle := ""

	for _, arg := range args {
		switch arg {
		case "fatal", "Fatal", "FATAL":
			logStyle = "fatal"
		case "panic", "Panic", "PANIC":
			logStyle = "panic"
		default:
			tag = arg
		}
	}

	//If the tag isn't gaven, then use the caller name instead
	if tag == "" {
		fpcs := make([]uintptr, 1)
		n := runtime.Callers(3, fpcs)
		if n != 0 {
			fun := runtime.FuncForPC(fpcs[0] - 1)
			if fun != nil {
				tag = "From " + fun.Name() + ": "
			}
		}
	}

	switch logStyle {
	case "fatal":
		log.Fatal(tag, err)
	case "panic":
		log.Panic(tag, err)
	default:
		log.Println(tag, err)
	}
}

//UDP listening server for client registration
func udpRegisterListenThread() {
	log.Printf("UDP server is starting...")
	ServerAddr, err := net.ResolveUDPAddr("udp", ":"+UDP_LISTEN_PORT)
	checkError(err, "Fatal")
	ServerConn, err := net.ListenUDP("udp", ServerAddr)
	checkError(err, "Fatal")

	defer ServerConn.Close()

	//Client register pack sample: [0]=0x01, [1]=0xab, [2]='O', [3]='K', [4]='!', buf[1023] = 0x0d
	buf := make([]byte, 1024)
	//tcpClinets = tcpClinets[:0] //clear tcpClinets
	for {
		n, addr, err := ServerConn.ReadFromUDP(buf)
		log.Printf("UDP received from [%s]", addr.IP.String())
		checkError(err)
		//log.Printf("Received %d bytes from %s ", n, addr)
		//log.Printf("%q ", buf[0:10])
		if n != 1024 {
			continue
		}

		if buf[0] == START_OF_HEADER && buf[1] == ACK && buf[1023] == CARRIAGE_RETURN {
			mutexClients.Lock()
			registered := false
			for _, v := range tcpClinets {
				if v.Ip == addr.IP.String() {
					registered = true
					break
				}
			}
			if registered {
				// Already registered
				//log.Printf("Client %s already in the list",  addr.IP.String())
			} else {
				var cd TCPClient
				cd.Ip = addr.IP.String()
				tcpClinets = append(tcpClinets, cd)
				log.Printf("Registered a new client: %s", cd.Ip)
			}
			mutexClients.Unlock()
			go udpRegisterResponseThread(addr.IP.String())
		}
	}
}

// UDP send confirmation to client
func udpRegisterResponseThread(clientIP string) {
	conn, err := net.Dial("udp", clientIP+":"+UDP_CLIENT_PORT)
	//log.Printf("udp sending to %s:%s...\n", clientIP, UDP_CLIENT_PORT)
	if err != nil {
		checkError(err)
	}
	defer conn.Close()
	buf := make([]byte, 1024)
	buf[0] = START_OF_HEADER
	buf[1] = ACK
	buf[2] = 'O'
	buf[3] = 'K'
	buf[4] = '!'
	buf[1023] = CARRIAGE_RETURN

	conn.Write(buf)

}

//Initialize all modems
func initializeModems() {
	log.Println("Enumerating serial ports...")
	var files []os.FileInfo
	var err error
	var isExisting bool

	for {
		files, err = ioutil.ReadDir("/dev/serial/by-path")
		if err != nil {
			checkError(err, "Fail to enumerate serial ports:")
			time.Sleep(CHECK_MODEMS_INTERVAL * time.Second)
			continue
		}

		serialPorts = serialPorts[:0] // clear

		mutexModems.Lock()
		for _, f := range files {
			isExisting = false
			for _, m := range modems {
				if m.Path == ("/dev/serial/by-path/" + f.Name()) {
					isExisting = true
					//log.Printf("/dev/serial/by-path/%s already in modem list " ,f.Name())
					break
				}

			}
			if !isExisting {
				serialPorts = append(serialPorts, "/dev/serial/by-path/"+f.Name())
				log.Printf("/dev/serial/by-path/%s added to serialPort list ", f.Name())
			}

		}
		mutexModems.Unlock()

		for _, p := range serialPorts { //range slice return two values, first one is index
			log.Println(p)

			//Sample :  /dev/serial/by-path/platform-3f980000.usb-usb-0:1.5:1.0
			c := &serial.Config{Name: p, Baud: 9600}
			s, err := serial.OpenPort(c)
			if err != nil {
				log.Println(p)
				checkError(err, "Serial port open error: ")
				continue
			}

			//Send ATZ command to initialize modem
			n, err := s.Write([]byte("\r\nATZ\r\n"))
			checkError(err, "Modem ATZ error: ")
			//Wait for command completed
			time.Sleep(1 * time.Second)

			//Read response
			buf := make([]byte, 128)
			n, err = s.Read(buf)
			checkError(err, "Serial port read error: ")
			//log.Printf("%q", buf[:n])
			log.Printf("%s", string(buf[:n]))

			//Send AT+VCID=1 enable caller id
			n, err = s.Write([]byte("\r\nAT+VCID=1\r\n"))
			checkError(err, "Modem enable caller id error: ")
			//Wait for command completed
			time.Sleep(1 * time.Second)

			//Read response
			n, err = s.Read(buf)
			checkError(err, "Serial port read error: ")
			//log.Printf("%q", buf[:n])	//Print out ASCII
			log.Printf("%s", string(buf[:n]))
			if !strings.Contains(string(buf[:n]), "OK") {
				continue //Enable caller ID failed
				s.Close()
			}

			var modem Modem
			modem.Id = len(modems) + 1
			modem.Path = p
			modem.Handle = s
			modems = append(modems, modem)

			go callMonitorThread(modem)
		}
		// Check the serial ports every 2 minutes
		time.Sleep(CHECK_MODEMS_INTERVAL * time.Second)

	}

}

func callMonitorThread(modem Modem) {

	log.Printf("Modem#%d thread is starting...", modem.Id)
	defer modem.Handle.Close()
	buffer := make([]byte, 128)

	//Caller ID parsing
	for {
		//Read response
		n, err := modem.Handle.Read(buffer)
		if err != nil {
			log.Printf("Serial port %s read error!", modem.Path)
			mutexModems.Lock()
			for i, m := range modems {
				if m.Path == modem.Path {
					modems = append(modems[:i], modems[i+1:]...)
					break
				}
			}
			log.Printf("Serial port %s removed from the list", modem.Path)
			mutexModems.Unlock()
			return
		}

		lines := strings.Split(string(buffer[:n]), "\n")
		var callerID CallerID
		for _, line := range lines {
			if len(line) < 6 {
				continue
			}
			if line[:4] == "DATE" {
				callerID.DATE = line[7:]
				callerID.DATE = strings.Trim(callerID.DATE, "\r ")
			}
			if line[:4] == "TIME" {
				callerID.TIME = line[7:]
				callerID.TIME = strings.Trim(callerID.TIME, "\r ")
			}
			if line[:4] == "NMBR" {
				callerID.NMBR = line[7:]
				callerID.NMBR = strings.Trim(callerID.NMBR, "\r+ ")
				// Remove first number '1'
				if len(callerID.NMBR) == 11 {
					// Only need last 10 numbers
					callerID.NMBR = callerID.NMBR[len(callerID.NMBR)-10:]
				}
			}
			if line[:4] == "NAME" {
				callerID.NAME = line[7:]
				callerID.NAME = strings.Trim(callerID.NAME, "\r ")
				callerID.LINE = modem.Id
				log.Println(callerID)

				//Finish receiving caller id, start tcp send thread
				go callTcpDispatcherThread(callerID, modem)
			}
		} //for range parsing string lines

	} // for loop
}

//TCP dispatcher thread
func callTcpDispatcherThread(callerID CallerID, modem Modem) {
	log.Printf("Modem#%d TCP dispatcher starting...", modem.Id)

	var js_mysql = CallerJson{
		//Line: strconv.Itoa(modem.LineNum),	//Will allow user specifies the lineNum to a Modem.Id
		Line: strconv.Itoa(modem.Id),
		Name: callerID.NAME,
		Nmbr: callerID.NMBR,
		Date: callerID.DATE,
		Time: callerID.TIME,
	}

	var js_sqlite = CallerJson{
		//Line: strconv.Itoa(modem.LineNum),   //Will allow user specifies the lineNum to a Modem.Id
		Line: strconv.Itoa(modem.Id),
		Name: callerID.NAME,
		Nmbr: callerID.NMBR,
		Date: callerID.DATE,
		Time: callerID.TIME,
	}

	var stmt *sql.Stmt
	var err error

	// Fetching and updating data from mysql
	if dbPingOk && ENABLE_MYSQL {

		// Fetching caller information
		stmt, err = db.Prepare("select customer_record,last_invoice_date, company_name, address, city, state from customers where telephone1=? or telephone2=? limit 1")
		checkError(err, "MySQL db.Prepare error: ")
		rows, err := stmt.Query(callerID.NMBR, callerID.NMBR)
		checkError(err, "MySQL Query error: ")

		var customer Customer
		for rows.Next() {
			err = rows.Scan(
				&customer.Customer_record,
				&customer.Last_invoice_date,
				&customer.Company_name,
				&customer.Address,
				&customer.City,
				&customer.State,
			)
			if err != nil {
				checkError(err, "MySQL rows.Scan error: ")
			}
			js_mysql.Customer_record = strconv.FormatInt(customer.Customer_record.Int64, 10)
			js_mysql.Last_invoice_date = customer.Last_invoice_date.String
			js_mysql.Company_name = customer.Company_name.String
			js_mysql.Address = customer.Address.String
			js_mysql.City = customer.City.String
			js_mysql.State = customer.State.String //Don't forget this comma at the last
		}
		//log.Println( customer.Company_name.String)	//sql.NullString need to be convert to string

		// Insert record into call_log table
		stmt, err = db.Prepare("INSERT INTO call_log (phone_line, date,time,nmbr,name) values (?, ?, ?, ?, ?)")
		checkError(err, "Prepare insert into caller_log error")
		_, err = stmt.Exec(strconv.Itoa(modem.Id), callerID.DATE, callerID.TIME, callerID.NMBR, callerID.NAME)
		checkError(err, "Excec insert into caller_log error")

	}

	// Sqlite database query
	if EMABLE_SQLITE {

		// Fetching caller information
		stmt, err = sqliteDB.Prepare("select customer_record,last_invoice_date, company_name, address, city, state from customers where telephone1=? or telephone2=? limit 1")
		checkError(err, "Sqlite db.Prepare error: ")
		rows, err := stmt.Query(callerID.NMBR, callerID.NMBR)
		checkError(err, "Sqlite Query error: ")

		var cust Customer
		for rows.Next() {
			err = rows.Scan(
				&cust.Customer_record,
				&cust.Last_invoice_date,
				&cust.Company_name,
				&cust.Address,
				&cust.City,
				&cust.State, //Don't forget this comma at the last
			)
			if err != nil {
				checkError(err, "Sqlite rows.Scan error: ")
			}
			js_sqlite.Customer_record = strconv.FormatInt(cust.Customer_record.Int64, 10)
			js_sqlite.Last_invoice_date = cust.Last_invoice_date.String
			js_sqlite.Company_name = cust.Company_name.String
			js_sqlite.Address = cust.Address.String
			js_sqlite.City = cust.City.String
			js_sqlite.State = cust.State.String 
		}
		//log.Println( cust.Company_name.String)	//sql.NullString need to be convert to string

		// Insert record into call_log table
		stmt, err = sqliteDB.Prepare("INSERT INTO call_log (phone_line, date,time,nmbr,name, customer_record, company_name, address,city,state,last_invoice_date) values (?,?,?,?,?,?,?,?,?,?,?)")
		checkError(err, "Sqlite repare insert into caller_log error")
		_, err = stmt.Exec(strconv.Itoa(modem.Id), callerID.DATE, callerID.TIME, callerID.NMBR, callerID.NAME, strconv.FormatInt(cust.Customer_record.Int64, 10),
			cust.Company_name.String, cust.Address.String, cust.City.String, cust.State.String, cust.Last_invoice_date.String)
		checkError(err, "Sqlite excec insert into caller_log error")
	}

	// Build json string
	j, err := json.Marshal(js_sqlite)
	checkError(err)
	for _, c := range tcpClinets {
		go callTcpSendThread(c, j)
	}
}

//TCP sender thread, called by dispatcher
func callTcpSendThread(client TCPClient, bJson []byte) {

	// Blcoks some IPs
	for _, Ip := range blockedIPs {
		if client.Ip == Ip {
			log.Print("IP: ", Ip, " is blocked")
			return
		}
	}

	log.Println("TCP Sending to: ", client.Ip)
	/*
	* JSON sample:
	* {"address":"","city":"","company_name":"","last_invoice_date":"","line":"1","name":"13475251918","nmbr":"13475251918","state":""}
	 */
	conn, err := net.Dial("tcp", client.Ip+":"+TCP_CLIENT_PORT)
	if err != nil {
		log.Println(err)

		//Remove this client
		mutexClients.Lock()
		var i int
		var cli TCPClient
		for i, cli = range tcpClinets {

			if cli.Ip == client.Ip {
				tcpClinets = append(tcpClinets[:i], tcpClinets[i+1:]...)
				break //stop searching
			}
		}
		mutexClients.Unlock()
		log.Printf("Client IP %s has been removed.\n", client.Ip)
		return
	}

	defer conn.Close()
	//Build package header
	var protocol_head [1024]byte //an array
	protocol_head[0] = START_OF_HEADER
	protocol_head[1] = ACK
	protocol_head[2] = byte(len(bJson)/1024 + 1) //If json over 1024 bytes, use this byte to determine how many package will be sent. Example: 2*1024
	protocol_head[1023] = CARRIAGE_RETURN

	//packing json data
	var protocol_data []byte = make([]byte, len(bJson)/1024*1024+1024)
	for i := 0; i < len(bJson); i++ {
		protocol_data[i] = bJson[i]
	}
	//Send header
	_, err = conn.Write(protocol_head[0:1024]) //conn.Write accepts slice. Not includes [1024]
	checkError(err, "TCP Send header error ")
	//Send json
	_, err = conn.Write(protocol_data)
	checkError(err, "TCP Send json error ")
	//fmt.Printf("%s", string(protocol_data))
}

// TCP Listen server, for sending call_log
func queryListenServerThread() {
	log.Printf("TCP server is starting...")
	ServerAddr, err := net.ResolveTCPAddr("tcp4", ":"+TCP_LISTEN_PORT)
	checkError(err, "Fatal")
	ServerConn, err := net.ListenTCP("tcp", ServerAddr)
	checkError(err, "Fatal")

	defer ServerConn.Close()
	for {
		conn, err := ServerConn.Accept()
		if err == nil {
			go queryResponseThread(conn)
		} else {
			checkError(err)
		}
	}
}

func queryResponseThread(conn net.Conn) {
	defer func() {
		conn.Close()
		log.Println("Query process is completed.")
	}() // defer an anonymous function

	buf := make([]byte, 1024)
	//cmd, err := ioutil.ReadAll(conn)
	// BUG, DOES'T SET TIMEOUT
	n, err := conn.Read(buf)
	checkError(err)
	if n != 1024 {
		log.Printf("Received data from %s , but it's an invalid package length!", conn.RemoteAddr().String())
		return
	}
	// Verifying protocol only need to do on server side(The listener)
	if buf[0] != START_OF_HEADER || buf[1] != ACK || buf[1023] != CARRIAGE_RETURN {
		log.Printf("Received data from %s , but it's an invalid protocol!", conn.RemoteAddr().String())
		return
	}
	// Parsing command. Examle: GET_CALL_LOG
	strCmd := string(buf[2:14]) // A command must be a 12 characters
	strCmd = strings.TrimSpace(strCmd)
	log.Printf("Received command:[%s] from %s", strCmd, conn.RemoteAddr().String())

	// Building data protocol
	var data []byte = make([]byte, 1024)
	data[0] = START_OF_HEADER
	data[1] = ACK
	data[2] = 0 //the size of json - int32 value is stored from this byte
	data[3] = 0
	data[4] = 0
	data[5] = 0
	data[1023] = CARRIAGE_RETURN

	// send call_log to clients
	switch strCmd {
	case "GET_CALL_LOG":

		// Fetching call_log
		log.Println("Starting GET_CALL_LOG")
		stmt, err := sqliteDB.Prepare(`select id,phone_line,strftime('%m%d%Y',time_stamp),time,nmbr,name,customer_record,last_invoice_date,company_name,address,city,state,
		 custom_col1,custom_col3,custom_col3,custom_col4,custom_col5,time_stamp from call_log order by id desc limit 250`)
		checkError(err, "Sqlite db.Prepare error: ")
		rows, err := stmt.Query()
		checkError(err, "Sqlite Query error: ")
		defer rows.Close()

		for rows.Next() {
			var r CallLogRow
			err = rows.Scan(
				&r.Id,
				&r.Phone_line,
				&r.Date,
				&r.Time,
				&r.Nmbr,
				&r.Name,
				&r.Customer_record,
				&r.Last_invoice_date,
				&r.Company_name,
				&r.Address,
				&r.City,
				&r.State,
				&r.Custom_col1,
				&r.Custom_col2,
				&r.Custom_col3,
				&r.Custom_col4,
				&r.Custom_col5,
				&r.Time_stamp,
			)
			if err != nil {
				checkError(err, "Sqlite rows.Scan error: ")
			}
			// Building json
			var js CallerJson
			js.Id = strconv.FormatInt(r.Id.Int64, 10) // 10 is decimal, 16 is Hex
			js.Line = r.Phone_line.String
			js.Date = r.Date.String
			js.Time = r.Time.String
			js.Nmbr = r.Nmbr.String
			js.Name = r.Name.String
			js.Customer_record = strconv.FormatInt(r.Customer_record.Int64, 10)
			js.Last_invoice_date = r.Last_invoice_date.String
			js.Company_name = r.Company_name.String
			js.Address = r.Address.String
			js.City = r.City.String
			js.State = r.State.String
			js.Custom_col1 = r.Custom_col1.String
			js.Custom_col2 = r.Custom_col2.String
			js.Custom_col3 = r.Custom_col3.String
			js.Custom_col4 = r.Custom_col4.String
			js.Custom_col5 = r.Custom_col5.String
			js.Time_stamp = r.Time_stamp

			// Build json string
			j, err := json.Marshal(js)
			checkError(err)

			// Build protocol
			//log.Printf("Json size is:%d", len(j))
			int32Array := make([]byte, 4)
			// ARM architecture is Big Endian
			binary.LittleEndian.PutUint32(int32Array, uint32(len(j))) // No need to be reversed on x86 side
			// Set json size(UInt32), buff[2 ~ 6]
			for i := 0; i < 4; i++ {
				data[2+i] = int32Array[i]
			}
			// Save json in package
			for i := 0; i < len(j); i++ {
				data[6+i] = j[i]
			}
			conn.Write(data)
			// Clear data, leave protocol signs
			for i := 2; i < 1023; i++ {
				data[i] = 0
			}

		}
		//log.Println( cust.Company_name.String)	//sql.NullString need to be convert to string

	} // if strCmd == GET_CALL_LOG

	// End of transmission
	data[1022] = END_OF_TRANSMISSION // ASCII 0x04 is flag for End of Transmission
	conn.Write(data)
}

func initializeMySql() {
	// Initialize MySql pool
	var err error
	// root:your_password_here@tcp(localhost:3306)
	db, err = sql.Open("mysql", "root:@tcp(localhost:3306)/modem?charset=utf8")
	checkError(err, "MySQL connection pool error: ")
	err = db.Ping() // test database connection
	if err == nil {
		dbPingOk = true
	} else {
		checkError(err, "Ping MySQL host error: ")
	}

}

func initializeSqlite() {
	// Initialize Sqlite
	// Check and create the app folder
	_, err := os.Stat("/usr/lib/cider")
	checkError(err)
	if os.IsNotExist(err) {
		log.Println(err)
		checkError(os.MkdirAll("/usr/lib/cider", os.ModePerm))
	}
	sqliteDB, err = sql.Open("sqlite3", "/usr/lib/cider/cider.db")
	if err != nil {
		checkError(err, "Sqlite open database err")
	}

	// Create tables if it's a brand new database
	stmt, err := sqliteDB.Prepare(`CREATE TABLE IF NOT EXISTS call_log (
		  id  integer primary key autoincrement,
		  phone_line varchar(25) DEFAULT NULL,
		  date char(4) DEFAULT NULL,
		  time char(4) DEFAULT NULL,
		  nmbr varchar(25) DEFAULT NULL,
		  name varchar(25) DEFAULT NULL,
		  customer_record  int(11),
		  last_invoice_date varchar(25)  DEFAULT NULL,
	      company_name      varchar(50)  DEFAULT NULL,
	      address           varchar(50) DEFAULT NULL,
	      city              varchar(50)  DEFAULT NULL,
	      state             varchar(25)  DEFAULT NULL,
		  custom_col1  varchar(50) DEFAULT NULL,
		  custom_col2  varchar(50) DEFAULT NULL,
		  custom_col3  varchar(50) DEFAULT NULL,
		  custom_col4  varchar(50) DEFAULT NULL,
		  custom_col5  varchar(50) DEFAULT NULL,
		  time_stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
		)`)
	checkError(err)
	stmt.Exec()

	stmt, err = sqliteDB.Prepare(`CREATE TABLE IF NOT EXISTS customers (
	  customer_record integer primary key,
	  last_invoice_date varchar(25) DEFAULT NULL,
	  company_name varchar(50) DEFAULT NULL,
	  address varchar(50) DEFAULT NULL,
	  city varchar(25) DEFAULT NULL,
	  state varchar(25) DEFAULT NULL,
	  telephone1 varchar(25) DEFAULT NULL,
	  telephone2 varchar(25) DEFAULT NULL,
	  id integer DEFAULT NULL
	)`)
	checkError(err)
	stmt.Exec()
}

func main() {
	log.Println("")
	log.Println("******************************************************************************")
	log.Println("The Cidmon App is starting... ")

	// Init MySql
	initializeMySql()
	if db != nil { //db is a global varable
		defer db.Close()
	}

	// Init Sqlite. KNOWN PROBLEM: sqlite is very low performance on SD card
	initializeSqlite()
	if sqliteDB != nil { //sqliteDB is a global varable
		defer sqliteDB.Close()
	}

	go initializeModems()

	// Run UDP server, and wait for client registration
	go udpRegisterListenThread()

	// Run TCP server, waiting for clients request call_log
	go queryListenServerThread()

	// Keep system runing on background and non stop
	for {
		time.Sleep(1 * time.Hour)
	}
}

/*
Custome protocol

| 0   |  1   | 2  | 3  |  4 | 5  | .......| 14  |                                      |1022 |1023|
|_____|______|____|____|____|____|________|_____|______________________________________|_____|____|
 0x01   0xab { UInt32: json size}                                                       0x04  0x0d
 SOH     ACK { Data ...                                                   data ....    } EOT   CR
             { QUERY CMD, EXAMPLE: GET_CALL_LOG }


 0x01 Start of Header
 0xab
 0x04 End of Transmission
 0x0d Carriage Return  (A package end, return to next one)


*/
/*
Caller ID sample:

DATE = 0427

TIME = 0002

NMBR = 12121234567	   #some modem will get phone# with initial 1, some doesn't

NAME = 12121234567

*/

/*
sqlite> .schema customers
CREATE TABLE customers (
          customer_record integer primary key,
          last_invoice_date varchar(25) DEFAULT NULL,
          company_name varchar(50) DEFAULT NULL,
          address varchar(50) DEFAULT NULL,
          city varchar(25) DEFAULT NULL,
          state varchar(25) DEFAULT NULL,
          telephone1 varchar(25) DEFAULT NULL,
          telephone2 varchar(25) DEFAULT NULL
        );

sqlite> .schema call_log
CREATE TABLE call_log (
                  id  integer primary key autoincrement,
                  phone_line varchar(25) DEFAULT NULL,
                  date char(4) DEFAULT NULL,
                  time char(4) DEFAULT NULL,
                  nmbr varchar(25) DEFAULT NULL,
                  name varchar(25) DEFAULT NULL,
                  customer_record  int(11),
                  last_invoice_date varchar(25)  DEFAULT NULL,
	              company_name      varchar(50)  DEFAULT NULL,
	              address           varchar(50)  DEFAULT NULL,
	              city              varchar(50)  DEFAULT NULL,
	              state             varchar(25)  DEFAULT NULL,
                  custom_col1  varchar(50) DEFAULT NULL,
                  custom_col2  varchar(50) DEFAULT NULL,
                  custom_col3  varchar(50) DEFAULT NULL,
                  custom_col4  varchar(50) DEFAULT NULL,
                  custom_col5  varchar(50) DEFAULT NULL,
                  time_stamp timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
                );

*/
