/* Copyright (C) 2022-2025 Stefan-Mihai MOGA
This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.

WebSearchEngine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

WebSearchEngine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
WebSearchEngine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// ConnectionSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WebSearchEngineDlg.h"
#include "ConnectionSettingsDlg.h"

#include <winreg.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32")
#pragma comment(lib, "advapi32")

// This method is used to display the last error generated by Crypto API calls
void DisplayLastError(LPCTSTR lpszOperation)
{
	//Display a message and the last error in the TRACE. 
	LPVOID lpMsgBuf = nullptr;
	CString	strLastError = _T("[CryptoAPI] ");

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		nullptr);
	strLastError += lpszOperation;
	strLastError += (LPCTSTR)lpMsgBuf;
	//Trim CR/LF from the error message.
	strLastError.TrimRight();

	//Display the last error.
	TRACE(_T("%s\n"), static_cast<LPCWSTR>(strLastError));
	AfxMessageBox(strLastError, MB_OK | MB_ICONERROR);

	// free alocated buffer by FormatMessage
	LocalFree(lpMsgBuf);
} // DisplayLastError(LPCTSTR lpszOperation)

/// Returns a decrypted password read from Windows Registry, using Crypto API calls
bool GetRegistryPassword(LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue, LPCTSTR lpszDefault)
{
	if (!lpszSection || !lpszEntry || !lpszValue)
		return false;

	if (!USE_CRYPTO_METHODS)
	{
		return (_tcscpy(lpszValue, AfxGetApp()->GetProfileString(lpszSection, lpszEntry, lpszDefault)) != NULL);
	}

	if (!lpszCryptoKey)
		lpszCryptoKey = AfxGetAppName();

	LPBYTE lpcbPassword = (LPBYTE)lpszCryptoKey;
	const DWORD dwPasswordLen = (DWORD)(sizeof(TCHAR) * _tcslen(lpszCryptoKey));

	BYTE lpcbDataValue[PASSWORD_MAXLENGTH];
	DWORD dwHowManyBytes = 0;
	LPBYTE lpcbTempBuffer = NULL;

	bool bDecryptionDone = false;
	HCRYPTPROV hCryptoProvider = NULL;
	HCRYPTHASH hCryptoHash = NULL;
	HCRYPTKEY hCryptoKey = NULL;

	if (AfxGetApp()->GetProfileBinary(lpszSection, lpszEntry, (LPBYTE *)&lpcbTempBuffer, (UINT *)&dwHowManyBytes))
	{
		if (dwHowManyBytes != 0)
		{
			ZeroMemory(lpcbDataValue, sizeof(lpcbDataValue));
			CopyMemory(lpcbDataValue, lpcbTempBuffer, dwHowManyBytes);

			if (CryptAcquireContext(&hCryptoProvider, NULL, NULL, PROV_RSA_FULL, 0))
			{
				if (CryptCreateHash(hCryptoProvider, CALG_MD5, NULL, 0, &hCryptoHash))
				{
					if (CryptHashData(hCryptoHash, lpcbPassword, dwPasswordLen, 0))
					{
						if (CryptDeriveKey(hCryptoProvider, CALG_RC4, hCryptoHash, CRYPT_EXPORTABLE, &hCryptoKey))
						{
							if (CryptDecrypt(hCryptoKey, NULL, TRUE, 0, lpcbDataValue, &dwHowManyBytes))
							{
								bDecryptionDone = true;
								_tcscpy(lpszValue, (LPTSTR)lpcbDataValue);
							}
							else
							{
								DisplayLastError(_T("CryptDecrypt: "));
							}
							VERIFY(CryptDestroyKey(hCryptoKey));
						}
						else
						{
							DisplayLastError(_T("CryptDeriveKey: "));
						}
					}
					else
					{
						DisplayLastError(_T("CryptHashData: "));
					}
					VERIFY(CryptDestroyHash(hCryptoHash));
				}
				else
				{
					DisplayLastError(_T("CryptCreateHash: "));
				}
				VERIFY(CryptReleaseContext(hCryptoProvider, 0));
			}
			else
			{
				DisplayLastError(_T("CryptAcquireContext: "));
			}
		}
	}
	else
	{
		if (!dwHowManyBytes)
		{
			_tcscpy(lpszValue, lpszDefault);
			bDecryptionDone = true;
		}
	}

	if (lpcbTempBuffer != NULL)
		delete lpcbTempBuffer;

	return bDecryptionDone;
} // GetRegistryPassword( LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue, LPCTSTR lpszDefault )

/// Writes an encrypted password to Windows Registry, using Crypto API calls
bool SetRegistryPassword(LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue)
{
	if (!lpszSection || !lpszEntry || !lpszValue)
		return false;

	if (!USE_CRYPTO_METHODS)
	{
		return AfxGetApp()->WriteProfileString(lpszSection, lpszEntry, lpszValue);
	}

	if (!lpszCryptoKey)
		lpszCryptoKey = AfxGetAppName();

	LPBYTE lpcbPassword = (LPBYTE)lpszCryptoKey;
	const DWORD dwPasswordLen = (DWORD)(sizeof(TCHAR) * _tcslen(lpszCryptoKey));

	BYTE lpcbDataValue[PASSWORD_MAXLENGTH];
	const DWORD dwDataValueLen = PASSWORD_MAXLENGTH;
	DWORD dwHowManyBytes = dwDataValueLen;

	bool bEncryptionDone = false;
	HCRYPTPROV hCryptoProvider = NULL;
	HCRYPTHASH hCryptoHash = NULL;
	HCRYPTKEY hCryptoKey = NULL;

	ZeroMemory(lpcbDataValue, sizeof(lpcbDataValue));
	CopyMemory(lpcbDataValue, lpszValue, dwDataValueLen);

	if (CryptAcquireContext(&hCryptoProvider, NULL, NULL, PROV_RSA_FULL, 0))
	{
		if (CryptCreateHash(hCryptoProvider, CALG_MD5, NULL, 0, &hCryptoHash))
		{
			if (CryptHashData(hCryptoHash, lpcbPassword, dwPasswordLen, 0))
			{
				if (CryptDeriveKey(hCryptoProvider, CALG_RC4, hCryptoHash, CRYPT_EXPORTABLE, &hCryptoKey))
				{
					if (CryptEncrypt(hCryptoKey, NULL, TRUE, 0, lpcbDataValue, &dwHowManyBytes, dwDataValueLen))
					{
						bEncryptionDone = AfxGetApp()->WriteProfileBinary(lpszSection, lpszEntry, lpcbDataValue, (UINT)dwHowManyBytes);
					}
					else
					{
						DisplayLastError(_T("CryptEncrypt: "));
					}
					VERIFY(CryptDestroyKey(hCryptoKey));
				}
				else
				{
					DisplayLastError(_T("CryptDeriveKey: "));
				}
			}
			else
			{
				DisplayLastError(_T("CryptHashData: "));
			}
			VERIFY(CryptDestroyHash(hCryptoHash));
		}
		else
		{
			DisplayLastError(_T("CryptCreateHash: "));
		}
		VERIFY(CryptReleaseContext(hCryptoProvider, 0));
	}
	else
	{
		DisplayLastError(_T("CryptAcquireContext: "));
	}

	return bEncryptionDone;
} // SetRegistryPassword( LPCTSTR lpszCryptoKey, LPCTSTR lpszSection, LPCTSTR lpszEntry, LPTSTR lpszValue )

// CConnectionSettingsDlg dialog

IMPLEMENT_DYNAMIC(CConnectionSettingsDlg, CDialogEx)

CConnectionSettingsDlg::CConnectionSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConnectionSettingsDlg::IDD, pParent)
{
}

CConnectionSettingsDlg::~CConnectionSettingsDlg()
{
}

void CConnectionSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOSTNAME, m_editHostName);
	DDX_Control(pDX, IDC_HOSTPORT, m_editHostPort);
	DDX_Control(pDX, IDC_DATABASE, m_editDatabase);
	DDX_Control(pDX, IDC_USERNAME, m_editUsername);
	DDX_Control(pDX, IDC_PASSWORD, m_editPassword);
}

BEGIN_MESSAGE_MAP(CConnectionSettingsDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CConnectionSettingsDlg message handlers

BOOL CConnectionSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editHostName.SetLimitText(64);
	m_editHostPort.SetLimitText(64);
	m_editDatabase.SetLimitText(64);
	m_editUsername.SetLimitText(64);
	m_editPassword.SetLimitText(64);

	CString strHostName, strHostPort, strDatabase, strUsername; TCHAR lpszPassword[0x100];

	CWinApp* pWinApp = AfxGetApp();
	ASSERT(pWinApp != NULL);

	strHostName = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTNAME, DEFAULT_HOSTNAME);
	strHostPort = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_HOSTPORT, DEFAULT_HOSTPORT);
	strDatabase = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_DATABASE, DEFAULT_DATABASE);
	strUsername = pWinApp->GetProfileString(REGKEY_SECTION, REGKEY_USERNAME, DEFAULT_USERNAME);

	VERIFY(GetRegistryPassword(NULL, REGKEY_SECTION, REGKEY_PASSWORD, lpszPassword, DEFAULT_PASSWORD));

	m_editHostName.SetWindowText(strHostName);
	m_editHostPort.SetWindowText(strHostPort);
	m_editDatabase.SetWindowText(strDatabase);
	m_editUsername.SetWindowText(strUsername);
	m_editPassword.SetWindowText(lpszPassword);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConnectionSettingsDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
}

void CConnectionSettingsDlg::OnBnClickedOk()
{
	CString strHostName, strHostPort, strDatabase, strUsername, strPassword;

	CWinApp* pWinApp = AfxGetApp();
	ASSERT(pWinApp != NULL);

	m_editHostName.GetWindowText(strHostName);
	m_editHostPort.GetWindowText(strHostPort);
	m_editDatabase.GetWindowText(strDatabase);
	m_editUsername.GetWindowText(strUsername);
	m_editPassword.GetWindowText(strPassword);

	if (strHostName.IsEmpty())
	{
		MessageBox(_T("Error: Server field cannot be empty! Please fill it in and try again."), _T("WebSearchEngine"), MB_OK | MB_ICONWARNING);
		return;
	}

	if (strDatabase.IsEmpty())
	{
		MessageBox(_T("Error: Database field cannot be empty! Please fill it in and try again."), _T("WebSearchEngine"), MB_OK | MB_ICONWARNING);
		return;
	}

	VERIFY(pWinApp->WriteProfileString(REGKEY_SECTION, REGKEY_HOSTNAME, strHostName));
	VERIFY(pWinApp->WriteProfileString(REGKEY_SECTION, REGKEY_HOSTPORT, strHostPort));
	VERIFY(pWinApp->WriteProfileString(REGKEY_SECTION, REGKEY_DATABASE, strDatabase));
	VERIFY(pWinApp->WriteProfileString(REGKEY_SECTION, REGKEY_USERNAME, strUsername));

	VERIFY(SetRegistryPassword(NULL, REGKEY_SECTION, REGKEY_PASSWORD, strPassword.GetBuffer(0)));
	strPassword.ReleaseBuffer();

	CDialogEx::EndDialog(IDOK);
}

void CConnectionSettingsDlg::OnBnClickedCancel()
{
	CDialogEx::EndDialog(IDCANCEL);
	PostQuitMessage(-1);
}
