/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

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

FrontierArray gFrontierOlder; // visited URLs
FrontierArray gFrontierArray; // enque/deque URLs
FrontierScore gFrontierScore; // the score of each URL
WebpageIndex gWebpageID; // map of each webpage ID
KeywordIndex gKeywordID; // map of each keyword ID
KeywordArray gWordArray; // list of all keywords

std::vector<std::wstring> gDataMiningTerms;

static __int64 gCurrentWebpageID = 0;
static __int64 gCurrentKeywordID = 0;

#define DELIMITERS _T("\t\n\r\"\' !?#$%&|(){}[]*/+-:;<>=.,")

const std::string UnquoteHTML(const std::string& InBuffer);

// convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

// adds a new URL to frontier
bool AddURLToFrontier(const std::string& lpszURL)
{
	bool found = false;
	for (auto it = gFrontierOlder.begin(); it != gFrontierOlder.end(); it++)
	{
		if (lpszURL.compare(it->c_str()) == 0)
		{
			return true; // URL already visited
		}
	}
	for (auto it = gFrontierArray.begin(); it != gFrontierArray.end(); it++)
	{
		if (lpszURL.compare(it->c_str()) == 0)
		{
			found = true;
			gFrontierScore[lpszURL]++;
			break;
		}
	}
	if (!found)
	{
		gFrontierArray.push_back(lpszURL);
		gFrontierScore[lpszURL] = 1;
	}
	return true;
}

// gets the next URL from frontier
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
				if (score < gFrontierScore[it]) // update the selection if necesary
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

// function to download a Web page
bool DownloadURLToFile(const std::string& lpszURL, std::string& lpszFilename)
{
	char lpszTempPath[MAX_PATH + 1] = { 0, };
	char lpszTempFile[MAX_PATH + 1] = { 0, };

	const DWORD dwTempPath = GetTempPathA(sizeof(lpszTempPath) - 1, lpszTempPath);
	if (dwTempPath > 0)
	{
		lpszTempPath[dwTempPath] = '\0';
		if (GetTempFileNameA(lpszTempPath, "map", 0, lpszTempFile) != 0)
		{
			if (URLDownloadToFileA(NULL, lpszURL.c_str(), lpszTempFile, 0, NULL) == S_OK)
			{
				lpszFilename = lpszTempFile;
				return true;
			}
		}
	}
	return false;
}

// finds and replaces all occurences from a given string
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

// trim from start (in place)
static inline std::wstring ltrim(std::wstring s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !iswspace(ch);
	}));
	return s;
}

// trim from end (in place)
static inline std::wstring rtrim(std::wstring s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !iswspace(ch);
	}).base(), s.end());
	return s;
}

// trim from both ends (in place)
static inline std::wstring trim(std::wstring s)
{
	return ltrim(rtrim(s));
}

// string to lower
static inline std::wstring to_lower(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return s;
}

// string to upper
static inline std::wstring to_upper(std::wstring s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); });
	return s;
}

// process HTML page and extract plain text and hyperlinks
bool ProcessHTML(CWebSearchEngineDlg* pWebSearchEngineDlg, const std::string& lpszFilename, const std::string& lpszURL)
{
	CString strMessage;
	CHtmlToText pHtmlToText;
	std::ifstream pHtmlFile(lpszFilename);
	std::string pHtmlContent;
	if (pHtmlFile.is_open())
	{
		pHtmlFile.seekg(0, std::ios::end);
		pHtmlContent.reserve(pHtmlFile.tellg());
		pHtmlFile.seekg(0, std::ios::beg);

		pHtmlContent.assign((std::istreambuf_iterator<char>(pHtmlFile)),
			std::istreambuf_iterator<char>());
		pHtmlFile.close();

		std::wstring pTitle;
		std::size_t found = pHtmlContent.find("<title>", 0);
		if (std::string::npos != found)
		{
			found += 7;
			const std::size_t last_char = pHtmlContent.find("</title>", found);
			if (std::string::npos != last_char)
			{
				pTitle = trim(utf8_to_wstring(UnquoteHTML(pHtmlContent.substr(found, last_char - found)))).substr(0, 0x100 - 1);
			}
		}
		if (pTitle.length() == 0)
			return true;

		found = pHtmlContent.find("<a href=\"", 0);
		while (std::string::npos != found)
		{
			found += 9;
			const std::size_t last_char = pHtmlContent.find('\"', found);
			if (std::string::npos != last_char)
			{
				std::string hyperlink = pHtmlContent.substr(found, last_char - found);
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
				if (CoInternetCombineUrl(lpszDomainURL, lpszRelativeURL, 0, lpszAbsoluteURL, MAX_URL_LENGTH, &dwLength, 0) == S_OK)
				{
					lpszAbsoluteURL[dwLength] = '\0';
					// OutputDebugString(CString(lpszAbsoluteURL) + _T("\n"));
					hyperlink = CStringA(lpszAbsoluteURL);
					if (hyperlink.length() < 0x100)
						AddURLToFrontier(hyperlink);
				}
			}
			found = pHtmlContent.find("<a href=\"", found);
		}

		const std::wstring& pURL = utf8_to_wstring(lpszURL);
		OutputDebugString(CString(pURL.c_str()) + _T("\n"));
		// OutputDebugString(CString(pTitle.c_str()) + _T("\n"));
		std::wstring& pPlainText = trim(utf8_to_wstring(UnquoteHTML(pHtmlToText.Convert(pHtmlContent))));
		findAndReplaceAll(pPlainText, _T("\t"), _T(" "));
		findAndReplaceAll(pPlainText, _T("\n"), _T(" "));
		findAndReplaceAll(pPlainText, _T("\r"), _T(" "));
		while (findAndReplaceAll(pPlainText, _T("  "), _T(" ")) > 0);
		pPlainText = pPlainText.substr(0, 0x10000 - 1);
		OutputDebugString(CString(pPlainText.c_str()) + _T("\n"));

		SQLRETURN nRet = 0;
		CWebpageInsert pWebpageInsert;
		if (!pWebpageInsert.Execute(pWebSearchEngineDlg->m_pConnection, pURL, pTitle, pPlainText)) // add webpage to database
		{
			pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
			do {
				::MessageBeep(0xFFFFFFFF);
				nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
				::Sleep(30 * 1000);
				nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
			} while (!SQL_SUCCEEDED(nRet));
			if (!pWebpageInsert.Execute(pWebSearchEngineDlg->m_pConnection, pURL, pTitle, pPlainText))
			{
				pWebSearchEngineDlg->MessageBox(_T("Cannot insert webpage into the database"), _T("Error"), MB_OK);
				return false;
			}
			pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
		}
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

			if (pKeyword.length() == 0)
				continue;

			if (pKeyword.find_first_not_of(_T("abcdefghijklmnopqrstuvwxyz")) != std::string::npos)
				continue;

			OutputDebugString(CString(pKeyword.c_str()) + _T("\n"));
			bool already_added = false;
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
				gWordArray.push_back(pKeyword);

				CKeywordInsert pKeywordInsert;
				if (!pKeywordInsert.Execute(pWebSearchEngineDlg->m_pConnection, pKeyword)) // add keyword to database
				{
					pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
					do {
						::MessageBeep(0xFFFFFFFF);
						nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
						::Sleep(30 * 1000);
						nRet = pWebSearchEngineDlg->m_pConnection.DriverConnect(const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(pWebSearchEngineDlg->m_sConnectionInString)), pWebSearchEngineDlg->m_sConnectionOutString);
					} while (!SQL_SUCCEEDED(nRet));
					if (!pKeywordInsert.Execute(pWebSearchEngineDlg->m_pConnection, pKeyword))
					{
						pWebSearchEngineDlg->MessageBox(_T("Cannot insert keyword into the database"), _T("Error"), MB_OK);
						return false;
					}
					pWebSearchEngineDlg->m_pProgress.SetMarquee(TRUE, 30);
				}
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
				const __int64 nKeywordID = gKeywordID[pKeyword];
				COccurrenceInsert pOccurrenceInsert;
				if (!pOccurrenceInsert.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, nKeywordID, 1))
				{
					COccurrenceUpdate pOccurrenceUpdate;
					if (!pOccurrenceUpdate.Execute(pWebSearchEngineDlg->m_pConnection, gCurrentWebpageID, nKeywordID))
					{
						pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
						do {
							::MessageBeep(0xFFFFFFFF);
							nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
							::Sleep(30 * 1000);
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

		if ((gCurrentWebpageID % 1000) == 0)
		{
			for (auto it = gDataMiningTerms.begin(); it != gDataMiningTerms.end(); it++)
			{
				strMessage.Format(_T("applying data mining for '%s'..."), it->c_str());
				pWebSearchEngineDlg->m_pCrawling.SetWindowText(strMessage);

				CDataMiningUpdate pDataMiningUpdate;
				if (!pDataMiningUpdate.Execute(pWebSearchEngineDlg->m_pConnection, *it))
				{
					pWebSearchEngineDlg->m_pProgress.SetMarquee(FALSE, 30);
					do {
						::MessageBeep(0xFFFFFFFF);
						nRet = pWebSearchEngineDlg->m_pConnection.Disconnect();
						::Sleep(30 * 1000);
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
			gDataMiningTerms.clear();
		}
		return true;
	}
	return false;
}
