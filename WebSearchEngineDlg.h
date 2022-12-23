/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// WebSearchEngineDlg.h : header file
//

#pragma once

#include "ODBCWrappers.h"
#include "afxwin.h"
#include "afxcmn.h"

// CWebSearchEngineDlg dialog
class CWebSearchEngineDlg : public CDialogEx
{
	// Construction
public:
	CWebSearchEngineDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WEBSEARCHENGINE_DIALOG };
#endif
	CEdit m_pCrawling;
	CProgressCtrl m_pProgress;
	CStatic m_pWebpageCounter;
	CStatic m_pKeywordCounter;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
public:
	bool m_bThreadRunning;
	HICON m_hIcon;
	CODBC::CEnvironment m_pEnvironment;
	CODBC::CConnection m_pConnection;
	CODBC::String m_sConnectionOutString;
	TCHAR m_sConnectionInString[0x100];
	DWORD m_nThreadID;
	HANDLE m_hThread;

protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedCancel();
	DECLARE_MESSAGE_MAP()
};
