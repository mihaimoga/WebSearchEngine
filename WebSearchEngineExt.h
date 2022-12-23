/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

#ifndef __WEBSEARCHENGINE_H__
#define __WEBSEARCHENGINE_H__

#pragma once

#include "stdafx.h"
#include "ODBCWrappers.h"
#include "WebSearchEngineDlg.h"

typedef std::vector<std::string> FrontierArray;
typedef std::map<std::string, int> FrontierScore;
typedef std::map<std::wstring, __int64> WebpageIndex;
typedef std::map<std::wstring, __int64> KeywordIndex;
typedef std::vector<std::wstring> KeywordArray;

bool AddURLToFrontier(const std::string& lpszURL);
bool ExtractURLFromFrontier(std::string& lpszURL);
bool DownloadURLToFile(const std::string& lpszURL, std::string& lpszFilename);
bool ProcessHTML(CWebSearchEngineDlg* pWebSearchEngineDlg, const std::string& lpszFilename, const std::string& lpszURL);

class CGenericStatement // execute one SQL statement; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, LPCTSTR lpszSQL)
	{
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
#pragma warning(suppress: 26465 26490 26492)
		nRet = statement.Prepare(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(lpszSQL)));
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

class CWebpageInsertAccessor // sets the data for inserting one row intro WEBPAGE table
{
public:
	// Parameter values
	TCHAR m_lpszURL[MAX_URL_LENGTH];
	TCHAR m_lpszTitle[0x100];
	TCHAR m_lpszContent[0x10000];

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CWebpageInsertAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszURL)
		ODBC_PARAM_ENTRY(2, m_lpszTitle)
		ODBC_PARAM_ENTRY(3, m_lpszContent)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CWebpageInsertAccessor, _T("INSERT INTO `webpage` (`url`, `title`, `content`) VALUES (?, ?, ?);"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

class CWebpageInsert : public CODBC::CAccessor<CWebpageInsertAccessor> // execute INSERT statement for WEBPAGE table; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, const std::wstring& pURL, const std::wstring& pTitle, const std::wstring& pContent)
	{
		ClearRecord();
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Bind the parameters
#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszURL, _countof(m_lpszURL), pURL.c_str());
		_tcscpy_s(m_lpszTitle, _countof(m_lpszTitle), pTitle.c_str());
		_tcscpy_s(m_lpszContent, _countof(m_lpszContent), pContent.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

class CKeywordInsertAccessor // sets the data for inserting one row intro KEYWORD table
{
public:
	// Parameter values
	TCHAR m_lpszName[0x100];

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CKeywordInsertAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszName)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CKeywordInsertAccessor, _T("INSERT INTO `keyword` (`name`) VALUES (?);"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

class CKeywordInsert : public CODBC::CAccessor<CKeywordInsertAccessor> // execute INSERT statement for KEYWORD table; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, const std::wstring& pKeyword)
	{
		ClearRecord();
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Bind the parameters
#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszName, _countof(m_lpszName), pKeyword.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

class COccurrenceInsertAccessor // sets the data for inserting one row intro OCCURRENCE table
{
public:
	// Parameter values
	__int64 m_nWebpageID;
	__int64 m_nKeywordID;
	__int64 m_nCounter;
	double m_rPageRank;

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(COccurrenceInsertAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_nWebpageID)
		ODBC_PARAM_ENTRY(2, m_nKeywordID)
		ODBC_PARAM_ENTRY(3, m_nCounter)
		ODBC_PARAM_ENTRY(4, m_rPageRank)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(COccurrenceInsertAccessor, _T("INSERT INTO `occurrence` (`webpage_id`, `keyword_id`, `counter`, `pagerank`) VALUES (?, ?, ?, ?);"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

class COccurrenceInsert : public CODBC::CAccessor<COccurrenceInsertAccessor> // execute INSERT statement for OCCURRENCE table; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, const __int64& nWebpageID, const __int64& nKeywordID, const __int64& nCounter)
	{
		ClearRecord();
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Bind the parameters
#pragma warning(suppress: 26485)
		m_nWebpageID = nWebpageID;
		m_nKeywordID = nKeywordID;
		m_nCounter = nCounter;
		m_rPageRank = 0.0;
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

class COccurrenceUpdateAccessor // sets the data for updating one row intro OCCURRENCE table
{
public:
	// Parameter values
	__int64 m_nWebpageID;
	__int64 m_nKeywordID;

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(COccurrenceUpdateAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_nWebpageID)
		ODBC_PARAM_ENTRY(2, m_nKeywordID)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(COccurrenceUpdateAccessor, _T("UPDATE `occurrence` SET `counter` = `counter` + 1 WHERE `webpage_id` = ? AND `keyword_id` = ?;"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

class COccurrenceUpdate : public CODBC::CAccessor<COccurrenceUpdateAccessor> // execute UPDATE statement for OCCURRENCE table; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, const __int64& nWebpageID, const __int64& nKeywordID)
	{
		ClearRecord();
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Bind the parameters
#pragma warning(suppress: 26485)
		m_nWebpageID = nWebpageID;
		m_nKeywordID = nKeywordID;
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

class CDataMiningUpdateAccessor // applying Data Mining
{
public:
	// Parameter values
	TCHAR m_lpszName[0x100];

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CDataMiningUpdateAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszName)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CDataMiningUpdateAccessor, _T("UPDATE `occurrence` INNER JOIN `keyword` USING(`keyword_id`) SET `pagerank` = data_mining(`webpage_id`, `name`) WHERE `name` = ?;"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

class CDataMiningUpdate : public CODBC::CAccessor<CDataMiningUpdateAccessor> // execute UPDATE statement for Data Mining; no output returned
{
public:
	// Methods
	BOOLEAN Execute(CODBC::CConnection& pDbConnect, const std::wstring& pKeyword)
	{
		ClearRecord();
		// Create the statement object
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Prepare the statement
		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Bind the parameters
#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszName, _countof(m_lpszName), pKeyword.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		// Execute the statement
		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return TRUE;
	}
};

#endif
