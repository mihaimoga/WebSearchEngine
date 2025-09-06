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

/**
 * @file WebSearchEngineExt.h
 * @brief Declarations for URL frontier management, HTML processing, and database accessors
 *        for the WebSearchEngine application.
 */

#ifndef __WEBSEARCHENGINE_H__
#define __WEBSEARCHENGINE_H__

#pragma once

#include "stdafx.h"
#include "ODBCWrappers.h"
#include "WebSearchEngineDlg.h"

 // Type aliases for core data structures used in the search engine
typedef std::vector<std::string> FrontierArray;           ///< List of URLs (frontier)
typedef std::map<std::string, int> FrontierScore;         ///< URL to score mapping
typedef std::map<std::wstring, __int64> WebpageIndex;     ///< Webpage URL to ID mapping
typedef std::map<std::wstring, __int64> KeywordIndex;     ///< Keyword to ID mapping
typedef std::vector<std::wstring> KeywordArray;           ///< List of keywords

/**
 * @brief Adds a new URL to the frontier if not already visited or present.
 * @param lpszURL The URL to add.
 * @return true if added or already present, false on error.
 */
bool AddURLToFrontier(const std::string& lpszURL);

/**
 * @brief Extracts the next URL from the frontier, prioritizing by score.
 * @param[out] lpszURL The extracted URL.
 * @return true if a URL was extracted, false if the frontier is empty.
 */
bool ExtractURLFromFrontier(std::string& lpszURL);

/**
 * @brief Downloads a web page from a URL and saves it to a temporary file.
 * @param lpszURL The URL to download.
 * @param[out] lpszFilename The path to the downloaded file.
 * @return true if the download succeeded, false otherwise.
 */
bool DownloadURLToFile(const std::string& lpszURL, std::string& lpszFilename);

/**
 * @brief Processes an HTML file: extracts title, hyperlinks, and plain text,
 *        updates the database with webpage and keyword information.
 * @param pWebSearchEngineDlg Pointer to the main dialog for UI and database access.
 * @param lpszFilename Path to the HTML file to process.
 * @param lpszURL The URL of the processed page.
 * @return true if processing succeeded, false otherwise.
 */
bool ProcessHTML(CWebSearchEngineDlg* pWebSearchEngineDlg, const std::string& lpszFilename, const std::string& lpszURL);

/**
 * @class CGenericStatement
 * @brief Executes a single SQL statement with no output.
 */
class CGenericStatement
{
public:
	/**
	 * @brief Executes a SQL statement.
	 * @param pDbConnect Database connection.
	 * @param lpszSQL SQL statement to execute.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, LPCTSTR lpszSQL)
	{
		// Implementation in header for simplicity
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(lpszSQL)));
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

/**
 * @class CWebpageInsertAccessor
 * @brief Accessor for inserting a row into the WEBPAGE table.
 */
class CWebpageInsertAccessor
{
public:
	TCHAR m_lpszURL[MAX_URL_LENGTH];      ///< Webpage URL
	TCHAR m_lpszTitle[0x100];             ///< Webpage title
	TCHAR m_lpszContent[0x10000];         ///< Webpage content

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CWebpageInsertAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszURL)
		ODBC_PARAM_ENTRY(2, m_lpszTitle)
		ODBC_PARAM_ENTRY(3, m_lpszContent)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CWebpageInsertAccessor, _T("INSERT INTO `webpage` (`url`, `title`, `content`) VALUES (?, ?, ?);"))

		/**
		 * @brief Clears all fields in the record.
		 */
		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

/**
 * @class CWebpageInsert
 * @brief Executes an INSERT statement for the WEBPAGE table.
 */
class CWebpageInsert : public CODBC::CAccessor<CWebpageInsertAccessor>
{
public:
	/**
	 * @brief Inserts a webpage record into the database.
	 * @param pDbConnect Database connection.
	 * @param pURL Webpage URL.
	 * @param pTitle Webpage title.
	 * @param pContent Webpage content.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, const std::wstring& pURL, const std::wstring& pTitle, const std::wstring& pContent)
	{
		ClearRecord();
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszURL, _countof(m_lpszURL), pURL.c_str());
		_tcscpy_s(m_lpszTitle, _countof(m_lpszTitle), pTitle.c_str());
		_tcscpy_s(m_lpszContent, _countof(m_lpszContent), pContent.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

/**
 * @class CKeywordInsertAccessor
 * @brief Accessor for inserting a row into the KEYWORD table.
 */
class CKeywordInsertAccessor
{
public:
	TCHAR m_lpszName[0x100]; ///< Keyword name

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CKeywordInsertAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszName)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CKeywordInsertAccessor, _T("INSERT INTO `keyword` (`name`) VALUES (?);"))

		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

/**
 * @class CKeywordInsert
 * @brief Executes an INSERT statement for the KEYWORD table.
 */
class CKeywordInsert : public CODBC::CAccessor<CKeywordInsertAccessor>
{
public:
	/**
	 * @brief Inserts a keyword record into the database.
	 * @param pDbConnect Database connection.
	 * @param pKeyword Keyword to insert.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, const std::wstring& pKeyword)
	{
		ClearRecord();
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszName, _countof(m_lpszName), pKeyword.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

/**
 * @class COccurrenceInsertAccessor
 * @brief Accessor for inserting a row into the OCCURRENCE table.
 */
class COccurrenceInsertAccessor
{
public:
	__int64 m_nWebpageID; ///< Webpage ID
	__int64 m_nKeywordID; ///< Keyword ID
	__int64 m_nCounter;   ///< Occurrence count
	double m_rPageRank;   ///< PageRank value

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

		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

/**
 * @class COccurrenceInsert
 * @brief Executes an INSERT statement for the OCCURRENCE table.
 */
class COccurrenceInsert : public CODBC::CAccessor<COccurrenceInsertAccessor>
{
public:
	/**
	 * @brief Inserts an occurrence record into the database.
	 * @param pDbConnect Database connection.
	 * @param nWebpageID Webpage ID.
	 * @param nKeywordID Keyword ID.
	 * @param nCounter Occurrence count.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, const __int64& nWebpageID, const __int64& nKeywordID, const __int64& nCounter)
	{
		ClearRecord();
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

#pragma warning(suppress: 26485)
		m_nWebpageID = nWebpageID;
		m_nKeywordID = nKeywordID;
		m_nCounter = nCounter;
		m_rPageRank = 0.0;
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

/**
 * @class COccurrenceUpdateAccessor
 * @brief Accessor for updating a row in the OCCURRENCE table.
 */
class COccurrenceUpdateAccessor
{
public:
	__int64 m_nWebpageID; ///< Webpage ID
	__int64 m_nKeywordID; ///< Keyword ID

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(COccurrenceUpdateAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_nWebpageID)
		ODBC_PARAM_ENTRY(2, m_nKeywordID)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(COccurrenceUpdateAccessor, _T("UPDATE `occurrence` SET `counter` = `counter` + 1 WHERE `webpage_id` = ? AND `keyword_id` = ?;"))

		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

/**
 * @class COccurrenceUpdate
 * @brief Executes an UPDATE statement for the OCCURRENCE table.
 */
class COccurrenceUpdate : public CODBC::CAccessor<COccurrenceUpdateAccessor>
{
public:
	/**
	 * @brief Updates the occurrence counter for a webpage-keyword pair.
	 * @param pDbConnect Database connection.
	 * @param nWebpageID Webpage ID.
	 * @param nKeywordID Keyword ID.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, const __int64& nWebpageID, const __int64& nKeywordID)
	{
		ClearRecord();
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

#pragma warning(suppress: 26485)
		m_nWebpageID = nWebpageID;
		m_nKeywordID = nKeywordID;
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

/**
 * @class CDataMiningUpdateAccessor
 * @brief Accessor for applying data mining updates to the OCCURRENCE table.
 */
class CDataMiningUpdateAccessor
{
public:
	TCHAR m_lpszName[0x100]; ///< Keyword name

#pragma warning(suppress: 26429)
	BEGIN_ODBC_PARAM_MAP(CDataMiningUpdateAccessor)
		SET_ODBC_PARAM_TYPE(SQL_PARAM_INPUT)
#pragma warning(suppress: 26446 26485 26486 26489)
		ODBC_PARAM_ENTRY(1, m_lpszName)
	END_ODBC_PARAM_MAP()

	DEFINE_ODBC_COMMAND(CDataMiningUpdateAccessor, _T("UPDATE `occurrence` INNER JOIN `keyword` USING(`keyword_id`) SET `pagerank` = data_mining(`webpage_id`, `name`) WHERE `name` = ?;"))

		void ClearRecord() noexcept
	{
		memset(this, 0, sizeof(*this));
	}
};

/**
 * @class CDataMiningUpdate
 * @brief Executes a data mining UPDATE statement for the OCCURRENCE table.
 */
class CDataMiningUpdate : public CODBC::CAccessor<CDataMiningUpdateAccessor>
{
public:
	/**
	 * @brief Applies data mining update for a given keyword.
	 * @param pDbConnect Database connection.
	 * @param pKeyword Keyword to update.
	 * @return true if successful, false otherwise.
	 */
	bool Execute(CODBC::CConnection& pDbConnect, const std::wstring& pKeyword)
	{
		ClearRecord();
		CODBC::CStatement statement;
		SQLRETURN nRet = statement.Create(pDbConnect);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Prepare(GetDefaultCommand());
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

#pragma warning(suppress: 26485)
		_tcscpy_s(m_lpszName, _countof(m_lpszName), pKeyword.c_str());
		nRet = BindParameters(statement);
		ODBC_CHECK_RETURN_FALSE(nRet, statement);

		nRet = statement.Execute();
		ODBC_CHECK_RETURN_FALSE(nRet, statement);
		return true;
	}
};

#endif
