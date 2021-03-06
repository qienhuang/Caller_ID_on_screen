
// ciderDlg.cpp : implementation file
//
/*

   OnInitDialog()
		|
		V
	 AfxBeginThread udpListenerThread
	 AfxBeginThread udpRegisterThread


*/
#include "stdafx.h"
#include "cider.h"
#include "ciderDlg.h"
#include "afxdialogex.h"
#include "afxsock.h"

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PROTOCOL_START 0x01
#define PROTOCOL_ACK   0xab
#define PROTOCOL_EOT   0x04	// Identify for last package
#define PROTOCOL_CR    0x0d // Frame end

#define UDP_REGISTER_PORT 5001
#define UDP_LISTEN_PORT 5002
#define TCP_SERVER_PORT 5005
#define TCP_LISTEN_PORT 5005

#define MAX_CALL_LOG_QUEUE 20

// size between maximum and minimum form size
#define WIDTH_ADJUSTMENT   216
#define HEIGHT_ADJUSTMENT  479

// timer IDs
#define UDP_REGISTER_TIMER 1
#define UDP_TIMEOUT_TIMER 2

// Initialize for static member variable
CArray<CALL_INFO> CciderDlg::CallInfoArray;
CArray<CString> CciderDlg::AdapterIPs;
CArray<CString> CciderDlg::ServerIPs;
int CciderDlg::nCallArrayIndex = 0;

// Global variables
CMutex g_MutexCallInfoArray;
CMutex g_MutexAdapterIPs;
CMutex g_MutexServerIPs;
CMutex g_MutexIsSvrAvailable;
CWinThread *g_pTimeoutUIThread = NULL;
BOOL g_bIsServerAvailable = TRUE;

// Thread pramater
typedef struct THREAD_PARAM
{
	SOCKET* sockConn;
	SOCKADDR_IN* clientAddr;
	CString broadcastIP;

} THREADSTRUCT;

struct PACKAGE_DATA {
	byte START;
	byte ACK;
	byte data[1020] = { 0 };
	byte EOT;
	byte CR;
};

struct PACKAGE_SEND {
	const byte START = PROTOCOL_START;
	const byte ACK = PROTOCOL_ACK;
	byte  data[1020] = { 0 };
	byte  EOT = 0x00;
	const byte CR = PROTOCOL_CR;
};

struct PACKAGE_JSON_SIZE {
	const byte START = PROTOCOL_START;
	const byte ACK = PROTOCOL_ACK;
	byte  jsonSize[4];
	byte  dummyData[1016] = { 0 };
	byte  EOT = 0x00;
	const byte CR = PROTOCOL_CR;
};

struct PACKAGE_JSON_DATA {
	byte  json[1024];
};

struct PACKAGE_QUERY_CMD {
	const byte START = PROTOCOL_START;
	const byte ACK = PROTOCOL_ACK;
	byte  command[12] = { 0 };
	const byte dummyData[1008] = { 0 };
	byte EOT = 0x00;
	const byte CR = PROTOCOL_CR;
};

struct PACKAGE_QUERY_DATA {
	const byte START = PROTOCOL_START;
	const byte ACK = PROTOCOL_ACK;
	byte  jsonSize[4];	
	const byte jsonData[1016] = { 0 };
	byte EOT = 0x00;
	const byte CR = PROTOCOL_CR;
};

struct PACKAGE_LAST {
	const byte START = PROTOCOL_START;
	const byte ACK = PROTOCOL_ACK;
	byte  data[1020] = { 0 };
	byte  EOT = PROTOCOL_EOT;
	const byte CR = PROTOCOL_CR;
};

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CciderDlg dialog


CciderDlg::CciderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_MAIN_DIALOG, pParent)
	, m_IsCallLogShowed(FALSE)
	, m_IsExit(FALSE)
	, m_staticIndex(_T(""))
	, m_staticDate(_T(""))
	, m_staticTime(_T(""))
	, m_editCustomer(_T(""))
	, m_editAddress(_T(""))
	, m_editCity(_T(""))
	, m_editState(_T(""))
	, m_editLastInvoice(_T(""))
	, m_staticLine(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	nCallArrayIndex = 0;
}

void CciderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_NMBR, m_staticNmbr);
	DDX_Control(pDX, IDC_STATIC_NAME, m_staticName);
	DDX_Control(pDX, IDC_LIST1, m_listCtrl);
	DDX_Text(pDX, IDC_STATIC_INDEX, m_staticIndex);
	DDX_Text(pDX, IDC_STATIC_DATE, m_staticDate);
	DDX_Text(pDX, IDC_STATIC_TIME, m_staticTime);
	DDX_Text(pDX, IDC_EDIT_CUSTOMER, m_editCustomer);
	DDX_Text(pDX, IDC_EDIT_ADDRESS, m_editAddress);
	DDX_Text(pDX, IDC_EDIT_CITY, m_editCity);
	DDX_Text(pDX, IDC_EDIT_STATE, m_editState);
	DDX_Text(pDX, IDC_EDIT_LAST_INVOICE, m_editLastInvoice);
	DDX_Control(pDX, IDC_BTN_SHOWLOG, m_btnShowLog);
	DDX_Text(pDX, IDC_STATIC_LINE, m_staticLine);
	DDX_Control(pDX, IDC_STATIC_WAITING, m_staticWaiting);
}

BEGIN_MESSAGE_MAP(CciderDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CciderDlg::OnBnClickedBtnClose)
	ON_BN_CLICKED(IDC_BTN_PRE2, &CciderDlg::OnBnClickedBtnShowlog)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_PRE, &CciderDlg::OnBnClickedBtnPre)
	ON_BN_CLICKED(IDC_BTN_NEXT, &CciderDlg::OnBnClickedBtnNext)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_SYSTEMTRAY, &CciderDlg::OnSystemtray)
	ON_COMMAND(ID_TRAYMENU_EXIT, &CciderDlg::OnTraymenuExit)
	ON_COMMAND(ID_TRAYMENU_ABOUT, &CciderDlg::OnTraymenuAbout)
	ON_COMMAND(ID_TRAYMENU_SHOWPROGRAM, &CciderDlg::OnTraymenuShowprogram)
END_MESSAGE_MAP()


// CciderDlg message handlers

BOOL CciderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDD_MAIN_DIALOG & 0xFFF0) == IDD_MAIN_DIALOG);
	ASSERT(IDD_MAIN_DIALOG < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDD_MAIN_DIALOG, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	// TODO: Add extra initialization here
	// Change Static Text font size
	CFont* pCurFont;
	pCurFont = GetDlgItem(IDC_STATIC_NMBR)->GetFont();
	LOGFONT lf;
	pCurFont->GetLogFont(&lf);
	lf.lfHeight = +26;
	//lf.lfWeight = 400;
	//newFont.CreatePointFont(110, L"Arial");
	newFont.CreateFontIndirectW(&lf);	// must be a global variable
	GetDlgItem(IDC_STATIC_NAME)->SetFont(&newFont);
	GetDlgItem(IDC_STATIC_NMBR)->SetFont(&newFont);

	m_staticName.SetWindowTextW(L"");
	m_staticNmbr.SetWindowTextW(L"");
	m_staticWaiting.SetWindowTextW(L"");


	// Save dialog maximum size
	CRect dlgRect;
	GetWindowRect(dlgRect);
	CPoint dlgBottomRight = dlgRect.BottomRight();
	m_dlgMaxRight = dlgBottomRight.x;
	m_dlgMaxBottom = dlgBottomRight.y;


	// Set dialog initial position and size
	CRect scrRect;
	GetDesktopWindow()->GetWindowRect(scrRect);
	//SetWindowPos(NULL, rect.Width() / 2 - 579 / 2, 10, 579, 178, SWP_NOMOVE | SWP_NOZORDER);
	m_dlgMinRight = m_dlgMaxRight - WIDTH_ADJUSTMENT;
	m_dlgMinBottom = m_dlgMaxBottom - HEIGHT_ADJUSTMENT;
	SetWindowPos(NULL, scrRect.Width() / 2 - m_dlgMaxRight / 2, 200, m_dlgMinRight, m_dlgMinBottom, SWP_NOZORDER);


	//OutputDebugString(L"Debug testing output");
	//char str[255];
	//sprintf(str, "size of UINT32 %d", sizeof(UINT32));
	//AfxMessageBox(CString(str));

	//CString str;
	//str.Format(L"Top: %d, left: %d, bottom: %d, right: %d", topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
	//AfxMessageBox(str);

	// Init Notify icon
	m_NotifyIconData.cbSize = (DWORD)sizeof(NOTIFYICONDATA);
	m_NotifyIconData.hWnd = m_hWnd;
	m_NotifyIconData.uID = (UINT)m_hIcon;
	m_NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_NotifyIconData.uCallbackMessage = WM_SYSTEMTRAY;
	m_NotifyIconData.hIcon = m_hIcon;
	wcscpy_s(m_NotifyIconData.szTip, sizeof(L"Caller ID"), L"Caller ID");
	Shell_NotifyIcon(NIM_ADD, &m_NotifyIconData);


	InitListCtrl();

	GetAdapterIPs();

	InitTimer();

	// Create and suspend a Text blinking thread
	g_pTimeoutUIThread = AfxBeginThread(UpdateTimeoutUIThread, NULL, 0, 0, CREATE_SUSPENDED, NULL);

	// Start udp listener
	m_pUdpListenerThread = AfxBeginThread(UdpListenerThread, NULL);
	// Start udp Register
	m_pUdpRegisterThread = AfxBeginThread(UdpRegisterThread, NULL);
	// Start caller id listener
	m_pTcpListenerThread = AfxBeginThread(TcpListenerThread, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// Initialize ListCtrl
void CciderDlg::InitListCtrl()
{
	//m_listCtrl.SetView(LVS_REPORT);
	//m_listCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	//m_listCtrl.SetExtendedStyle(LVS_EX_GRIDLINES);

	LONG lStyle;
	lStyle = GetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; 
	lStyle |= LVS_REPORT; 
	SetWindowLong(m_listCtrl.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyle = m_listCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	m_listCtrl.SetExtendedStyle(dwStyle); 

	m_listCtrl.InsertColumn(0, L"Index", LVCFMT_LEFT, 55);
	m_listCtrl.InsertColumn(1, L"Date", LVCFMT_LEFT, 40);
	m_listCtrl.InsertColumn(2, L"Time", LVCFMT_LEFT, 40);
	m_listCtrl.InsertColumn(3, L"Line", LVCFMT_CENTER, 35);
	m_listCtrl.InsertColumn(4, L"Phone Number", LVCFMT_LEFT, 90);
	m_listCtrl.InsertColumn(5, L"Caller ID", LVCFMT_LEFT, 110);
	m_listCtrl.InsertColumn(6, L"Customer Name", LVCFMT_LEFT, 140);
	m_listCtrl.InsertColumn(7, L"Address", LVCFMT_LEFT, 120);
	m_listCtrl.InsertColumn(8, L"City", LVCFMT_LEFT, 80);
	m_listCtrl.InsertColumn(9, L"State", LVCFMT_LEFT, 40);

}

// Initialize Timers
void CciderDlg::InitTimer()
{
	// Set timer (CWnd::SetTimer)
	SetTimer(UDP_REGISTER_TIMER, 5000, NULL);
}

// On Timer
void CciderDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch (nIDEvent)
	{
	case UDP_REGISTER_TIMER:
		AfxBeginThread(UdpRegisterThread, NULL);
		break;

	case UDP_TIMEOUT_TIMER:
		g_MutexIsSvrAvailable.Lock();
		g_bIsServerAvailable = FALSE;
		g_MutexIsSvrAvailable.Unlock();
		// Force update window
		Invalidate();
		UpdateWindow();
		g_pTimeoutUIThread->ResumeThread();
		KillTimer(UDP_TIMEOUT_TIMER);
		break;
	}


	CDialog::OnTimer(nIDEvent);
}



void CciderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDD_MAIN_DIALOG)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if (nID == SC_CLOSE)
	{
		ShowWindow(SW_MINIMIZE);
		ShowWindow(SW_HIDE);
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CciderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CciderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


HBRUSH CciderDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{

	// TODO: Change any attributes of the DC here
	int  nID = pWnd->GetDlgCtrlID(); //pWnd指向当前的控件
	switch (nID)
	{

	case	IDC_EDIT_CUSTOMER:
	case	IDC_EDIT_ADDRESS:
	case	IDC_EDIT_CITY:
	case	IDC_EDIT_STATE:
	case	IDC_EDIT_LAST_INVOICE:
		return ::CreateSolidBrush(RGB(255, 255, 255));	//White background
		break;

	case	IDC_STATIC_NMBR:
	case 	IDC_STATIC_CALLER:
	case	IDC_STATIC_ADDRESS:
	case	IDC_STATIC_LASTINVOICE:
	case	IDC_STATIC_NAME:
	case	IDC_STATIC_DATE:
	case	IDC_STATIC_TIME:
	case	IDC_STATIC_INDEX:
	case	IDC_STATIC_LINE:
	case	IDC_STATIC_WAITING:
		pDC->SetBkMode(TRANSPARENT);	//Transparent background
		break;
	}
	// TODO:  Change any attributes of the DC here

	// TODO:  Return a different brush if the default is not desired
	// Dialog background color;
	if (g_bIsServerAvailable)
	{
		return ::CreateSolidBrush(RGB(154, 205, 50));	//Green_Yellow Background
	}
	else
	{
		return ::CreateSolidBrush(RGB(255, 153, 0));	// Orange Background
	}

}


void CciderDlg::OnBnClickedBtnClose()
{
	//CloseWindow();	// just close the window
	//DestroyWindow();	// will terminate program
	ShowWindow(SW_MINIMIZE);
	ShowWindow(SW_HIDE);
}


void CciderDlg::OnBnClickedBtnShowlog()
{
	// Set dialog size
	if (m_IsCallLogShowed)
	{
		CRect rectDlg;
		GetWindowRect(rectDlg);
		SetWindowPos(NULL, 0, 0, m_dlgMinRight, m_dlgMinBottom, SWP_NOMOVE | SWP_NOZORDER);
		m_listCtrl.DeleteAllItems();
		m_btnShowLog.SetWindowTextW(L"Show Log >>>");
		m_IsCallLogShowed = FALSE;
	}
	else
	{
		CRect rectDlg;
		GetWindowRect(rectDlg);
		SetWindowPos(NULL, 0, 0, m_dlgMaxRight, m_dlgMaxBottom, SWP_NOMOVE | SWP_NOZORDER);
		//CenterWindow();
		m_IsCallLogShowed = TRUE;
		AfxBeginThread(TcpQueryThread, NULL);
		m_btnShowLog.SetWindowTextW(L"Close Log <<<");
	}

	//GetDlgItem(IDC_STATIC_NAME)->SetWindowTextW(L"Clicked");
	//AfxMessageBox(L"button clicked");
}

UINT CciderDlg::UdpRegisterThread(LPVOID param)
{
	// Called AfxSocketInit() on CWinApp::InitInstance()
	SOCKET registeSocket = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN addrSrv;
	CString strIP;
	// Broadcast to all adapter IPs
	for (int i = 0; i < AdapterIPs.GetCount(); i++)
	{
		strIP = AdapterIPs.GetAt(i);
		if (strIP.Compare(L"0.0.0.0") == 0) // CString.Compare() return 0 is identical
			continue;
		// Build broadcast IP
		strIP = strIP.Left(strIP.ReverseFind('.') + 1) + L"255";
		//AfxMessageBox(strIP);
		InetPton(AF_INET, strIP, &addrSrv.sin_addr.S_un.S_addr);
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(5001);
		PACKAGE_SEND package_send;
		sendto(registeSocket, (char*)&package_send, 1024, 0, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

		OutputDebugString(L"Send register package to " + strIP + "\n");

		closesocket(registeSocket);
	}
	if (g_bIsServerAvailable == TRUE)
	{
		AfxGetApp()->GetMainWnd()->SetTimer(UDP_TIMEOUT_TIMER, 2000, NULL);
	}

	OutputDebugString(L"updRegisterThread exit.\n");
	return 0;
}

UINT CciderDlg::UdpListenerThread(LPVOID param)
{
	SOCKET listenerSocket = socket(AF_INET, SOCK_DGRAM, 0);

	SOCKADDR_IN addrLocal;
	addrLocal.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrLocal.sin_family = AF_INET;
	addrLocal.sin_port = htons(5002);

	bind(listenerSocket, (SOCKADDR*)&addrLocal, sizeof(SOCKADDR));

	SOCKADDR_IN addrSender;
	int len = sizeof(SOCKADDR);
	PACKAGE_DATA recv_buff;
	char tempBuf[1024];
	int nReceived = 0;
	CString strIP;
	BOOL bIsAddedServer;
	for (;;)
	{
		bIsAddedServer = false;
		nReceived = recvfrom(listenerSocket, (char*)&recv_buff, 1024, 0, (SOCKADDR*)&addrSender, &len);
		if (nReceived == SOCKET_ERROR) continue;	// SOCKET_ERROR = -1
		if (nReceived == 1024)
		{
			if (recv_buff.START == PROTOCOL_START &&
				recv_buff.ACK == PROTOCOL_ACK &&
				recv_buff.CR == PROTOCOL_CR)
			{
				AfxGetApp()->GetMainWnd()->KillTimer(UDP_TIMEOUT_TIMER);

				if (g_bIsServerAvailable == FALSE)
				{
					g_MutexIsSvrAvailable.Lock();
					g_bIsServerAvailable = TRUE;
					g_MutexIsSvrAvailable.Unlock();

					g_pTimeoutUIThread->SuspendThread();
					AfxGetApp()->GetMainWnd()->SetDlgItemTextW(IDC_STATIC_WAITING, L"");
					AfxGetApp()->GetMainWnd()->Invalidate();
					AfxGetApp()->GetMainWnd()->UpdateWindow();
				}

				for (int i = 0; i < ServerIPs.GetCount(); i++)
				{
					if (CString(inet_ntoa(addrSender.sin_addr)).Compare(ServerIPs.GetAt(i)) == 0)
					{
						bIsAddedServer = true;
						OutputDebugString(CString(inet_ntoa(addrSender.sin_addr)) + " already in the list\n");
					}
				}

				if (!bIsAddedServer)
				{
					ServerIPs.Add(CString(inet_ntoa(addrSender.sin_addr)));
					OutputDebugString(L"Added a new server IP: " + CString(inet_ntoa(addrSender.sin_addr)) + "\n");
				}
			}

			sprintf_s(tempBuf, "Received from [%s]: %s", inet_ntoa(addrSender.sin_addr), recv_buff.data);
			//AfxMessageBox(CString(tempBuf));
		}
	}

	return 0;
}

UINT CciderDlg::TcpListenerThread(LPVOID param)
{
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN listenerAddr;
	listenerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	listenerAddr.sin_family = AF_INET;
	listenerAddr.sin_port = htons(5005);

	if (0 != bind(listenerSocket, (SOCKADDR*)&listenerAddr, sizeof(SOCKADDR)))
	{
		AfxMessageBox(L"TCP listener binding error");
	}

	listen(listenerSocket, 10);

	SOCKADDR_IN clientAddr;
	int len = sizeof(SOCKADDR);

	while (TRUE)
	{
		SOCKET sockConn = accept(listenerSocket, (SOCKADDR*)&clientAddr, &len);
		THREAD_PARAM* param = new THREAD_PARAM;
		param->sockConn = &sockConn;
		param->clientAddr = &clientAddr;

		AfxBeginThread(TcpConnectionThread, param);
	}

	return 0;
}

UINT CciderDlg::TcpConnectionThread(LPVOID param)
{

	int nReceived = 0;
	int nOffset = 0;
	int nByteRemain = 1024;
	PACKAGE_JSON_SIZE	buff_JsonSize;
	char tempBuf[1024];

	SOCKET _sockConn = *((THREAD_PARAM*)param)->sockConn;
	SOCKADDR_IN _clientAddr = *((THREAD_PARAM*)param)->clientAddr;

	// Receive first package, parse json size
	while (true)
	{
		nReceived = recv(_sockConn, (char*)&buff_JsonSize + nOffset, nByteRemain, 0);
		// SOCKET_ERROR = -1
		if (nReceived == SOCKET_ERROR)
		{
			closesocket(_sockConn);
			return 1;
		}
		if (nReceived >= nByteRemain) break;
		// Keep reading util buffer filled
		nByteRemain = nByteRemain - nReceived;
		nOffset = nOffset + nReceived;
	}

	// Verify the package
	if (buff_JsonSize.START != PROTOCOL_START ||
		buff_JsonSize.ACK != PROTOCOL_ACK ||
		buff_JsonSize.CR != PROTOCOL_CR)
	{
		closesocket(_sockConn);
		return 1;
	}

	// Convert byte array to UINT32  -QIEN
	UINT32 u32JsonSize = buff_JsonSize.jsonSize[3] << 24 |
		buff_JsonSize.jsonSize[2] << 16 |
		buff_JsonSize.jsonSize[1] << 8 |
		buff_JsonSize.jsonSize[0];
	// sprintf output UINT32 -QIEN 
	sprintf_s(tempBuf, "TCP received from [%s]: %I32d", inet_ntoa(_clientAddr.sin_addr), u32JsonSize);
	//AfxMessageBox(CString(tempBuf));

	// Receive data package
	nOffset = 0;
	nReceived = 0;
	nByteRemain = u32JsonSize * 1024;;
	byte* buff_Json = new byte[nByteRemain];
	while (true)
	{
		nReceived = recv(_sockConn, (char*)buff_Json + nOffset, nByteRemain, 0);
		// SOCKET_ERROR = -1
		if (nReceived == SOCKET_ERROR)
		{
			closesocket(_sockConn);
			return 1;
		}
		// Keep reading util buffer filled
		if (nReceived >= nByteRemain) break;
		nByteRemain = nByteRemain - nReceived;
		nOffset = nOffset + nReceived;
	}

	//sprintf(tempBuf, "TCP received from [%s]: %s", inet_ntoa(_clientAddr.sin_addr), (char*)buff_Json);
	//AfxMessageBox(CString((LPCSTR)buff_Json, u32JsonSize * 1024));

	/*
	* JSON sample:
	* {"address":"","city":"","company_name":"","last_invoice_date":"","line":"1","name":"KEVIN HUANG","nmbr":"13475250000","state":""}
	*/
	using namespace rapidjson;
	Document document;
	if (document.ParseInsitu((char*)buff_Json).HasParseError())
	{
		AfxMessageBox(L"Json parse error.");
		delete[] buff_Json;
		return 1;
	}
	if (!document.IsObject())
	{
		delete[] buff_Json;
		return 1;
	}

	CALL_INFO callInfo;

	if (document.HasMember("name"))
	{
		callInfo.name = CString(document["name"].GetString());
	}
	if (document.HasMember("nmbr"))
	{
		callInfo.nmbr = CString(document["nmbr"].GetString());
		if (callInfo.nmbr.GetLength() == 10)
		{
			callInfo.nmbr.Insert(6, L"-");
			callInfo.nmbr.Insert(3, L"-");
		}
	}

	//if (document.HasMember("id"))
	//{
	if (document.HasMember("id"))
	{
		CString strIndex;
		if (CallInfoArray.GetCount() < MAX_CALL_LOG_QUEUE)
		{
			strIndex.Format(L"#%d", CallInfoArray.GetCount() + 1);
		}
		else
		{
			strIndex.Format(L"#%d", CallInfoArray.GetCount());
		}

		callInfo.id = strIndex;
	}

	if (document.HasMember("line"))
	{
		callInfo.phone_line.Format(L"Line %s", CString(document["line"].GetString()));
	}

	if (document.HasMember("date"))
	{
		callInfo.date = CString(document["date"].GetString());
		if (callInfo.date.GetLength() == 4)
		{
			callInfo.date.Insert(2, L"/");
		}
	}
	if (document.HasMember("time"))
	{
		callInfo.time = CString(document["time"].GetString());
		if (callInfo.time.GetLength() == 4)
		{
			callInfo.time.Insert(2, L":");
		}
	}
	if (document.HasMember("company_name"))
	{
		callInfo.company_name = CString(document["company_name"].GetString());
	}
	if (document.HasMember("address"))
	{
		callInfo.address = CString(document["address"].GetString());
	}
	if (document.HasMember("city"))
	{
		callInfo.city = CString(document["city"].GetString());
	}
	if (document.HasMember("state"))
	{
		callInfo.state = CString(document["state"].GetString());
	}
	if (document.HasMember("last_invoice_date"))
	{
		callInfo.last_invoice_date = CString(document["last_invoice_date"].GetString());
	}

	// Add call to CallInfoArray, limited 20
	CSingleLock wait(&g_MutexCallInfoArray);
	if (CallInfoArray.GetCount() < MAX_CALL_LOG_QUEUE)
	{
		CallInfoArray.Add(callInfo);
	}
	else
	{
		CallInfoArray.RemoveAt(0);
		CallInfoArray.Add(callInfo);
	}
	nCallArrayIndex = CallInfoArray.GetCount() - 1;
	wait.Unlock();

	// Update UI
	((CciderDlg*)AfxGetApp()->GetMainWnd())->UpdateMainUI();
	((CciderDlg*)AfxGetApp()->GetMainWnd())->ShowWindow(SW_SHOW);
	((CciderDlg*)AfxGetApp()->GetMainWnd())->ShowWindow(SW_NORMAL);

	closesocket(_sockConn);
	//AfxMessageBox(L"tcpConnetionThread exit");

	// delete dynamic array varyable -QIEN
	delete[] buff_Json;
	return 0;
}

void UpdateUIText()
{

}

UINT CciderDlg::TcpQueryThread(LPVOID param)
{
	CString strSvrIP;
	if (ServerIPs.GetCount() == 0)
	{
		//AfxMessageBox(L"No server found.");
		return 1;
	}
	else
	{
		strSvrIP = ServerIPs.GetAt(0);
	}

	SOCKET socketSvr = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN sockAddrSrv;
	//sockAddrSrv.sin_addr.S_un.S_addr = inet_addr("192.168.1.202");  // Caused _WINSOCK_DEPRECATED_NO_WARNINGS
	InetPton(AF_INET, strSvrIP, &sockAddrSrv.sin_addr.S_un.S_addr);
	sockAddrSrv.sin_family = AF_INET;
	sockAddrSrv.sin_port = htons(TCP_SERVER_PORT);
	connect(socketSvr, (SOCKADDR*)&sockAddrSrv, sizeof(SOCKADDR));

	PACKAGE_QUERY_CMD pkgQryCmd;
	// String to char array -QIEN
	//strcpy((char*)pkgQryCmd.command, CT2A(L"GET_CALL_LOG"));
	// L"GET_CALL_LOG") is 13 bytes, not 12
	strcpy_s((char*)(pkgQryCmd.command), 13, CT2A(L"GET_CALL_LOG"));
	send(socketSvr, (char*)&pkgQryCmd, sizeof(pkgQryCmd), 0);

	// Receive data package
	PACKAGE_QUERY_DATA pkgQryData;
	//UINT32 u32JsonSize;
	int nOffset;
	int nReceived;
	int nByteRemain;
	int i = 0;
	CString strDate;
	CString strTime;
	CString strNmbr;
	CListCtrl* listCtrl = (CListCtrl*)AfxGetApp()->GetMainWnd()->GetDlgItem(IDC_LIST1);
	listCtrl->SetRedraw(FALSE);
	while (true)
	{
		// Receive data package, parse json size
		nOffset = 0;
		nReceived = 0;
		nByteRemain = 1024;
		while (true)
		{
			nReceived = recv(socketSvr, (char*)&pkgQryData + nOffset, nByteRemain, 0);
			// SOCKET_ERROR = -1
			if (nReceived == SOCKET_ERROR)
			{
				closesocket(socketSvr);
				return 1;
			}
			if (nReceived >= nByteRemain) break;
			// Keep reading util buffer filled
			nByteRemain = nByteRemain - nReceived;
			nOffset = nOffset + nReceived;
		}

		if (pkgQryData.EOT == PROTOCOL_EOT)
		{
			listCtrl->SetRedraw(TRUE);
			listCtrl->Invalidate();
			listCtrl->UpdateWindow();
			if (socketSvr != NULL) closesocket(socketSvr);
			return 0;
		}

		// Convert byte array to UINT32  -QIEN
		//u32JsonSize = pkgQryData.jsonSize[3] << 24 |
		//	pkgQryData.jsonSize[2] << 16 |
		//	pkgQryData.jsonSize[1] << 8 |
		//	pkgQryData.jsonSize[0];
		/*
		* JSON sample:
		* {"address":"","city":"","company_name":"","last_invoice_date":"","line":"1","name":"KEVIN HUANG","nmbr":"13475250000","state":""}
		*/
		using namespace rapidjson;
		Document* pDocument = new Document();
		if (pDocument->ParseInsitu((char*)pkgQryData.jsonData).HasParseError())
		{
			AfxMessageBox(L"Json parse error.");
			break;
		}
		if (!pDocument->IsObject())
		{
			AfxMessageBox(L"Json is not valid.");
			break;
		}
		// Update ListCtrl
		((CciderDlg*)AfxGetApp()->GetMainWnd())->UpdateListCtrl(i, pDocument);
		if (pDocument != NULL)
		{
			delete pDocument;
		}

		i++; // The insert position is growing up
	}
	listCtrl->SetRedraw(TRUE);
	listCtrl->Invalidate();
	listCtrl->UpdateWindow();
	if (socketSvr != NULL) closesocket(socketSvr);
	return 1;
}




void CciderDlg::OnBnClickedBtnPre()
{
	if (nCallArrayIndex > 0)
	{
		CSingleLock wait(&g_MutexCallInfoArray);
		--nCallArrayIndex;
		UpdateMainUI();
		wait.Unlock();
	}
}


void CciderDlg::OnBnClickedBtnNext()
{
	// If no call received, exit
	if (CallInfoArray.GetCount() == 0)
	{
		return;
	}

	// If is not the last one
	if (nCallArrayIndex < CallInfoArray.GetCount() - 1)
	{
		CSingleLock wait(&g_MutexCallInfoArray);
		++nCallArrayIndex;
		UpdateMainUI();
		wait.Unlock();
	}
}

// Updade main ui for displying previous/next call
void CciderDlg::UpdateMainUI()
{
	// This function will be called by threads, can't updaet UI by UpdateDate(FALSE)
	// SetDlgItemTextW() will send message instead
	CALL_INFO c = CallInfoArray.GetAt(nCallArrayIndex);
	SetDlgItemTextW(IDC_STATIC_NMBR, c.nmbr);
	SetDlgItemTextW(IDC_STATIC_NAME, c.name);
	SetDlgItemTextW(IDC_STATIC_INDEX, c.id);
	SetDlgItemTextW(IDC_STATIC_LINE, c.phone_line);
	SetDlgItemTextW(IDC_STATIC_DATE, c.date);
	SetDlgItemTextW(IDC_STATIC_TIME, c.time);
	SetDlgItemTextW(IDC_EDIT_CUSTOMER, c.company_name);
	SetDlgItemTextW(IDC_EDIT_ADDRESS, c.address);
	SetDlgItemTextW(IDC_EDIT_CITY, c.city);
	SetDlgItemTextW(IDC_EDIT_STATE, c.state);
	SetDlgItemTextW(IDC_EDIT_LAST_INVOICE, c.last_invoice_date);

}

UINT CciderDlg::UpdateTimeoutUIThread(LPVOID param)
{
	for (;;)
	{
		((CciderDlg*)AfxGetApp()->GetMainWnd())->SetDlgItemTextW(IDC_STATIC_WAITING, L"");
		Sleep(500);
		((CciderDlg*)AfxGetApp()->GetMainWnd())->SetDlgItemTextW(IDC_STATIC_WAITING, L"Waiting for server...");
		Sleep(850);
	}

}

// Update ListCtrl, insert one row
void CciderDlg::UpdateListCtrl(int iPosition, rapidjson::Document *pDocument)
{
	// Insert(index, string)   Index of the item to be inserted -QIEN
	int nItem;
	nItem = m_listCtrl.InsertItem(iPosition, L"");

	if (pDocument->HasMember("id"))
	{
		m_listCtrl.SetItemText(nItem, 0, CString((*pDocument)["id"].GetString()));
		//OutputDebugString(CString(pDocument["id"].GetString()) + "\n");
	}
	if (pDocument->HasMember("date"))
	{
		CString strDate = CString((*pDocument)["date"].GetString());
		if (strDate.GetLength() == 4)
		{
			strDate.Insert(2, L"/");
		}
		m_listCtrl.SetItemText(nItem, 1, strDate);
	}
	if (pDocument->HasMember("time"))
	{
		CString strTime = CString((*pDocument)["time"].GetString());
		if (strTime.GetLength() == 4)
		{
			strTime.Insert(2, L":");
		}
		m_listCtrl.SetItemText(nItem, 2, strTime);
	}
	if (pDocument->HasMember("line"))
	{
		m_listCtrl.SetItemText(nItem, 3, CString((*pDocument)["line"].GetString()));
	}
	if (pDocument->HasMember("nmbr"))
	{
		CString strNmbr = CString((*pDocument)["nmbr"].GetString());
		if (strNmbr.GetLength() == 10)
		{
			strNmbr.Insert(6, L"-");
			strNmbr.Insert(3, L"-");
		}
		m_listCtrl.SetItemText(nItem, 4, strNmbr);
	}
	if (pDocument->HasMember("name"))
	{
		m_listCtrl.SetItemText(nItem, 5, CString((*pDocument)["name"].GetString()));
	}
	if (pDocument->HasMember("company_name"))
	{
		m_listCtrl.SetItemText(nItem, 6, CString((*pDocument)["company_name"].GetString()));
	}
	if (pDocument->HasMember("address"))
	{
		m_listCtrl.SetItemText(nItem, 7, CString((*pDocument)["address"].GetString()));
	}
	if (pDocument->HasMember("city"))
	{
		m_listCtrl.SetItemText(nItem, 8, CString((*pDocument)["city"].GetString()));
	}
	if (pDocument->HasMember("state"))
	{
		m_listCtrl.SetItemText(nItem, 9, CString((*pDocument)["state"].GetString()));
	}

}



void CciderDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
	Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
}


afx_msg LRESULT CciderDlg::OnSystemtray(WPARAM wParam, LPARAM lParam)
{
	CMenu menu;
	CPoint curpos;

	switch (lParam)
	{
	case WM_LBUTTONUP:
		if (::IsWindowVisible(m_hWnd))
		{
			ShowWindow(SW_MINIMIZE);
			ShowWindow(SW_HIDE);
		}
		else
		{
			ShowWindow(SW_SHOW);
			ShowWindow(SW_SHOWNORMAL);
		}
		break;
	case WM_RBUTTONUP:
		GetCursorPos(&curpos);
		if (menu.LoadMenuW(IDR_MENU1))
		{
			CMenu *pPopup;
			pPopup = menu.GetSubMenu(0);
			SetForegroundWindow();
			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, curpos.x, curpos.y, AfxGetMainWnd());
			HMENU hmenu = menu.Detach();
			menu.DestroyMenu();
		}
		break;
	default:
		break;
	}
	return 0;
}


void CciderDlg::OnTraymenuExit()
{
	m_IsExit = TRUE;
	PostMessage(WM_QUIT);
}


void CciderDlg::OnTraymenuAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


void CciderDlg::OnTraymenuShowprogram()
{
	if (!IsWindowVisible())
	{
		ShowWindow(SW_SHOW);
		ShowWindow(SW_SHOWNORMAL);
	}
}

void CciderDlg::GetAdapterIPs()
{
	/* Declare and initialize variables */

	// It is possible for an adapter to have multiple
	// IPv4 addresses, gateways, and secondary WINS servers
	// assigned to the adapter. 
	//
	// Note that this sample code only prints out the 
	// first entry for the IP address/mask, and gateway, and
	// the primary and secondary WINS server for each adapter. 

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	/* variables used to print DHCP time info */
	struct tm newtime;
	char buffer[32];
	errno_t error;

	CString str;
	CString strGateway;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		OutputDebugString(L"Error allocating memory needed to call GetAdaptersinfo\n");
		return;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			OutputDebugString(L"Error allocating memory needed to call GetAdaptersinfo\n");
			return;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;

		while (pAdapter) {
			str.Format(L"\tComboIndex: %d ", pAdapter->ComboIndex);
			OutputDebugString(str);
			str.Format(L"\tAdapter Name: \t%s\n", CString(pAdapter->AdapterName));
			OutputDebugString(str);
			str.Format(L"\tAdapter Desc: \t%s\n", CString(pAdapter->Description));
			OutputDebugString(str);
			str.Format(L"\tAdapter Addr: \t");
			OutputDebugString(str);
			for (i = 0; i < pAdapter->AddressLength; i++) {
				if (i == (pAdapter->AddressLength - 1))
				{
					str.Format(L"%.2X\n", (int)pAdapter->Address[i]);
					OutputDebugString(str);
				}
				else
				{
					str.Format(L"%.2X-", (int)pAdapter->Address[i]);
					OutputDebugString(str);
				}
			}
			str.Format(L"\tIndex: \t%d\n", pAdapter->Index);
			OutputDebugString(str);
			OutputDebugString(L"\tType: \t");
			switch (pAdapter->Type) {
			case MIB_IF_TYPE_OTHER:
				OutputDebugString(L"Other\n");
				break;
			case MIB_IF_TYPE_ETHERNET:
				OutputDebugString(L"Ethernet\n");
				break;
			case MIB_IF_TYPE_TOKENRING:
				OutputDebugString(L"Token Ring\n");
				break;
			case MIB_IF_TYPE_FDDI:
				OutputDebugString(L"FDDI\n");
				break;
			case MIB_IF_TYPE_PPP:
				OutputDebugString(L"PPP\n");
				break;
			case MIB_IF_TYPE_LOOPBACK:
				OutputDebugString(L"Lookback\n");
				break;
			case MIB_IF_TYPE_SLIP:
				OutputDebugString(L"Slip\n");
				break;
			default:
				str.Format(L"Unknown type %ld\n", pAdapter->Type);
				OutputDebugString(str);
				break;
			}

			// Saving IPs to array
			AdapterIPs.Add(CString(pAdapter->IpAddressList.IpAddress.String));

			str.Format(L"\tIP Address: \t%s\n", CString(pAdapter->IpAddressList.IpAddress.String));
			OutputDebugString(str);
			str.Format(L"\tIP Mask: \t%s\n", CString(pAdapter->IpAddressList.IpMask.String));
			OutputDebugString(str);
			str.Format(L"\tGateway: \t%s\n", CString(pAdapter->GatewayList.IpAddress.String));
			OutputDebugString(str);
			//AfxMessageBox(CString(pAdapter->GatewayList.IpAddress.String));
			OutputDebugString(L"\t***\n");

			if (pAdapter->DhcpEnabled) {
				OutputDebugString(L"\tDHCP Enabled: Yes\n");
				str.Format(L"\t  DHCP Server: \t%s\n", CString(pAdapter->DhcpServer.IpAddress.String));
				OutputDebugString(str);

				OutputDebugString(L"\t  Lease Obtained: ");
				/* Display local time */
				error = _localtime32_s(&newtime, (__time32_t*)&pAdapter->LeaseObtained);
				if (error)
					OutputDebugString(L"Invalid Argument to _localtime32_s\n");
				else {
					// Convert to an ASCII representation 
					error = asctime_s(buffer, 32, &newtime);
					if (error)
						OutputDebugString(L"Invalid Argument to asctime_s\n");
					else
						/* asctime_s returns the string terminated by \n\0 */
						str.Format(L"%s", CString(buffer));
					OutputDebugString(str);
				}

				OutputDebugString(L"\t  Lease Expires:  ");
				error = _localtime32_s(&newtime, (__time32_t*)&pAdapter->LeaseExpires);
				if (error)
					OutputDebugString(L"Invalid Argument to _localtime32_s\n");
				else {
					// Convert to an ASCII representation 
					error = asctime_s(buffer, 32, &newtime);
					if (error)
						OutputDebugString(L"Invalid Argument to asctime_s\n");
					else
						/* asctime_s returns the string terminated by \n\0 */
						str.Format(L"%s", CString(buffer));
					OutputDebugString(str);
				}
			}
			else
				OutputDebugString(L"\tDHCP Enabled: No\n");

			if (pAdapter->HaveWins) {
				OutputDebugString(L"\tHave Wins: Yes\n");
				str.Format(L"\t  Primary Wins Server:    %s\n", CString(pAdapter->PrimaryWinsServer.IpAddress.String));
				OutputDebugString(str);
				str.Format(L"\t  Secondary Wins Server:  %s\n", CString(pAdapter->SecondaryWinsServer.IpAddress.String));
				OutputDebugString(str);
			}
			else
				OutputDebugString(L"\tHave Wins: No\n");
			pAdapter = pAdapter->Next;
			OutputDebugString(L"\n");
		}
	}
	else {
		str.Format(L"GetAdaptersInfo failed with error: %d\n", dwRetVal);
		OutputDebugString(str);

	}
	if (pAdapterInfo)
		free(pAdapterInfo);

	return;

	//AfxMessageBox(L"Get gateway IP!");
}

void CciderDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	if (IDYES == AfxMessageBox(L"Do you want to exit this program?", MB_YESNO))
	{
		DestroyWindow();
	}
	//CDialog::OnCancel();
}
