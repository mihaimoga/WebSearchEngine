/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// ConnectionSettingsDlg.h : header file
//

#ifndef __CONNECTIONSETTINGSDLG__
#define __CONNECTIONSETTINGSDLG__

#pragma once

#include "Resource.h"

// set the following value to FALSE if you don't want to use Crypto API calls
#define USE_CRYPTO_METHODS FALSE

// maximum length of the password stored as array of BYTEs in Windows Registry
#define PASSWORD_MAXLENGTH 128

BOOL GetRegistryPassword(LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue, LPCTSTR lpszDefault);

BOOL SetRegistryPassword(LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue);

// CConnectionSettingsDlg dialog

class CConnectionSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConnectionSettingsDlg)

public:
	CConnectionSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConnectionSettingsDlg();

	// Dialog Data
	enum { IDD = IDD_CONNECTIONSETTINGSDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	CEdit m_editHostName;
	CEdit m_editHostPort;
	CEdit m_editDatabase;
	CEdit m_editUsername;
	CEdit m_editPassword;

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	DECLARE_MESSAGE_MAP()
};

#endif // __CONNECTIONSETTINGSDLG__
