
// ciderDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#define WM_SYSTEMTRAY WM_USER+101

struct CALL_INFO {
	CString id;
	CString phone_line;
	CString	date;
	CString	time;
	CString	nmbr;
	CString	name;
	CString	customer_record;
	CString	last_invoice_date;
	CString	company_name;
	CString	address;
	CString	city;
	CString	state;
	CString	custom_col1;
	CString	custom_col2;
	CString	custom_col3;
	CString	custom_col4;
	CString	custom_col5;
};

// CciderDlg dialog
class CciderDlg : public CDialog
{
	// Construction
public:
	CciderDlg(CWnd* pParent = NULL);	// standard constructor


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	CFont newFont;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	//	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
		//CEdit m_editCallerName;
	afx_msg void OnBnClickedBtnClose();
	afx_msg void OnBnClickedBtnShowlog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	CWinThread* m_pUdpRegisterThread;
	CWinThread* m_pUdpListenerThread;
	CWinThread* m_pTcpQueryThread;
	CWinThread* m_pTcpListenerThread;

	static UINT UdpRegisterThread(LPVOID param);
	static UINT UdpListenerThread(LPVOID param);
	static UINT TcpQueryThread(LPVOID param);
	static UINT TcpListenerThread(LPVOID param);
	static UINT TcpConnectionThread(LPVOID param);
	static UINT UpdateTimeoutUIThread(LPVOID paramn);


	void InitListCtrl();

	BOOL m_IsCallLogShowed;
	bool m_IsExit;

	CStatic m_staticNmbr;
	CStatic m_staticName;

	CString m_staticIndex;
	CString m_staticLine;
	CString m_staticDate;
	CString m_staticTime;
	CListCtrl m_listCtrl;

	LONG m_dlgMinRight;
	LONG m_dlgMinBottom;
	LONG m_dlgMaxRight;
	LONG m_dlgMaxBottom;

	// Static 
	static CArray<CALL_INFO> CallInfoArray;
	static CArray<CString>  AdapterIPs;
	static CArray<CString> ServerIPs;
	static int nCallArrayIndex;



	afx_msg void OnBnClickedBtnPre();
	afx_msg void OnBnClickedBtnNext();
	CString m_editCustomer;
	CString m_editAddress;
	CString m_editCity;
	CString m_editState;
	CString m_editLastInvoice;
	CButton m_btnShowLog;

	NOTIFYICONDATA m_NotifyIconData;

private:
	void UpdateMainUI();
	void UpdateListCtrl(int iPostion, rapidjson::Document* pDocument);
public:
	afx_msg void OnDestroy();
protected:
	afx_msg LRESULT OnSystemtray(WPARAM wParam, LPARAM lParam);
	
public:
	afx_msg void OnTraymenuExit();
	afx_msg void OnTraymenuAbout();
	afx_msg void OnTraymenuShowprogram();
	CStatic m_staticWaiting;
	void GetAdapterIPs();
	void InitTimer();
	virtual void OnCancel();
};
