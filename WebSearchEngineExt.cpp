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

/**
 * @file WebSearchEngineExt.cpp
 * @brief Implements core logic for URL frontier management, HTML processing, and database interaction
 *        for the WebSearchEngine application.
 */

#include "stdafx.h"
#include "WebSearchEngineExt.h"
#include "HtmlToText.h"
#include "ODBCWrappers.h"
#include <string>
#include <vector>
#include <map>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <Windows.h>

#include <Urlmon.h>
#pragma comment(lib, "Urlmon")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

 // Global data structures for URL and keyword management
FrontierArray gFrontierOlder;   ///< List of already visited URLs
FrontierArray gFrontierArray;   ///< Queue of URLs to visit
FrontierScore gFrontierScore;   ///< Score (priority) for each URL in the frontier
WebpageIndex gWebpageID;        ///< Mapping from webpage URL to unique ID
KeywordIndex gKeywordID;        ///< Mapping from keyword to unique ID
KeywordArray gWordArray;        ///< List of all discovered keywords

std::vector<std::wstring> gDataMiningTerms; ///< Terms to be used for data mining

static __int64 gCurrentWebpageID = 0; ///< Counter for assigning unique webpage IDs
static __int64 gCurrentKeywordID = 0; ///< Counter for assigning unique keyword IDs

#define DELIMITERS _T("\t\n\r\"\' !?#$%&|(){}[]*/+-:;<>=.,")

/**
 * @brief Removes HTML quotes from a string.
 * @param InBuffer The input string.
 * @return The unquoted string.
 */
const std::string UnquoteHTML(const std::string& InBuffer);

/**
 * @brief Converts a UTF-8 encoded std::string to std::wstring.
 * @param str The UTF-8 string.
 * @return The corresponding wide string.
 */
std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

/**
 * @brief Converts a std::wstring to a UTF-8 encoded std::string.
 * @param str The wide string.
 * @return The corresponding UTF-8 string.
 */
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

/**
 * @brief Adds a new URL to the frontier if it has not been visited or queued.
 *        Increases the score if already in the queue.
 * @param lpszURL The URL to add.
 * @return true if the operation succeeded.
 */
bool AddURLToFrontier(const std::string& lpszURL)
{
	bool found = false;
	// Check if the URL was already crawled; if so, skip it
	for (auto it = gFrontierOlder.begin(); it != gFrontierOlder.end(); it++)
	{
		if (lpszURL.compare(it->c_str()) == 0)
		{
			return true; // URL already visited
		}
	}
	// Check if the URL is already waiting in the frontier queue
	for (auto it = gFrontierArray.begin(); it != gFrontierArray.end(); it++)
	{
		if (lpszURL.compare(it->c_str()) == 0)
		{
			// URL is already queued; boost its priority score
			found = true;
			gFrontierScore[lpszURL]++;
			break;
		}
	}
	if (!found)
	{
		// New URL: add to frontier queue with initial priority score of 1
		gFrontierArray.push_back(lpszURL);
		gFrontierScore[lpszURL] = 1;
	}
	return true;
}

/**
 * @brief Extracts the next URL from the frontier, prioritizing by score.
 *        Removes the URL from the queue and score map, and adds it to the visited list.
 * @param[out] lpszURL The extracted URL.
 * @return true if a URL was extracted, false if the frontier is empty.
 */
bool ExtractURLFromFrontier(std::string& lpszURL)
{
	lpszURL = "";
	int score = 0;
	if (gFrontierArray.size() > 0)
	{
		for (std::string it : gFrontierArray)
		{
			if (lpszURL.empty()) // select the first element
			{
				lpszURL = it;
				score = gFrontierScore[it];
			}
			else
			{
				if (score < gFrontierScore[it]) // update the selection if necessary
				{
					lpszURL = it;
					score = gFrontierScore[it];
				}
			}
		}

		// remove selected element from frontier
		for (auto it = gFrontierArray.begin(); it != gFrontierArray.end(); it++)
		{
			if (lpszURL.compare(it->c_str()) == 0)
			{
				gFrontierArray.erase(it);
				break;
			}
		}
		for (auto it = gFrontierScore.begin(); it != gFrontierScore.end(); it++)
		{
			if (lpszURL.compare(it->first.c_str()) == 0)
			{
				gFrontierScore.erase(it);
				break;
			}
		}

		gFrontierOlder.push_back(lpszURL);
		return true;
	}
	return false;
}

/**
 * @brief Downloads a web page from a URL and saves it to a temporary file.
 * @param lpszURL The URL to download.
 * @param[out] lpszFilename The path to the downloaded file.
 * @return true if the download succeeded, false otherwise.
 */
bool DownloadURLToFile(const std::string& lpszURL, std::string& lpszFilename)
{
	char lpszTempPath[MAX_PATH + 1] = { 0, };
	char lpszTempFile[MAX_PATH + 1] = { 0, };

	// Retrieve the system temporary directory path
	const DWORD dwTempPath = GetTempPathA(sizeof(lpszTempPath) - 1, lpszTempPath);
	if (dwTempPath > 0)
	{
		lpszTempPath[dwTempPath] = '\0'; // ensure null-termination
		// Generate a unique temporary file name in the temp directory
		if (GetTempFileNameA(lpszTempPath, "map", 0, lpszTempFile) != 0)
		{
			// Download the web page content directly into the temporary file
			if (URLDownloadToFileA(NULL, lpszURL.c_str(), lpszTempFile, 0, NULL) == S_OK)
			{
				lpszFilename = lpszTempFile; // return the temp file path to the caller
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief Finds and replaces all occurrences of a substring in a string.
 * @param data The string to modify.
 * @param toSearch The substring to search for.
 * @param replaceStr The replacement string.
 * @return The number of replacements made.
 */
int findAndReplaceAll(std::wstring& data, std::wstring toSearch, std::wstring replaceStr)
{
	int counter = 0;
	// Get the first occurrence
	size_t pos = data.find(toSearch);

	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		counter++;
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
	return counter;
}

/**
 * @brief Trims whitespace from the start of a string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline std::wstring ltrim(std::wstring s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !iswspace((wint_t)ch);
		}));
	return s;
}

/**
 * @brief Trims whitespace from the end of a string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline std::wstring rtrim(std::wstring s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !iswspace((wint_t)ch);
		}).base(), s.end());
	return s;
}

/**
 * @brief Trims whitespace from both ends of a string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline std::wstring trim(std::wstring s)
{
	return ltrim(rtrim(s));
}

/**
 * @brief Converts a string to lowercase.
 * @param s The string to convert.
 * @return The lowercase string.
 */
static inline std::wstring to_lower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return (wchar_t)std::tolower(c); });
	return s;
}

/**
 * @brief Converts a string to uppercase.
 * @param s The string to convert.
 * @return The uppercase string.
 */
static inline std::wstring to_upper(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](wchar_t c) { return (wchar_t)std::toupper(c); });
	return s;
}

/**
 * @brief Processes an HTML file: extracts the title, hyperlinks, and plain text,
 *        updates the database with webpage and keyword information, and manages data mining terms.
 * @param pWebSearchEngineDlg Pointer to the main dialog for UI and database access.
 * @param lpszFilename Path to the HTML file to process.
 * @param lpszURL The URL of the processed page.
 * @return true if processing succeeded, false otherwise.
 */
bool ProcessHTML(CWebSearchEngineDlg* pWebSearchEngineDlg, const std::string& lpszFilename, const std::string& lpszURL)
{
	CString strMessage;
	CHtmlToText pHtmlToText;
	std::ifstream pHtmlFile(lpszFilename);
	std::string pHtmlContent;
	if (pHtmlFile.is_open())
	{
		// Pre-allocate the string to hold the entire file content, then read it in one pass
		pHtmlFile.seekg(0, std::ios::end);
		pHtmlContent.reserve(static_cast<unsigned int>(pHtmlFile.tellg()));
		pHtmlFile.seekg(0, std::ios::beg);
		pHtmlContent.assign((std::istreambuf_iterator<char>(pHtmlFile)),
			std::istreambuf_iterator<char>());
		pHtmlFile.close();

		// Extract the page title from the <title>...</title> tag
		std::wstring pTitle;
		std::size_t found = pHtmlContent.find("<title>", 0);
		if (std::string::npos != found)
		{
			found += 7; // advance past the opening <title> tag
			const std::size_t last_char = pHtmlContent.find("</title>", found);
			if (std::string::npos != last_char)
			{
				// Decode HTML entities, convert to wide string, trim whitespace, and cap length
				pTitle = trim(utf8_to_wstring(UnquoteHTML(pHtmlContent.substr(found, last_char - found)))).substr(0, 0x100 - 1);
			}
		}
		// Pages without a title are considered invalid; skip further processing
		if (pTitle.length() == 0)
			return true;

		// Scan the HTML for all anchor links and add discovered URLs to the frontier
		found = pHtmlContent.find("<a href=\"", 0);
		while (std::string::npos != found)
		{
			found += 9; // advance past the '<a href="' prefix
			const std::size_t last_char = pHtmlContent.find('\"', found);
			if (std::string::npos != last_char)
			{
				std::string hyperlink = pHtmlContent.substr(found, last_char - found);
				// Strip any fragment identifier (#section) from the URL
				std::size_t diez = hyperlink.find('#');
				if (std::string::npos != diez)
				{
					hyperlink = hyperlink.substr(0, diez);
				}
				// OutputDebugString(CString(hyperlink.c_str()) + _T("\n"));
				TCHAR lpszRelativeURL[MAX_URL_LENGTH] = { 0 };
				TCHAR lpszAbsoluteURL[MAX_URL_LENGTH] = { 0 };
				TCHAR lpszDomainURL[MAX_URL_LENGTH] = { 0 };
				DWORD dwLength = MAX_URL_LENGTH;
				wcscpy_s(lpszRelativeURL, _countof(lpszRelativeURL), CString(hyperlink.c_str()));
				wcscpy_s(lpszDomainURL, _countof(lpszDomainURL), CString(lpszURL.c_str()));
				// Resolve the potentially relative URL against the base page URL
				if (CoInternetCombineUrl(lpszDomainURL, lpszRelativeURL, 0, lpszAbsoluteURL, MAX_URL_LENGTH, &dwLength, 0) == S_OK)
				{
					lpszAbsoluteURL[dwLength] = '\0'; // null-terminate the resolved URL
					// OutputDebugString(CString(lpszAbsoluteURL) + _T("\n"));
					hyperlink = CStringA(lpszAbsoluteURL);
					// Only queue reasonably short URLs to avoid malformed entries
					if (hyperlink.length() < 0x100)
						AddURLToFrontier(hyperlink);
				}
			}
			found = pHtmlContent.find("<a href=\"", found); // search for the next anchor
		}

		const std::wstring& pURL = utf8_to_wstring(lpszURL);
		OutputDebugString(CString(pURL.c_str()) + _T("\n"));
		// OutputDebugString(CString(pTitle.c_str()) + _T("\n"));

		// Convert the raw HTML to plain text and normalize all whitespace to single spaces
		std::wstring pPlainText = trim(utf8_to_wstring(UnquoteHTML(pHtmlToText.Convert(pHtmlContent))));
		findAndReplaceAll(pPlainText, _T("\t"), _T(" ")); // tabs -> space
		findAndReplaceAll(pPlainText, _T("\n"), _T(" ")); // newlines -> space
		findAndReplaceAll(pPlainText, _T("\r"), _T(" ")); // carriage returns -> space
		while (findAndReplaceAll(pPlainText, _T("  "), _T(" ")) > 0); // collapse multiple spaces
		pPlainText = pPlainText.substr(0, 0x10000 - 1); // cap content length for the DB column
		OutputDebugString(CString(pPlainText.c_str()) + _T("\n"));

		SQLRETURN nRet = 0;
		CWebpageInsert pWebpageInsert;
		// Insert the webpage record; on failure reconnect and retry once
		if (!pWebpageInsert.Execute(pWebSearchEngineDlg->m_pConnection, pURL, pTitle, pPlainText)) // add webpage to database
		{
			pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
			// Wait for the DB connection to recover, then reconnect
			do {
				::MessageBeep(0xFFFFFFFF);
				nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
				::Sleep(30 * 1000); // back-off 30 seconds before retrying
				nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
			} while (!SQL_SUCCEEDED(nRet));
			if (!pWebpageInsert.Execute(pWebSearchEngineDlg->m_pConnection, pURL, pTitle, pPlainText))
			{
				pWebSearchEngineDlg->MessageBox(_T("Cannot insert webpage into the database"), _T("Error"), MB_OK);
				return false;
			}
			pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
		}
		// Record the new webpage ID and update the UI counter
		gWebpageID[pURL] = ++gCurrentWebpageID;
		pWebSearchEngineDlg->m_pWebpageCounter.SetWindowText(std::to_wstring(gCurrentWebpageID).c_str());

		const std::wstring pLowerCaseText = to_lower(pPlainText);
		// Skip delimiters at beginning.
		std::size_t lastPos = pLowerCaseText.find_first_not_of(DELIMITERS, 0);
		// Find first "non-delimiter".
		std::size_t pos = pLowerCaseText.find_first_of(DELIMITERS, lastPos);

		while ((std::string::npos != pos) || (std::string::npos != lastPos))
		{
			// Found a token, add it to the vector.
			const std::wstring pKeyword = pLowerCaseText.substr(lastPos, pos - lastPos);
			// Skip delimiters.  Note the "not_of"
			lastPos = pLowerCaseText.find_first_not_of(DELIMITERS, pos);
			// Find next "non-delimiter"
			pos = pLowerCaseText.find_first_of(DELIMITERS, lastPos);

			// Discard empty tokens produced by consecutive delimiters
			if (pKeyword.length() == 0)
				continue;

			// Only index purely alphabetic words; skip tokens with digits or punctuation
			if (pKeyword.find_first_not_of(_T("abcdefghijklmnopqrstuvwxyz")) != std::string::npos)
				continue;

			OutputDebugString(CString(pKeyword.c_str()) + _T("\n"));
			bool already_added = false;
			// Check whether this keyword was encountered in a previous page
			for (auto it = gWordArray.begin(); it != gWordArray.end(); it++)
			{
				if (pKeyword.compare(it->c_str()) == 0)
				{
					already_added = true;
					break;
				}
			}

			if (!already_added)
			{
				// First time seeing this keyword: register it globally and persist it
				gWordArray.push_back(pKeyword);

				CKeywordInsert pKeywordInsert;
				// Insert the new keyword; on failure reconnect and retry once
				if (!pKeywordInsert.Execute(pWebSearchEngineDlg->m_pConnection, pKeyword)) // add keyword to database
				{
					pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
					do {
						::MessageBeep(0xFFFFFFFF);
						nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
						::Sleep(30 * 1000); // back-off 30 seconds before retrying
						nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
					} while (!SQL_SUCCEEDED(nRet));
					if (!pKeywordInsert.Execute(pWebSearchEngineDlg->m_pConnection, pKeyword))
					{
						pWebSearchEngineDlg->MessageBox(_T("Cannot insert keyword into the database"), _T("Error"), MB_OK);
						return false;
					}
					pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
				}
				// Record the new keyword ID and update the UI counter
				gKeywordID[pKeyword] = ++gCurrentKeywordID;
				pWebSearchEngineDlg->m_pKeywordCounter.SetWindowText(std::to_wstring(gCurrentKeywordID).c_str());

				COccurrenceInsert pOccurrenceInsert;
				if (!pOccurrenceInsert.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, gCurrentKeywordID, 1))
				{
					pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
					do {
						::MessageBeep(0xFFFFFFFF);
						nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
						::Sleep(30 * 1000);
						nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
					} while (!SQL_SUCCEEDED(nRet));
					if (!pOccurrenceInsert.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, gCurrentKeywordID, 1))
					{
						pWebSearchEngineDlg->MessageBox(_T("Cannot insert occurrence into the database"), _T("Error"), MB_OK);
						return false;
					}
					pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
				}
			}
			else
			{
				// Keyword already known: try to insert a new occurrence row for this page
				const __int64 nKeywordID = gKeywordID[pKeyword];
				COccurrenceInsert pOccurrenceInsert;
				// If a row already exists (duplicate), fall back to incrementing the counter
				if (!pOccurrenceInsert.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, nKeywordID, 1))
				{
					COccurrenceUpdate pOccurrenceUpdate;
					// Update the existing occurrence counter; on failure reconnect and retry once
					if (!pOccurrenceUpdate.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, nKeywordID))
					{
						pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
						do {
							::MessageBeep(0xFFFFFFFF);
							nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
							::Sleep(30 * 1000); // back-off 30 seconds before retrying
							nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
						} while (!SQL_SUCCEEDED(nRet));
						if (!pOccurrenceUpdate.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, nKeywordID))
						{
							pWebSearchEngineDlg->MessageBox(_T("Cannot update occurrence into the database"), _T("Error"), MB_OK);
							return false;
						}
						pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
					}
				}
			}

			// Accumulate this keyword in the pending data mining term list
			already_added = false;
			for (auto it = gDataMiningTerms.begin(); it != gDataMiningTerms.end(); it++)
			{
				if (pKeyword.compare(it->c_str()) == 0)
				{
					already_added = true;
					break;
				}
			}
			if (!already_added)
				gDataMiningTerms.push_back(pKeyword);
		}

		// Every 1000 crawled pages run data mining to refresh PageRank scores
		if ((gCurrentWebpageID % 1000) == 0)
		{
			for (auto it = gDataMiningTerms.begin(); it != gDataMiningTerms.end(); it++)
			{
				strMessage.Format(_T("applying data mining for '%s'..."), it->c_str());
				pWebSearchEngineDlg->m_pCrawling.SetWindowText(strMessage);

				CDataMiningUpdate pDataMiningUpdate;
				// Apply data mining update; on failure reconnect and retry once
				if (!pDataMiningUpdate.Execute(pWebSearchEngineDlg->m_pConnection, *it))
				{
					pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
					do {
						::MessageBeep(0xFFFFFFFF);
						nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
						::Sleep(30 * 1000); // back-off 30 seconds before retrying
						nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
					} while (!SQL_SUCCEEDED(nRet));
					if (!pDataMiningUpdate.Execute(pWebSearchEngineDlg->m_pConnection, *it))
					{
						pWebSearchEngineDlg->MessageBox(_T("Cannot apply data mining to the database"), _T("Error"), MB_OK);
						return false;
					}
					pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
				}
			}
			gDataMiningTerms.clear(); // reset the list for the next batch
		}
		return true;
	}
	return false;
}
