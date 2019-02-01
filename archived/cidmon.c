/*
 * modem caller ID monitoring
 * platform - Rasbian
 * c progrmming by Qien Huang 04-2016 Brooklyn, NY. 
 * Email: kevin11206@gmail.com
 * gcc callmon.c -o callmon -pthread `pkg-config --cflags --libs glib-2.0` `mysql_config --cflags --libs` 
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <glib.h>
#include <mysql.h>

#define UART_SPEED B115200
#define SERIALPATH  "/dev/serial/by-path/"
#define REV_BUF_LEN  1024
#define MAX_LINE    16

#define TCPSERVERPORT  5005
#define TCPCLIENTPORT  5005
#define UDPSERVERPORT  5001
#define UDPCLIENTPORT  5002

/* Definitions for modem */
char modem[MAX_LINE][255];
int modem_fd[MAX_LINE];     // modem file descriptor
int  modem_qty = 0;   //Qty of modems
pthread_t line_thread[MAX_LINE];
pthread_t udpserver_thread;

/* Definitions for Socket */
int udpserver_fd = 0;
GList* client_addr = NULL;  //unsigned long

/* Caller ID */
struct CID {
	char cid_line[4];
	char cid_date[10];  /* date/time string is 4 bytes, must define the array longer the 4, otherwise will cut the \0 end */
	char cid_time[10];
	char cid_nmbr[15];
	char cid_name[50];
};

/* args for pthread_create() */
typedef struct  tag_parameter   /* c doesn't support typedef with tag */
{
	char* ip_address;
	char* json_string;
} struct_parameter;

/*  custom print function, Since the printf doesn't work, cache issue */
void print_buf(char *buffer, int buffer_size)
{
	int i;
	for (i = 0; i < buffer_size; i++)
	{
		//printf(" %c-%d", buffer[i], buffer[i]);
		printf("%c", buffer[i]);
	}
}


/* Enumrates modems */
void enum_dev()
{
	DIR  *d;
	struct dirent  *dir;

	d = opendir(SERIALPATH);	//"/dev/serial/by-path/"
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			// only list devices, DT_LNK = 10
			if (dir->d_type == DT_LNK)
			{
				strcpy(modem[modem_qty], SERIALPATH);
				strcat(modem[modem_qty], dir->d_name);	/* string append */
				printf("Found the modem:\n");
				printf("%s\n", modem[modem_qty]);
				modem_qty++;
			}
		}
		closedir(d);
	}
}

/* Initializing serial port */
void init_serial(int fd)
{
	struct termios termios;
	int res;

	res = tcgetattr(fd, &termios);
	if (res < 0) {
		fprintf(stdout, "Termios get error: %s\n", strerror(errno));
		exit(-1);
	}

	cfsetispeed(&termios, UART_SPEED);
	cfsetospeed(&termios, UART_SPEED);

	termios.c_iflag &= ~(IGNPAR | IXON | IXOFF);
	termios.c_iflag |= IGNPAR;

	termios.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CREAD | CLOCAL);
	termios.c_cflag |= CS8;
	termios.c_cflag |= CREAD;
	termios.c_cflag |= CLOCAL;

	termios.c_lflag &= ~(ICANON | ECHO);
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	res = tcsetattr(fd, TCSANOW, &termios);
	if (res < 0) {
		fprintf(stdout, "Termios set error: %s\n", strerror(errno));
		exit(-1);
	}
}

/* initializing modem to receive caller id */
int init_modem(int fd)
{
	int _fd = fd;
	int res;
	char buf[REV_BUF_LEN];

	////// Reset modem /////////
	//MODEM commands you must be ended with a carriage return (CR) and without a newline (NL). 
	//The C character constant for CR is "\r"
	res = write(_fd, "\rATZ\r", sizeof("\rATZ\r"));
	if (res < 0) {
		fprintf(stdout, "Sending ATZ command error: %s\n", strerror(errno));
		return -1;
	}
	//sleep(1);

	res = read(_fd, buf, REV_BUF_LEN);
	if (res < 0) {
		fprintf(stdout, "ATZ command return error: %s\n", strerror(errno));
		return -1;
	}
	print_buf(buf,res);

	sleep(1);
	////// Enable caller ID detection /////////
	res = write(_fd, "\rAT+VCID=1\r", sizeof("\rAT+VCID=1\r"));
	if (res < 0) {
		fprintf(stdout, "Sending AT+VCID=1 command error: %s\n", strerror(errno));
		return -1;
	}
	//sleep(1);
	res = read(_fd, buf, REV_BUF_LEN);
	//printf ("%d\n", res);
	if (res < 0) {
		fprintf(stdout, "AT+VCID=1 command return error: %s\n", strerror(errno));
		return -1;
	}
	print_buf(buf,res);

	return 1;
}

void database_query(char* jsonstr, char* str_line, char* str_date, char* str_time, char* str_number, char* str_name)
{
	/*
	// sample data
	char str_line[] = "2";
	char str_date[] = "0504";
	char str_time[] = "0959";
	char str_number[] = "347000000";
	char str_name[] = "KEVIN HUANG";
	*/	
	char str_sql_insert[255];	// insert a row to call_log table
	sprintf(
		str_sql_insert, 
		"INSERT INTO call_log (phone_line, date,time,nmbr,name) values ('%s','%s','%s','%s','%s' )", 
		str_line, str_date, str_time, str_number, str_name 	);
	printf(str_sql_insert);
	printf("\n");

	char str_sql_query[255];	// query from customers
	sprintf( 
		str_sql_query, 
		"select last_invoice_date, company_name, address, city, state \
		from customers where telephone1='%s'  \
		or telephone2='%s' limit 1",
		str_number, str_number	);

	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *server = "localhost";
	char *user = "root";
	char *password = "";
	char *database = "modem";
	conn = mysql_init(NULL);

	if(!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0 ))
	{
		fprintf(stderr, "%s\n", mysql_error(conn));
		return;
	}

	if(mysql_query(conn, str_sql_insert ))
	{
		fprintf(stderr, "%s\n", mysql_error(conn));
		return;
	}

	printf(str_sql_query);

	if(mysql_query(conn, str_sql_query ))
	{
		fprintf(stderr, "%s\n", mysql_error(conn));
		return;
	}

	printf("\n\n");
	res = mysql_use_result(conn);
	char sql_temp[] = "{\"line\":\"%s\",\"date\":\"%s\",\"time\":\"%s\",\"nmbr\":\"%s\",\"name\":\"%s\",\"last_invoice_date\":\"%s\",\"company_name\":\"%s\",\"address\":\"%s\",\"city\":\"%s\",\"state\":\"%s\"}";
	if((row = mysql_fetch_row(res)))
	{		
		//printf("%s %s %s %s ",row[0] ,row[1] ,row[2] ,row[3]);
		sprintf(jsonstr, sql_temp,
		str_line, str_date, str_time, str_number, str_name, row[0], row[1], row[2], row[3], row[4] );
	}
	else
	{
		sprintf(jsonstr, sql_temp,
		str_line, str_date, str_time, str_number, str_name, "", "", "", "", "" );
	}
	printf("\njson string: %s\n", jsonstr);

	mysql_free_result(res);
	mysql_close(conn);
}

/* create individul thread for each client socket */
void* send_socket_thread(void* p)
{
	fprintf(stdout, "client socket started ... ...\n");
	int sock;
	if(0>(sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
		perror("socket create error");      

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(TCPSERVERPORT);   /* The receiver client */

	struct_parameter *pargs = p;
	printf("From send_socket_thread, the ip is: %s\n",  (char*)(pargs->ip_address));
	//printf("From send_socket_thread, the json is: %s\n", (char*)(pargs->json_string ));
	servaddr.sin_addr.s_addr = inet_addr( (char*)(pargs->ip_address) ); 

	if(0> connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)))
	{
		perror("tcp socket connect error");

		/* remove this ip address from the list */
		GList *lst = NULL;
		for (lst = client_addr; lst; lst = lst->next) {
			if(0 == strcmp((char*)(pargs->ip_address),(char*)lst->data))
			{
				client_addr = g_list_remove(client_addr, lst->data);
				printf("IP address: %s is removed!", (char*)(pargs->ip_address));
			}           
		}
	   
	}

	/* custom protocal, header packing */
	char header_buf[1024];
	header_buf[0] = 0x01;
	header_buf[1] = 0xab;
	header_buf[1023] = 0x0d;

	/* sending data content */
	char jsonstr_buff[1024];
	strcpy(jsonstr_buff,(char*)(pargs->json_string ));
	printf("json is: %s\n",jsonstr_buff);
	write(sock, header_buf, sizeof(header_buf));
	write(sock, jsonstr_buff, strlen(jsonstr_buff));    /* only send jsonstr length, not entire 1024 bytes*/
	close(sock);
	free(p);
	pthread_detach(pthread_self());
	return 0;
}

/* TCP sender, dispatch tcp socket */
void* sender_thread(void* p)
{
	fprintf(stderr,"TCP sender is running ... ...\n");
	//fprintf(stdout,"%s\n",(char*)p);
	GList *lst = NULL;
	pthread_t psocket_thread;
	struct tag_parameter* pargs[255];
	int i=0;
	for (lst = client_addr; lst; lst = lst->next) 
	{
		printf("ip: %s\n", (char*)lst->data);
		pargs[i] = malloc(sizeof(struct_parameter));
		pargs[i]->ip_address = (char*)lst->data;
		pargs[i]->json_string = (char*)p;
		if(0 != pthread_create(&psocket_thread, NULL, send_socket_thread, (void*)pargs[i]))
		{
			perror("TCP sender error\n");  
		}
		i++;
	}
	//pthread_detach(pthread_self());
	return 0;
}

/* Stand-alone phone line cid monitoring thread */
void* phone_line_thread(void* p)
{
	struct CID cid;
	int i = *(int*)p;   /* get line id */
	sprintf(cid.cid_line, "%d", i+1);   

	sleep(i+2);
	printf("Phone_line_thread #%d is running ... ...\n", i);
	modem_fd[i] = open(modem[i], O_RDWR | O_NOCTTY);
	if (modem_fd[i] < 0) {
		fprintf(stdout, "Cannot open modem fd %s: %s\n", modem[i], strerror(errno));
		return NULL;
	}

	/* Initial serial port */
	init_serial(modem_fd[i]);

	/* Initial modem */
	init_modem(modem_fd[i]);

	// Monitoring Caller ID
	char buf[REV_BUF_LEN];
	char recv_buf[REV_BUF_LEN];	// for trying to receive all 62 bytes cid
	char jsonstr[512];	// must have enough bytes to store string
	int res=0;	// the received bytes in one-read
	int res_count = 0; // the total received bytes in multi-read. since some modems won't send all cid in one-read
	int j;  // an index for a buffer of one string line
	int k;  // current position in receive-buffer
	int index;  // for check the data start byte, the modem format of data can be varied
	int s=0; //for space trim

	char temp[1024];    // one string line
	pthread_t tcpsend;
	memset(buf, 0, REV_BUF_LEN);
	while (1)   /* keep reading from modem */
	{
		res = read(modem_fd[i], buf, REV_BUF_LEN);
		if (res < 0) 
		{
			fprintf(stdout, "Read error: %s\n", strerror(errno));
			return 0;
		}
		print_buf(buf, res);    /* has cache probelm with printf,fprintf */

		/* some modem can not send all data in one-read */
		if( (strstr(buf,"DATE") !=NULL) && res <62)	// search "DATE", a cid is 62 bytes
		{
			while(res < 62)
			{
				j = read(modem_fd[i], recv_buf, REV_BUF_LEN);
				strncpy(buf+res, recv_buf,j);
				res = res + j;
			}
			res = res+1;
			printf("logged in multi-read :\n");
			print_buf(buf,strlen(buf)+1);
			printf("strlen(buf):%d, res:%d", strlen(buf), res);
		}
		else
		{
			printf("logged in one-read :\n");
			print_buf(buf,strlen(buf));
		}
		printf("\n------\n");


		j =0;
		for(k=0;k<res;k++)      /* pharing all received string */
		{
			if(buf[k]==10 || buf[k]==0) /* check if it's new line or string end \0 */
			{
				if(j != 0)
				{
					// print_buf(temp,j+1);
					// printf("\n");
					if(memcmp(temp,"DATE",4) == 0)
					{
						printf("\ndate string lenght: %d\n",j);						
						index=4;
						while(temp[index] ==32 || temp[index] ==61 )    index++;    // Don't save space or '='

						strncpy(cid.cid_date, temp+index, 4);   //Only copy chars from 7
						cid.cid_date[4] = '\0';     // Set the string end flag
						print_buf(cid.cid_date, 4);
					}

					if(memcmp(temp,"TIME",4) == 0)
					{
						printf("\ntime string lenght: %d\n",j);						
						index=4;
						while(temp[index] ==32 || temp[index] ==61 )    index++;    // Don't save space or '='

						strncpy(cid.cid_time, temp+index, 4);
						cid.cid_time[4] = '\0';     // Set the string end flag
						print_buf(cid.cid_time, 4);
					}

					if(memcmp(temp,"NMBR",4) == 0)
					{
						printf("\nNMBR string lenght: %d\n",j);						
						index=4;
						while(temp[index] ==32 || temp[index] ==61 )    index++;    // Don't save space or '='


						if(temp[index] == '1') index++;		// remove first number '1'
						printf("\nNMBR index: %d\n",index);		

						strncpy(cid.cid_nmbr, temp+index, j-index+1);
						cid.cid_nmbr[j-index+1] = '\0';     // Set the string end flag

						printf("\nNMBR cid.cid_nmbr : %s\n",cid.cid_nmbr);	

						s = strlen(cid.cid_nmbr)-1;   
						// right trim					                     
						while(cid.cid_nmbr[s]== 32 || cid.cid_nmbr[s]=='\0' || cid.cid_nmbr[s]=='\r')
						{
							cid.cid_nmbr[s]= '\0';
							s--;
							if(s<0) break;
						}

						print_buf(cid.cid_nmbr, j-index+1);
					}

					if(memcmp(temp,"NAME",4) == 0)
					{
						printf("\nNAME string lenght: %d\n",j);						
						index=4;
						while(temp[index] ==32 || temp[index] ==61 )    index++;    // Don't save space or '='

						strncpy(cid.cid_name, temp+index, j-index+1);
						cid.cid_name[j-index+1] = '\0';     // Set the string end flag
											   
						s = strlen(cid.cid_name)-1; 
						// right trim                      
						while(cid.cid_name[s]== 32 || cid.cid_name[s]=='\0' || cid.cid_name[s]=='\r')
						{
							cid.cid_name[s]= '\0';
							s--;
							if(s<0) break;
						}					
						
						print_buf(cid.cid_name, j-index+1);
						

						/* update call_log and fetch data */
						 database_query(jsonstr, cid.cid_line, cid.cid_date, cid.cid_time, cid.cid_nmbr, cid.cid_name);

						/* dispatch json to each client */
						if(0 != pthread_create(&tcpsend, NULL, sender_thread, jsonstr))
						{
							perror("Fail to start udpserver_thread, error\n");  
						}

						/* clear */
						cid.cid_date[0] = '\0';
						cid.cid_time[0] = '\0';
						cid.cid_nmbr[0] = '\0';
						cid.cid_name[0] = '\0';
					}
				}

				j =0;

			}
			else
			{
			/* valid data, save it to a string-line buffer*/
				temp[j] = buf[k];
				j++;
			}
		}       

	}

	pthread_detach(pthread_self());
//pthread_exit("I am over !");
}

/* Udp listener thread for Client register */
void* udpserver(void* p)
{
	printf("Udpserver_thread is running ... ...\r\n");

	struct sockaddr_in addr;      /* our address */
	//struct sockaddr_in remoteaddr;      /* our address */
	socklen_t addrlen = sizeof(struct sockaddr_in); //sizeof(addr);    /* length of addresses */
	int recvlen;                    /*  bytes received */
	unsigned char buf[REV_BUF_LEN];     /* receive buffer */
	char* pip;   /*for saving string of an ip address */
	/* create a UDP socket */
		if ((udpserver_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create socket");
			return 0;
		}

	/* bind the socket to any valid IP address and a specified port */
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	addr.sin_port = htons(UDPSERVERPORT);

	if (bind(udpserver_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("binding udpserver failed");
		return 0;
	}

	/* now loop, receiving data and printing what we received */
	GList *lst =NULL;
	int is_new_ip = 1;
	while(1)
	{
		//printf("waiting on port %d\n", UDPSERVERPORT);
		bzero(buf, sizeof(buf));
		recvlen = recvfrom(udpserver_fd, buf, REV_BUF_LEN, 0, (struct sockaddr *)&addr, &addrlen);
		if (recvlen > 0) {

			printf("received from %s \n",inet_ntoa(addr.sin_addr));
			// printf("received %d bytes\n", recvlen);

			/* validate the data package */
			if( buf[0] == 0x01 &&
				buf[1] == 0xab &&
				buf[REV_BUF_LEN-1] == 0x0d)
			{
				//printf("Valid data package!!!\n");
				is_new_ip = 1;
				for (lst = client_addr; lst; lst = lst->next) 
				{
					//printf("data: %s \n", (char*)lst->data);
					if( 0 == strcmp((char*)lst->data, inet_ntoa(addr.sin_addr)))  /* return 0 if they are the same */
					{
						//printf("This ip already in the list\n\n");
						is_new_ip = 0;
						break;
					}
				}

				if(is_new_ip)
				{
					pip = malloc(sizeof(char)*16);
					strcpy(pip, inet_ntoa(addr.sin_addr));
					client_addr = g_list_append(client_addr, pip); 
					printf("New ip added!!!!\n");
				}

			}

		}
	}
	/* never exits */
	pthread_detach(pthread_self());
}

void onExit(int sig)
{
	int i;
	printf("\nPorgram is exiting...\n");

	for(i=0;i<modem_qty;i++)
	{
		pthread_cancel(line_thread[i]);
		if(0 != modem_fd[i])
		{
			close(modem_fd[i]);
			printf("modem#%d fd released!\n", i);
		}

	}

	if(0 != udpserver_fd)
	{
		close(udpserver_fd);
		printf("udpserver fd released!\n");
	}

	sleep(1);
	exit(0);
}

/*  main entry */
int main(int argc, char **argv)
{

	int i;

	/* modem fd array */
	for(i=0;i<MAX_LINE;i++)
	{
		modem_fd[i] = 0;	// zero-init
	}

	/* Catch Ctrl+C interruption and clean up before exit */
	signal(SIGINT, onExit);     /* catch ctrl+c */
	signal(SIGTERM, onExit);    /* catch kill command(termination) */
	signal(SIGPIPE, SIG_IGN);   /* ignores tcp time out exceptions*/

	/* Enumerate modems */
	enum_dev();

	/* Start each modem monitoring thread */
	if(modem_qty>0)
	{
		for(i=0;i<modem_qty;i++)
		{
			if( 0 != pthread_create(&line_thread[i],NULL, phone_line_thread, &i))
			{
				printf("Fail to start phone_thread #%d\n",i);  
				return -1;
			}

			printf("Created phone_thread#%d\n", i);
		}
	}
	else
	{
		printf("No modem found!\n");
		printf("Program exit!\n");
		exit(0);
	}


	/* Start udp listening */
	if(0 != pthread_create(&udpserver_thread, NULL, udpserver, NULL))
	{
		printf("Fail to start udpserver_thread\n");
		exit(0);
	}


	// sleep(5);
	 //    GList *lst = NULL;
	 //    for (lst = client_addr; lst; lst = lst->next) {
	 //        printf("%s\n", lst->data);
	 //        if(0 == strcmp("192.168.1.200",lst->data))
	 //        {
	 //         client_addr = g_list_remove(client_addr, lst->data);
	 //        }
	 //    }

	/* only for testing */
	//client_addr = g_list_append(client_addr,"192.168.1.211");
	//client_addr = g_list_append(client_addr,"192.168.1.221");
	//sleep(5);
	// char jsonstr[1024] = "{\"address\":\"\",\"city\":\"\",\"company_name\":\"\",\"last_invoice_date\":\"\",\"line\":\"1\",\"name\":\"KEVIN HUANG\",\"nmbr\":\"13470000000\",\"state\":\"\"}";
	// pthread_t psender_thread = NULL;
	// if(0 != pthread_create(&psender_thread, NULL, sender_thread, (void*)jsonstr))
	// {
	//  perror("Fail to start udpserver_thread, error\n");  
	// }


	while(1)
	{
		//Keep program running!
		sleep(24*60*60);	// a non-sleep loop will cause high cpu usage
	}
	/* Never stop */
	return 0;
}
