/* Copyright (C) 2022-2026 Stefan-Mihai MOGA
This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.

WebSearchEngine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

WebSearchEngine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
WebSearchEngine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// WebSearchEngineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WebSearchEngine.h"

#include "WebSearchEngineDlg.h"
#include "WebSearchEngineExt.h"
#include "ConnectionSettingsDlg.h"
#include "HtmlToText.h"

#include "HLinkCtrl.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Entry point for the background web-crawling worker thread.
 * @param lpParam Pointer to the owning CWebSearchEngineDlg instance.
 * @return Thread exit code.
 */
DWORD WINAPI CrawlingThreadProc(LPVOID lpParam);

/**
 * @brief Dialog displayed when the user selects "About" from the system menu.
 *
 * Shows the application version, license text, website, and contact e-mail.
 */
class CAboutDlg : public CDialog
{
public:
	/** @brief Standard constructor. */
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	/**
	 * @brief Exchanges data between dialog controls and member variables.
	 * @param pDX Pointer to the data-exchange object.
	 */
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
public:
	/**
	 * @brief Initializes the About dialog, populating the version label,
	 *        license text, website, and e-mail hyperlinks.
	 * @return TRUE to set input focus to the first control; FALSE otherwise.
	 */
	virtual BOOL OnInitDialog();

	/**
	 * @brief Handles WM_DESTROY; performs cleanup before the window is destroyed.
	 */
	afx_msg void OnDestroy();

protected:
	CStatic m_ctrlVersion;
	CEdit m_ctrlWarning;
	CVersionInfo m_pVersionInfo;
	CHLinkCtrl m_ctrlWebsite;
	CHLinkCtrl m_ctrlEmail;

	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VERSION, m_ctrlVersion);
	DDX_Control(pDX, IDC_WARNING, m_ctrlWarning);
	DDX_Control(pDX, IDC_WEBSITE, m_ctrlWebsite);
	DDX_Control(pDX, IDC_EMAIL, m_ctrlEmail);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/**
 * @brief Retrieves the fully-qualified path of the current executable.
 * @param pdwLastError Optional output parameter that receives the last Win32
 *                     error code on failure, or ERROR_SUCCESS on success.
 * @return CString containing the full module path, or an empty string on failure.
 */
CString GetModuleFileName(_Inout_opt_ DWORD* pdwLastError = nullptr)
{
	CString strModuleFileName;
	DWORD dwSize{ _MAX_PATH };   // start with the common path length
	while (true)
	{
		// allocate a buffer large enough for the current attempt
		TCHAR* pszModuleFileName{ strModuleFileName.GetBuffer(dwSize) };
		const DWORD dwResult{ ::GetModuleFileName(nullptr, pszModuleFileName, dwSize) };
		if (dwResult == 0)
		{
			// API failed; propagate the Win32 error and return empty
			if (pdwLastError != nullptr)
				*pdwLastError = GetLastError();
			strModuleFileName.ReleaseBuffer(0);
			return CString{};
		}
		else if (dwResult < dwSize)
		{
			// buffer was large enough; the path fits in dwResult characters
			if (pdwLastError != nullptr)
				*pdwLastError = ERROR_SUCCESS;
			strModuleFileName.ReleaseBuffer(dwResult);
			return strModuleFileName;
		}
		else if (dwResult == dwSize)
		{
			// buffer was too small; double the size and retry
			strModuleFileName.ReleaseBuffer(0);
			dwSize *= 2;
		}
	}
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// obtain the full path of this executable to load version resources
	CString strFullPath{ GetModuleFileName() };
	if (strFullPath.IsEmpty())
#pragma warning(suppress: 26487)
		return FALSE;

	if (m_pVersionInfo.Load(strFullPath.GetString()))
	{
		CString strName = m_pVersionInfo.GetProductName().c_str();
		CString strVersion = m_pVersionInfo.GetProductVersionAsString().c_str();
		// normalise the version string: remove spaces and replace commas with dots
		strVersion.Replace(_T(" "), _T(""));
		strVersion.Replace(_T(","), _T("."));
		// locate the first two dot separators to build a "major.minor" string
		const int nFirst = strVersion.Find(_T('.'));
		const int nSecond = strVersion.Find(_T('.'), nFirst + 1);
		strVersion.Truncate(nSecond);   // keep only major.minor
		// zero-pad single-digit minor versions (e.g. "1.5" -> "1.05")
		if (nSecond == (nFirst + 2))
			strVersion.Insert(nFirst + 1, _T("0"));
#if _WIN32 || _WIN64
#if _WIN64
		m_ctrlVersion.SetWindowText(strName + _T(" version ") + strVersion + _T(" (64-bit)"));
#else
		m_ctrlVersion.SetWindowText(strName + _T(" version ") + strVersion + _T(" (32-bit)"));
#endif
#endif
	}

	m_ctrlWarning.SetWindowText(_T("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>."));

	m_ctrlWebsite.SetHyperLink(_T("https://www.moga.doctor/"));
	m_ctrlEmail.SetHyperLink(_T("mailto:stefan-mihai@moga.doctor"));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

// CWebSearchEngineDlg dialog

/**
 * @brief Constructs the main dialog, loading the application icon and
 *        initialising the thread tracking members.
 * @param pParent Optional pointer to the parent window.
 */
CWebSearchEngineDlg::CWebSearchEngineDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WEBSEARCHENGINE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bThreadRunning = false;
	m_nThreadID = 0;
}

/**
 * @brief Exchanges data between dialog controls and member variables.
 * @param pDX Pointer to the data-exchange object.
 */
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

/**
 * @brief Initializes the dialog: adds social/repository entries to the system
 *        menu, establishes the ODBC connection, creates the database schema,
 *        and launches the background crawling thread.
 * @return TRUE to set input focus to the first control; FALSE if initialization
 *         fails (e.g., connection settings cancelled or ODBC error).
 */
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
			// append separator and the standard About entry
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
		// append social-media links
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_TWITTER, _T("Twitter"));
		pSysMenu->AppendMenu(MF_STRING, IDM_LINKEDIN, _T("LinkedIn"));
		pSysMenu->AppendMenu(MF_STRING, IDM_FACEBOOK, _T("Facebook"));
		pSysMenu->AppendMenu(MF_STRING, IDM_INSTAGRAM, _T("Instagram"));
		// append GitHub repository links
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ISSUES, _T("Issues"));
		pSysMenu->AppendMenu(MF_STRING, IDM_DISCUSSIONS, _T("Discussions"));
		pSysMenu->AppendMenu(MF_STRING, IDM_WIKI, _T("Wiki"));
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	CConnectionSettingsDlg pConnectionSettingsDlg(this);
	if (pConnectionSettingsDlg.DoModal() != IDOK)
		return FALSE;

	// read connection settings persisted in the registry
	CWinApp* pWinApp = AfxGetApp();
	ASSERT(pWinApp != NULL);

	CString strHostName = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTNAME, DEFAULT_HOSTNAME);
	CString strHostPort = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTPORT, DEFAULT_HOSTPORT);
	CString strDatabase = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_DATABASE, DEFAULT_DATABASE);
	// CString strFileName = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_FILENAME, DEFAULT_FILENAME);
	CString strUsername = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_USERNAME, DEFAULT_USERNAME);

	// retrieve the encrypted/stored password from the registry
	TCHAR lpszPassword[0x100] = { 0, };
	VERIFY(GetRegistryPassword(NULL, REGKEY_SECTION, REGKEY_PASSWORD, lpszPassword, DEFAULT_PASSWORD));

	// create the ODBC environment and request ODBC 3.8 behaviour
	SQLRETURN nRet = m_pEnvironment.Create();
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	nRet = m_pEnvironment.SetAttr(SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3_80);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	// enable default ODBC connection pooling to reuse connections
	nRet = m_pEnvironment.SetAttrU(SQL_ATTR_CONNECTION_POOLING, SQL_CP_DEFAULT);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pEnvironment);

	// allocate a connection handle bound to the environment
	nRet = m_pConnection.Create(m_pEnvironment);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pConnection);

	// build the MySQL ODBC connection string from the retrieved settings
	_stprintf(m_sConnectionInString, _T("Driver={MySQL ODBC 8.0 Unicode Driver};Server=%s;Port=%s;Database=%s;User=%s;Password=%s;"),
		strHostName.GetBuffer(0), strHostPort.GetBuffer(0), strDatabase.GetBuffer(0), strUsername.GetBuffer(0), lpszPassword);
	strHostName.ReleaseBuffer();
	strHostPort.ReleaseBuffer();
	strDatabase.ReleaseBuffer();
	strUsername.ReleaseBuffer();
	// open the database connection; the driver fills m_sConnectionOutString on success
	nRet = m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(m_sConnectionInString)), m_sConnectionOutString);
	ODBC_CHECK_RETURN_FALSE(nRet, m_pConnection);

	// reset the UI counters shown on the dialog
	m_pWebpageCounter.SetWindowText(_T("0"));
	m_pKeywordCounter.SetWindowText(_T("0"));

	// (re-)create the database schema from scratch so every run starts clean
	CGenericStatement pGenericStatement;
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `occurrence`;")));  // drop dependant table first
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `keyword`;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("DROP TABLE IF EXISTS `webpage`;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `webpage` (`webpage_id` BIGINT NOT NULL AUTO_INCREMENT, `url` VARCHAR(256) NOT NULL, `title` VARCHAR(256) NOT NULL, `content` LONGTEXT NOT NULL, PRIMARY KEY(`webpage_id`)) ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_general_ci;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `keyword` (`keyword_id` BIGINT NOT NULL AUTO_INCREMENT, `name` VARCHAR(256) NOT NULL, PRIMARY KEY(`keyword_id`)) ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_general_ci;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE TABLE `occurrence` (`webpage_id` BIGINT NOT NULL, `keyword_id` BIGINT NOT NULL, `counter` BIGINT NOT NULL, `pagerank` REAL NOT NULL, PRIMARY KEY(`webpage_id`, `keyword_id`), FOREIGN KEY webpage_fk(webpage_id) REFERENCES webpage(webpage_id), FOREIGN KEY keyword_fk(keyword_id) REFERENCES keyword(keyword_id)) ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_general_ci;")));
	VERIFY(pGenericStatement.Execute(m_pConnection, _T("CREATE UNIQUE INDEX index_name ON `keyword`(`name`);")));  // enforce unique keywords

	// launch the background crawling thread, passing this dialog as context
	m_hThread = ::CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CrawlingThreadProc, this, 0, &m_nThreadID);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

/**
 * @brief Handles WM_SYSCOMMAND messages.
 *
 * Dispatches the About box and opens social-media / repository URLs
 * chosen from the system menu; all other commands are forwarded to the
 * base-class handler.
 *
 * @param nID    System command identifier (IDM_ABOUTBOX, IDM_TWITTER, etc.).
 * @param lParam Additional message-specific data.
 */
void CWebSearchEngineDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// mask the low nibble so IDM_ABOUTBOX variants all resolve correctly
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		// show the About dialog modally
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		if (nID == IDM_TWITTER)   // open author's Twitter/X profile
		{
			::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://x.com/stefanmihaimoga"), nullptr, nullptr, SW_SHOW);
		}
		else
		{
			if (nID == IDM_LINKEDIN)   // open author's LinkedIn profile
			{
				::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.linkedin.com/in/stefanmihaimoga/"), nullptr, nullptr, SW_SHOW);
			}
			else
			{
				if (nID == IDM_FACEBOOK)   // open author's Facebook profile
				{
					::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.facebook.com/stefanmihaimoga"), nullptr, nullptr, SW_SHOW);
				}
				else
				{
					if (nID == IDM_INSTAGRAM)   // open author's Instagram profile
					{
						::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://www.instagram.com/stefanmihaimoga/"), nullptr, nullptr, SW_SHOW);
					}
					else
					{
						if (nID == IDM_ISSUES)   // open GitHub Issues page
						{
							::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/WebSearchEngine/issues"), nullptr, nullptr, SW_SHOW);
						}
						else
						{
							if (nID == IDM_DISCUSSIONS)   // open GitHub Discussions page
							{
								::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/WebSearchEngine/discussions"), nullptr, nullptr, SW_SHOW);
							}
							else
							{
								if (nID == IDM_WIKI)   // open GitHub Wiki page
								{
									::ShellExecute(GetSafeHwnd(), _T("open"), _T("https://github.com/mihaimoga/WebSearchEngine/wiki"), nullptr, nullptr, SW_SHOW);
								}
								else
								{
									// unrecognised command; let the base class handle it (e.g. SC_MOVE, SC_SIZE)
									CDialog::OnSysCommand(nID, lParam);
								}
							}
						}
					}

				}
			}
		}
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

/**
 * @brief Handles WM_PAINT.
 *
 * When the window is minimized the application icon is drawn centred in the
 * client area; otherwise the default CDialogEx painting is performed.
 */
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
/**
 * @brief Returns the cursor displayed while the user drags the minimized window.
 * @return Handle to the application icon cast to HCURSOR.
 */
HCURSOR CWebSearchEngineDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/**
 * @brief Background worker thread that performs web crawling and indexing.
 *
 * Iterates over URLs, fetches HTML content, extracts keywords, and stores
 * results in the connected ODBC database. Updates the dialog controls to
 * reflect crawling progress. Sets m_bThreadRunning to false when finished.
 *
 * @param lpParam Pointer to the owning CWebSearchEngineDlg instance.
 * @return Always returns 0.
 */
DWORD WINAPI CrawlingThreadProc(LPVOID lpParam)
{
	std::string lpszURL, lpszFilename;
	if (lpParam != NULL)
	{
		CWebSearchEngineDlg* pWebSearchEngineDlg = (CWebSearchEngineDlg*)lpParam;
		pWebSearchEngineDlg->m_bThreadRunning = true;   // signal that the thread is active
		pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);  // start indeterminate progress animation
		// seed the frontier with the initial URL
		AddURLToFrontier("https://en.wikipedia.org/");
		while (pWebSearchEngineDlg->m_bThreadRunning)
		{
			// dequeue the next URL from the frontier; stop if the queue is empty
			if (ExtractURLFromFrontier(lpszURL))
			{
				// display the URL currently being processed
				pWebSearchEngineDlg->m_pCrawling.SetWindowText(CString(lpszURL.c_str()));
				// fetch the page content to a temporary file
				if (DownloadURLToFile(lpszURL, lpszFilename))
				{
					// parse HTML, extract keywords, store in DB; abort on error
					if (!ProcessHTML(pWebSearchEngineDlg, lpszFilename, lpszURL))
					{
						break;
					}
				}
			}
			else
				break;  // frontier exhausted
		}

		// crawling finished (normally or due to an error); reset UI state
		pWebSearchEngineDlg->m_bThreadRunning = false;
		pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);  // stop progress animation
	}

	::ExitThread(0);
	return 0;
}

BOOL WaitWithMessageLoop(HANDLE hEvent, DWORD dwTimeout)
{
	DWORD dwRet;
	MSG msg;
	// if no event was supplied, create a dummy one that never fires
	hEvent = hEvent ? hEvent : CreateEvent(NULL, FALSE, FALSE, NULL);

	while (true)
	{
		// wait for the event OR for new window messages, whichever comes first
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
		if (dwRet == WAIT_OBJECT_0)
			return TRUE;  // event signalled; success
		if (dwRet != WAIT_OBJECT_0 + 1)
			break;  // timeout or error
		// a message arrived; drain the queue before waiting again
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// check whether the event fired while processing messages
			if (hEvent && (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0))
				return TRUE;
		}
	}
	return FALSE;
}

/**
 * @brief Handles the Cancel button click.
 *
 * If the crawling thread is still running, signals it to stop by clearing
 * m_bThreadRunning and waits for the thread to terminate before closing
 * the dialog.
 */
void CWebSearchEngineDlg::OnBnClickedCancel()
{
	if (m_bThreadRunning)
	{
		// signal the worker thread to stop by clearing the running flag
		m_bThreadRunning = false;
		// pump messages while waiting so the UI stays responsive
		VERIFY(WaitWithMessageLoop(m_hThread, INFINITE));
	}
	// close the dialog
	CDialogEx::OnCancel();
}
