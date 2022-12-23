/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

// WebSearchEngine.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CWebSearchEngineApp:
// See WebSearchEngine.cpp for the implementation of this class
//

class CWebSearchEngineApp : public CWinApp
{
public:
	CWebSearchEngineApp();

	// Overrides
public:
	virtual BOOL InitInstance();

	// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CWebSearchEngineApp theApp;
