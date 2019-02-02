using System;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Newtonsoft.Json.Linq;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;

namespace cider
{
	public partial class MainForm : Form
	{
		public readonly static int tcpServerPort = 5005;
		public readonly static int tcpClientPort = 5005;
		public readonly static int udpServerPort = 5001;
		public readonly static int udpListenPort = 5002;

		// Definition command for TCP Protocal
		public readonly static string GET_CALL_LOG = "GET_CALL_LOG";


		// A Struct for save each record and retrieve
		public class CID_Record
		{

			public String id = "";
			public String phoneLine = "";
			public String date = "";
			public String time = "";
			public String nmbr = "";
			public String name = "";
			public String customerRecord = "";
			public String lastInvoice = "";
			public String customerName = "";
			public String address = "";
			public String city = "";
			public String state = "";

			public String customCol1 = "";
			public String customCol2 = "";
			public String customCol3 = "";
			public String customCol4 = "";
			public String customCol5 = "";

		}

		public static readonly int maxRecordQty = 50;
		public static CID_Record[] cid_records = new CID_Record[maxRecordQty];
		public static int currentDisplayIndex = -1;  //Current displaying record
		public static int currentRecordIndex = 0;
		public static bool bIsFull = false; //cidrecords is full, then override
		public static bool bIsExpanded = false;
		public static bool bUdpNotRespon = false;
		public static string serverIP = "";

		// A protector for cidrecords
		private static Mutex mut = new Mutex();

		// Update UIs
		public delegate void Notifier(int currentIndex);  //delegate for UIUpdater_MainForm(); //类似定义指针
		public Notifier notifier;   //

		// Update UI for udp timeout
		public delegate void UdpTimeoutUpdater(bool isTimeout);
		public UdpTimeoutUpdater udpTimeroutUpdater;

		// Update UI for flashing text
		public delegate void FlashingTextUpdater(string lbText);
		public FlashingTextUpdater flashingTextUpdater;

		// Update UI for ListView(call_log)
		public delegate void CallLogUpdater(string strOperation, CID_Record cid);
		public CallLogUpdater callLogUpdater;

		static bool isExsting;

		public MainForm()
		{
			InitializeComponent();
		}

		private void MainForm_Load(object sender, EventArgs e)
		{
			// Set initial form height
			this.Height = 178;
			this.Width = 579;
			this.Location = new System.Drawing.Point(Screen.PrimaryScreen.WorkingArea.Width / 2 - 360, 260);

			// Start register socket
			Thread udpRegisterThread = new Thread(UdpRegisterThread);
			udpRegisterThread.IsBackground = true;
			udpRegisterThread.Start();

			// UDP listener for checking if server online
			Thread udplisteningThread = new Thread(UdpListeningThread);
			udplisteningThread.IsBackground = true;
			udplisteningThread.Start();

			// Start CID receive socket
			Thread socketServerThread = new Thread(TCPListenThread);
			socketServerThread.IsBackground = true;
			socketServerThread.Start();

			// Start flashing text thread
			Thread flashingTextThread = new Thread(FlashingTextThread);
			flashingTextThread.IsBackground = true;
			flashingTextThread.Start();

			//Update UI delegate
			notifier = new Notifier(UIUpdater_MainForm);

			//UPdate UI delegate for UDP timerout
			udpTimeroutUpdater = new UdpTimeoutUpdater(UIUpdater_UDPTimeout);

			// Update UI delegate for flashing text
			flashingTextUpdater = new FlashingTextUpdater(UIUpdater_flashingText);

			// Update UI delegate for ListView(call_log)
			callLogUpdater = new CallLogUpdater(UIUpdater_CallLog);

			//A flag to determine is the close button pressed or the exit menu-item clicked
			//only exit menu-item selected will be really exit the application
			isExsting = false;
			bUdpNotRespon = false;
			//Label initialize 
			lbNumber.Text = "";
			lbName.Text = "";
			lbTime.Text = "";
			lbUdpTimeout.Text = "";

			Console.WriteLine("Form Width: " + this.Width + ", height:"+ this.Height);
			Console.WriteLine("ListView Width: " + this.listView1.Width + ", height:"+ this.listView1.Height);

		}


		private void exitMenuItem_Click(object sender, EventArgs e)
		{
			notifyIcon1.Visible = false;
			this.Close();
			this.Dispose();
			Application.Exit();
		}
		private void showMenuItem_Click(object sender, EventArgs e)
		{
			this.Show();                                
			this.WindowState = FormWindowState.Normal;    
			this.Activate();
		}

		/* Register this client on Resbperry Pi server to receive a CID notification */
		public void UdpRegisterThread()
		{
			while (true)
			{
				try
				{
					UdpClient udpClient = new UdpClient();
					//udpClient.Client.ReceiveTimeout = 15000;
					IPEndPoint ipEndPoint = null;
					String strBroadcastIP = GetBroadcastIP();
					if (strBroadcastIP.Equals("")) return;

					/* Send udp pack */
					ipEndPoint = new IPEndPoint(IPAddress.Parse(strBroadcastIP), udpServerPort);
					udpClient.Connect(ipEndPoint);
					byte[] udpPack = new byte[1024];
					udpPack[0] = 0x01;          //start_byte
					udpPack[1] = 0xab;          //flag_bype
					udpPack[2] = (int)'O';
					udpPack[3] = (int)'K';
					udpPack[4] = (int)'!';
					udpPack[1023] = 0x0d;       //end_byte

					udpClient.Send(udpPack, udpPack.Length);
					udpClient.Close();
				}
				catch (SocketException ex)
				{
					Console.WriteLine(ex.ToString());
				}

				Thread.Sleep(1000 * 60 * 1);    //Register every 1 minute
			}
		}

		// Waiting server response every 1 minutes
		public void UdpListeningThread()
		{
			while (true)
			{
				UdpClient udpListenClient = new UdpClient(udpListenPort);
				try
				{

					IPEndPoint ipEndPoint = new IPEndPoint(IPAddress.Any, 0);
					udpListenClient.Client.ReceiveTimeout = 60*1000;
					Byte[] receivedBytes = udpListenClient.Receive(ref ipEndPoint);
					if (receivedBytes.Length == 1024)
					{
						if (receivedBytes[0] == 0x01 &&
								receivedBytes[1] == 0xab &&
								receivedBytes[2] == (int)'O' &&
								receivedBytes[3] == (int)'K' &&
								receivedBytes[4] == (int)'!' &&
								receivedBytes[1023] == 0x0d
								)
						{
							Console.WriteLine("Received confirmation from server!");
							bUdpNotRespon = false;
							serverIP = ipEndPoint.Address.ToString();
							Console.WriteLine("Server IP: " + serverIP);
							this.BeginInvoke(udpTimeroutUpdater, false); // false, no time out
						}
					}

				}
				catch (SocketException ex)
				{
					Console.WriteLine(ex.ToString());
					bUdpNotRespon = true;
					this.BeginInvoke(udpTimeroutUpdater, true); // true, is time out

				}
				udpListenClient.Close();
				//Thread.Sleep(1000 * 60 * 1);    //Check connection every 1 minute (used udp timeout insteed)
			}

		}

		public void TCPListenThread()
		{
			IPEndPoint ip = new IPEndPoint(IPAddress.Parse(GetLocalIP()), tcpServerPort);
			Socket svrSock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			svrSock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
			svrSock.Bind(ip);
			svrSock.Listen(20);

			while (true)
			{
				try
				{
					Socket clientSock = svrSock.Accept();
					Thread socketThread = new Thread(new ParameterizedThreadStart(TCPConnThread));
					socketThread.IsBackground = true;   // Exit thread before exit maid process
					socketThread.Start(clientSock);
				}
				catch (SocketException ex)
				{
					MessageBox.Show(ex.ToString());
				}
			}
		}

		public void TCPConnThread(object obj)
		{
			Socket clientSock = obj as Socket;
			int length = 1024;
			byte[] protocal_head = new byte[length];

			int offset = 0;
			int bytesRead = 0;

			try
			{
				while ((bytesRead = clientSock.Receive(protocal_head, offset, length, SocketFlags.None)) != 0)
				{
					if (bytesRead == length)
					{
						break;
					}
					length = length - bytesRead;
					offset = offset + bytesRead;
				}

				if (protocal_head[0] != 0x01 ||
						protocal_head[1] != 0xab ||
						protocal_head[1023] != 0x0d
						)
				{
					clientSock.Close();
					return;
				}

				int data_size = protocal_head[2] * 1024;
				byte[] data_buffer = new byte[data_size];   //how many data package will be needed
																										//MessageBox.Show("Data size:" + data_size.ToString());
				length = data_size;
				offset = 0;
				bytesRead = 0;

				while ((bytesRead = clientSock.Receive(data_buffer, offset, length, SocketFlags.None)) != 0)
				{
					if (bytesRead == length)
					{
						break;
					}
					length = length - bytesRead;
					offset = offset + bytesRead;
				}
				/*
				 * Sample:
				 * {"address":"","city":"","company_name":"","last_invoice_date":"","phone_line":"1","name":"KEVIN HAUNG","nmbr":"134700000000","state":""}
				 */
				String _strJson = Encoding.ASCII.GetString(data_buffer, 0, data_size);
				//MessageBox.Show(_strJson);
				dynamic JSonObj = JObject.Parse(_strJson);
				Console.WriteLine("call in: "+JSonObj);
				String strNmbr = (String)JSonObj["nmbr"];
				strNmbr = strNmbr.Trim();
				switch (strNmbr.Length)
				{
					case 10:
						strNmbr = strNmbr.Insert(6, "-").Insert(3, "-");
						break;

					case 11:
						strNmbr = strNmbr.Substring(1).Insert(6, "-").Insert(3, "-");
						break;

					default:
						break;
				}

				// For record storage
				CID_Record cid = new CID_Record();
				cid.id = "    *";
				String strName = (String)JSonObj["name"];
				cid.name = strName.Trim();
				cid.nmbr = strNmbr;
				cid.customerName = (String)JSonObj["company_name"];
				cid.address = (String)JSonObj["address"];
				cid.city = (String)JSonObj["city"];
				cid.state = (String)JSonObj["state"];
				cid.lastInvoice = (String)JSonObj["last_invoice_date"];
				cid.date = (String)JSonObj["date"]; // DateTime.Now.ToString("MMM dd,");
				cid.time = (String)JSonObj["time"]; // DateTime.Now.ToString("hh:mm tt");

				if(cid.date.Length == 4)
				{
					cid.date = cid.date.Insert(2, "-");
				}

				if(cid.time.Length == 4)
				{
					cid.time = cid.time.Insert(2, ":");
				}

                if(cid.nmbr.Equals("P"))
                {
                    cid.nmbr = "Private";
                }

                if(cid.name.Equals("O"))
                {
                    cid.name = "Out of range";
                }
				mut.WaitOne();
				if (currentRecordIndex >= maxRecordQty)
				{
					currentRecordIndex = 0;
					bIsFull = true;
				}
				cid_records[currentRecordIndex] = cid;
				this.BeginInvoke(notifier, currentRecordIndex);
				currentRecordIndex++;
				mut.ReleaseMutex();

				this.BeginInvoke(callLogUpdater, "insert_item", cid);

				clientSock.Close();
				return;
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.ToString());
				clientSock.Close();
			}
		}

		public void TCPQueryThread(string CMD)
		{
			if (serverIP.Equals("")) return;
			TcpClient tcpClient = new TcpClient(serverIP, tcpServerPort);
			NetworkStream stream = tcpClient.GetStream();
			byte[] protocal_cmd = new byte[1024];
			byte[] cmd = System.Text.Encoding.ASCII.GetBytes(CMD); //CMD = "GET_CALL_LOG"
																														 // Build protocal
			protocal_cmd[0] = 0x01;          //start_byte
			protocal_cmd[1] = 0xab;          //flag_bype
			protocal_cmd[1023] = 0x0d;       //end_byte

			if (CMD.Equals(GET_CALL_LOG))
			{
				this.BeginInvoke(callLogUpdater, "clear", null);
				try
				{
					// Command body, from protocal[2]...
					for (var i = 0; i < cmd.Length; i++)
					{
						protocal_cmd[i + 2] = cmd[i];
					}
					// Send protoal_head
					stream.Write(protocal_cmd, 0, protocal_cmd.Length);
					Console.WriteLine("TCP GET_CALL_LOG sent...");

					// Read response
					Byte[] data = new Byte[1024];
					Int32 bytesRead = 0;
					Int32 offset = 0;
					Int32 bytesRemain = 1024;
					// Receive json packages, each sized 1024 bytes
					while (true)
					{
						while (true)
						{
							// Try to read the specified length of data
							bytesRead = stream.Read(data, offset, bytesRemain);
							if (bytesRead == bytesRemain) break;
							bytesRemain = bytesRemain - bytesRead;
							offset = offset + bytesRead;
						}
						if (data[0] != 0x01 || data[1] != 0xab || data[1023] != 0x0d
							 || data[1022] == 0x04)  // ASCII 0x04 End of Transmission
						{
							stream.Close();
							tcpClient.Close();
							Console.WriteLine("End of transmission.");
							return;  // Invalid data
						}
						// Verify the json size
						UInt32 jsonSize;
						Byte[] intArray = new Byte[4];
						for (var i = 0; i < 4; i++)
						{
							intArray[i] = data[2 + i]; //data[2] ... data[4] stores json length
						}
						// ARM is big endian, Most x86 architecture CPU is Little Endian, but converted by golang
						//if (BitConverter.IsLittleEndian)
						//   Array.Reverse(intArray);
						jsonSize = BitConverter.ToUInt32(intArray, 0);
						Console.WriteLine("Received json size:" + jsonSize.ToString());

						String strJson = Encoding.ASCII.GetString(data, 6, Convert.ToInt32(jsonSize));
						Console.WriteLine(strJson);
						dynamic JSonObj = JObject.Parse(strJson);
						CID_Record cid = new CID_Record();
						cid.id = (String)JSonObj["id"];
						cid.phoneLine = (String)JSonObj["line"];
						cid.date = (String)JSonObj["date"];
						cid.time = (String)JSonObj["time"];
						cid.nmbr = (String)JSonObj["nmbr"];
						cid.name = (String)JSonObj["name"];
						cid.customerRecord = (String)JSonObj["customer_record"];
						cid.lastInvoice = (String)JSonObj["last_invoice_date"];
						cid.customerName = (String)JSonObj["company_name"];
						cid.address = (String)JSonObj["address"];
						cid.city = (String)JSonObj["city"];
						cid.state = (String)JSonObj["state"];
						cid.customCol1 = (String)JSonObj["custom_col1"];
						cid.customCol2 = (String)JSonObj["custom_col2"];
						cid.customCol3 = (String)JSonObj["custom_col3"];
						cid.customCol4 = (String)JSonObj["custom_col4"];
						cid.customCol5 = (String)JSonObj["custom_col5"];

						// Formating string
						if(cid.date.Length == 4)
						{
							cid.date = cid.date.Insert(2, "-");
						}
						if(cid.time.Length == 4)
						{
							cid.time = cid.time.Insert(2, ":");
						}
						if(cid.nmbr.Length == 10)
						{
							cid.nmbr = cid.nmbr.Insert(6, "-").Insert(3, "-");
						}

						Console.WriteLine(cid.nmbr);
						this.BeginInvoke(callLogUpdater,"add_item", cid);
						// String responseData = System.Text.Encoding.ASCII.GetString(data, 0, bytes);
						//Console.WriteLine("Received: {0}", responseData);
					}

				}
				catch (SocketException ex)
				{
					Console.WriteLine(ex.ToString());
				}
			}

			stream.Close();
			tcpClient.Close();
		}

		public void FlashingTextThread()
		{
			while (true)
			{
				if (bUdpNotRespon)
				{
					this.BeginInvoke(flashingTextUpdater, "");
					Thread.Sleep(750);
					if (bUdpNotRespon)
					{
						this.BeginInvoke(flashingTextUpdater, "Waiting for server...");
					}
					Thread.Sleep(1000);
				}
				else
				{
					Thread.Sleep(1000*60);
				}
			}
		}

		public void UIUpdater_flashingText(string lbText)
		{
			mut.WaitOne();
			lbUdpTimeout.Text = lbText;
			mut.ReleaseMutex();

		}

		public void UIUpdater_MainForm(int recordIndex)
		{
			currentDisplayIndex = recordIndex;
			mut.WaitOne();
			Console.WriteLine("UIUpdater_MainForm start.., CurrentRec: " + currentDisplayIndex.ToString());
			lbName.Text = cid_records[recordIndex].name;
			lbNumber.Text = cid_records[recordIndex].nmbr;
			lbTime.Text = "#" + (recordIndex + 1).ToString() + "    " + cid_records[recordIndex].date + " " + cid_records[recordIndex].time;
			tbCustomer.Text = cid_records[recordIndex].customerName;
			tbAddress.Text = cid_records[recordIndex].address;
			tbCity.Text = cid_records[recordIndex].city;
			tbState.Text = cid_records[recordIndex].state;
			tbLastInvoice.Text = cid_records[recordIndex].lastInvoice;

			mut.ReleaseMutex();
			Show();                               
			WindowState = FormWindowState.Normal;  
			Activate();
		}

		// Update UI ListView for call_log
		public void UIUpdater_CallLog(String strOperation, CID_Record cid)
		{
			// Clear listview
			if (strOperation.Equals("clear"))
			{
				mut.WaitOne();
				listView1.Items.Clear();
				mut.ReleaseMutex();
				return;
			}

			ListViewItem lvi = new ListViewItem();
			var item = new ListViewItem(new[] {
					cid.id,
					cid.date,
					cid.time,
					cid.nmbr,
					cid.name,
					cid.customerName,
					cid.address,
					cid.city,
					cid.state,
				});

			if (strOperation.Equals("add_item"))
			{
				mut.WaitOne();
				listView1.Items.Add(item);
				mut.ReleaseMutex();
			}
			
			if(strOperation.Equals("insert_item"))
			{
				mut.WaitOne();
				listView1.Items.Insert(0,item);
				mut.ReleaseMutex();
			}

		}

		// Update UI for UDP time out
		public void UIUpdater_UDPTimeout(bool isTimeout)
		{
			mut.WaitOne();
			if (isTimeout)
			{
				if (BackColor != Color.Orange)
				{
					BackColor = Color.Orange;
					lbUdpTimeout.Text = "Waiting for server...";
				}

			}
			else
			{
				BackColor = Color.YellowGreen;
				lbUdpTimeout.Text = "";

			}
			mut.ReleaseMutex();
		}

		private void btPreRecord_Click(object sender, EventArgs e)
		{
			mut.WaitOne();

			if (currentDisplayIndex <= 0 && !bIsFull)
			{
				mut.ReleaseMutex();
				return; //Already at first record
			}

			if (currentDisplayIndex <= 0) currentDisplayIndex = maxRecordQty;
			currentDisplayIndex--;
			UIUpdater_MainForm(currentDisplayIndex);
			mut.ReleaseMutex();
		}

		private void btNextRecord_Click(object sender, EventArgs e)
		{
			mut.WaitOne();
			if (currentDisplayIndex >= currentRecordIndex - 1 && !bIsFull)
			{
				mut.ReleaseMutex();
				return; //Already at last record
			}
			currentDisplayIndex++;
			if (currentDisplayIndex >= maxRecordQty) currentDisplayIndex = 0;
			UIUpdater_MainForm(currentDisplayIndex);
			mut.ReleaseMutex();
		}

		/* Cumstom Functions  */
		public static string GetGatewayIP()
		{
			NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
			foreach (NetworkInterface adapter in nics)
			{
				IPInterfaceProperties ip = adapter.GetIPProperties();
				GatewayIPAddressInformationCollection gateways = ip.GatewayAddresses;
				foreach (var gateWay in gateways)
				{
					String strGatewayIP = gateWay.Address.ToString();
					if (strGatewayIP.IndexOf(':') == -1)
					{
						return strGatewayIP;
					}
				}

			}

			return "";
		}

		public static string GetLocalIP()
		{
			var host = Dns.GetHostEntry(Dns.GetHostName());
			var prefix_getewayIP = GetGatewayIP().Remove(GetGatewayIP().LastIndexOf('.') + 1);
			var localIP = "";
			foreach (var ip in host.AddressList)
			{
				if (ip.AddressFamily == AddressFamily.InterNetwork)
				{
					localIP = ip.ToString();
					// if gateway ip and nic ip are in the same ip range. Some nic may have multi-ip
					if (localIP.Remove(localIP.LastIndexOf('.') + 1) == prefix_getewayIP)
					{
						return ip.ToString();
					}
				}
			}
			return "";
		}

		public static string GetBroadcastIP()
		{
			String localIP = GetLocalIP();
			if (localIP != "")
			{
				return localIP.Remove(localIP.LastIndexOf('.') + 1) + "255";
			}
			else
			{
				return "";
			}

		}

		private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
		{
			if (!isExsting)
			{
				e.Cancel = true;
				//this.WindowState = FormWindowState.Minimized;
				Hide();
			}

		}

		private void notifyIcon1_MouseClick(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Left)  
			{
				/*
				Type t = typeof(NotifyIcon);
				MethodInfo mi = t.GetMethod("ShowContextMenu", BindingFlags.NonPublic | BindingFlags.Instance);
				mi.Invoke(this.notifyIcon1, null);
				*/
				Show();
				WindowState = FormWindowState.Normal;
				Activate();
			}
		}

		private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
		{
			AboutBox1 aboutDlg = new AboutBox1();
			aboutDlg.ShowDialog();
		}



		private void btClose_Click(object sender, EventArgs e)
		{
			Hide();
		}

		private void openMenuItem_Click(object sender, EventArgs e)
		{
			Process.Start("http://localhost:88");   // Show calllog on a local web server
		}

		private void btnLog_Click(object sender, EventArgs e)
		{
			if (bIsExpanded == false)
			{
				this.Height += 400;
				this.Width += 200;
				btnLog.Text = "Close &Log <<<";
				bIsExpanded = true;

				Thread tcpQueryThread = new Thread(() => TCPQueryThread(GET_CALL_LOG));
				tcpQueryThread.IsBackground = true;
				tcpQueryThread.Start();

			}
			else
			{
				this.Height -= 400;
				this.Width -= 200;
				btnLog.Text = "Show &Log >>>";
				bIsExpanded = false;
			}


		}

	}


}
