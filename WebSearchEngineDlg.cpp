/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// WebSearchEngineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WebSearchEngine.h"
#include "WebSearchEngineDlg.h"
#include "WebSearchEngineExt.h"
#include "ConnectionSettingsDlg.h"
#include "HtmlToText.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DWORD WINAPI CrawlingThreadProc(LPVOID lpParam);

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
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CWebSearchEngineDlg dialog

CWebSearchEngineDlg::CWebSearchEngineDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WEBSEARCHENGINE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bThreadRunning = false;
	m_nThreadID = 0;
}

void CWebSearchEngineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CRAWLING, m_pCrawling);
	DDX_Control(pDX, IDC_PROGRESS, m_pProgress);
	DDX_Control(pDX, IDC_WEBPAGES, m_pWebpageCounter);
	DDX_Control(pDX, IDC_KEYWORDS, m_pKeywordCounter);
}

BEGIN_MESSAGE_MAP(CWebSearchEngineDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CWebSearchEngineDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CWebSearchEngineDlg message handlers

BOOL CWebSearchEngineDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

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
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	CConnectionSettingsDlg pConnectionSettingsDlg(this);
	if (pConnectionSettingsDlg.DoModal() != IDOK)
		return FALSE;

	CWinApp* pWinApp = AfxGetApp();
	ASSERT(pWinApp != NULL);

	CString strHostName = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTNAME, DEFAULT_HOSTNAME);
	CString strHostPort = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTPORT, DEFAULT_HOSTPORT);
	CString strDatabase = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_DATABASE, DEFAULT_DATABASE);
	// CString strFileName = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_FILENAME, DEFAULT_FILENAME);
	CString strUsername = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_USERNAME, DEFAULT_USERNAME);

	TCHAR lpszPassword[0x100] = { 0, };
	VERIFY(GetRegistryPassword(NULL, REGKEY_SECTION, REGKEY_PASSWORD, lpszPassword, DEFAULT_PASSWORD));

	SQLRETURN nRet = m_pEnvironment.Create();
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	nRet = m_pEnvironment.SetAttr(SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	nRet = m_pEnvironment.SetAttrU(SQL_ATTR_CONNECTION_POOLING, SQL_CP_OFF);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	nRet = m_pConnection.Create(m_pEnvironment);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pConnection);

	_stprintf(m_sConnectionInString, _T("Driver={MySQL ODBC 3.51 Driver};Server=%s;Port=%s;Database=%s;Uid=%s;Pwd=%s;"),
		strHostName.GetBuffer(0), strHostPort.GetBuffer(0), strDatabase.GetBuffer(0), strUsername.GetBuffer(0), lpszPassword);
	strHostName.ReleaseBuffer();
	strHostPort.ReleaseBuffer();
	strDatabase.ReleaseBuffer();
	strUsername.ReleaseBuffer();
	nRet = m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(m_sConnectionInString)), m_sConnectionOutString);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pConnection);

	m_pWebpageCounter.SetWindowText(_T("0"));
	m_pKeywordCounter.SetWindowText(_T("0"));

	CGenericStatement pGenericStatement;
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `occurrence`;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `keyword`;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `webpage`;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `webpage` (`webpage_id` BIGINT NOT NULL AUTO_INCREMENT, `url` VARCHAR(256) NOT NULL, `title` VARCHAR(256) NOT NULL, `content` TEXT NOT NULL, PRIMARY KEY(`webpage_id`)) ENGINE=InnoDB;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `keyword` (`keyword_id` BIGINT NOT NULL AUTO_INCREMENT, `name` VARCHAR(256) NOT NULL, PRIMARY KEY(`keyword_id`)) ENGINE=InnoDB;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `occurrence` (`webpage_id` BIGINT NOT NULL, `keyword_id` BIGINT NOT NULL, `counter` BIGINT NOT NULL, `pagerank` REAL NOT NULL, PRIMARY KEY(`webpage_id`, `keyword_id`), FOREIGN KEY webpage_fk(webpage_id) REFERENCES webpage(webpage_id), FOREIGN KEY keyword_fk(keyword_id) REFERENCES keyword(keyword_id)) ENGINE=InnoDB;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE UNIQUE INDEX index_name ON `keyword`(`name`);")));

	m_hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CrawlingThreadProc, this, 0, &m_nThreadID);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWebSearchEngineDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWebSearchEngineDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWebSearchEngineDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD WINAPI CrawlingThreadProc(LPVOID lpParam)
{
	std::string lpszURL, lpszFilename;
	if (lpParam != NULL)
	{
		CWebSearchEngineDlg* pWebSearchEngineDlg = (CWebSearchEngineDlg*)lpParam;
		pWebSearchEngineDlg->m_bThreadRunning = true;
		pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
		AddURLToFrontier("https://en.wikipedia.org/");
		while (pWebSearchEngineDlg->m_bThreadRunning)
		{
			if (ExtractURLFromFrontier(lpszURL))
			{
				pWebSearchEngineDlg->m_pCrawling.SetWindowText(CString(lpszURL.c_str()));
				if (DownloadURLToFile(lpszURL, lpszFilename))
				{
					if (!ProcessHTML(pWebSearchEngineDlg, lpszFilename, lpszURL))
					{
						break;
					}
				}
			}
			else
				break;
		}

		pWebSearchEngineDlg->m_bThreadRunning = false;
		pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
	}

	::ExitThread(0);
	return 0;
}

BOOL WaitWithMessageLoop(HANDLE hEvent, DWORD dwTimeout)
{
	DWORD dwRet;
	MSG msg;
	hEvent = hEvent ? hEvent : CreateEvent(NULL, FALSE, FALSE, NULL);

	while (true)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
		if (dwRet == WAIT_OBJECT_0)
			return TRUE;
		if (dwRet != WAIT_OBJECT_0 + 1)
			break;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE;
		}
	}
	return FALSE;
}

void CWebSearchEngineDlg::OnBnClickedCancel()
{
	if (m_bThreadRunning)
	{
		m_bThreadRunning = false;
		VERIFY(WaitWithMessageLoop(m_hThread, INFINITE));
	}
	CDialogEx::OnCancel();
}
