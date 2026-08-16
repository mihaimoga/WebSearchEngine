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

// WebSearchEngineDlg.h : header file
//

#pragma once

#include "ODBCWrappers.h"

/**
 * @brief Main application dialog for the WebSearchEngine.
 *
 * Manages the web-crawling UI, ODBC database connection,
 * and the background crawling thread.
 */
class CWebSearchEngineDlg : public CDialogEx
{
	// Construction
public:
	/**
	 * @brief Standard constructor.
	 * @param pParent Optional pointer to the parent window.
	 */
	CWebSearchEngineDlg(CWnd* pParent = NULL);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WEBSEARCHENGINE_DIALOG };
#endif
	/** @brief Edit control that displays the URL currently being crawled. */
	CEdit m_pCrawling;
	/** @brief Progress bar reflecting the overall crawling progress. */
	CProgressCtrl m_pProgress;
	/** @brief Static label showing the total number of web pages processed. */
	CStatic m_pWebpageCounter;
	/** @brief Static label showing the total number of keywords indexed. */
	CStatic m_pKeywordCounter;

protected:
	/**
	 * @brief Exchanges data between dialog controls and member variables.
	 * @param pDX Pointer to the data-exchange object.
	 */
	virtual void DoDataExchange(CDataExchange* pDX);

// Implementation
public:
	/** @brief Flag indicating whether the background crawling thread is active. */
	bool m_bThreadRunning;
	/** @brief Handle to the application icon. */
	HICON m_hIcon;
	/** @brief ODBC environment handle. */
	CODBC::CEnvironment m_pEnvironment;
	/** @brief ODBC connection handle. */
	CODBC::CConnection m_pConnection;
	/** @brief Connection string returned by the ODBC driver after a successful connect. */
	CODBC::String m_sConnectionOutString;
	/** @brief Connection string passed to the ODBC driver when connecting. */
	TCHAR m_sConnectionInString[0x100] = { 0, };
	/** @brief Thread identifier of the background crawling thread. */
	DWORD m_nThreadID = 0;
	/** @brief Handle to the background crawling thread. */
	HANDLE m_hThread = nullptr;

protected:
	// Generated message map functions

	/**
	 * @brief Initializes the dialog: sets up the system menu, ODBC connection,
	 *        database schema, and starts the background crawling thread.
	 * @return TRUE to set input focus to the first control; FALSE otherwise.
	 */
	virtual BOOL OnInitDialog();

	/**
	 * @brief Handles WM_SYSCOMMAND messages, dispatching About box and
	 *        social-media / repository links opened via the system menu.
	 * @param nID  System command identifier.
	 * @param lParam Additional message-specific data.
	 */
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	/**
	 * @brief Handles WM_PAINT; draws the icon when the window is minimized.
	 */
	afx_msg void OnPaint();

	/**
	 * @brief Returns the cursor to display while the user drags the minimized window.
	 * @return Handle to the application cursor.
	 */
	afx_msg HCURSOR OnQueryDragIcon();

	/**
	 * @brief Handles the Cancel button click: signals the crawling thread to stop
	 *        and closes the dialog.
	 */
	afx_msg void OnBnClickedCancel();

	DECLARE_MESSAGE_MAP()
};
