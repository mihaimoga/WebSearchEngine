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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#define _CRT_SECURE_NO_WARNINGS

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#include <afxsock.h>            // MFC socket extensions

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

//Pull in support for ATL
#include <atlbase.h>

//Pull in support for ODBC
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>

//Pull in support for STL
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

#define REGKEY_SECTION _T("Settings")
#define REGKEY_DBTYPE _T("dbtype")
#define REGKEY_HOSTNAME _T("hostname")
#define REGKEY_HOSTPORT _T("hostport")
#define REGKEY_DATABASE _T("database")
#define REGKEY_FILENAME _T("filename")
#define REGKEY_USERNAME _T("username")
#define REGKEY_PASSWORD _T("password")

#define DEFAULT_DBTYPE DB_MYSQL
#define DEFAULT_HOSTNAME _T("localhost")
#define DEFAULT_HOSTPORT _T("3306") /*only for MySQL*/
#define DEFAULT_DATABASE _T("TextMining")
#define DEFAULT_FILENAME _T("")
#define DEFAULT_USERNAME _T("root")
#define DEFAULT_PASSWORD _T("")

#define MAX_URL_LENGTH 0x1000

//Another flavour of an ODBC_CHECK_RETURN macro
#define ODBC_CHECK_RETURN_FALSE(nRet, handle) \
	handle.ValidateReturnValue(nRet); \
	if (!SQL_SUCCEEDED(nRet)) \
{ \
	return false; \
}
