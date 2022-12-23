/*
Module : ODBCWrappers.h
Purpose: Defines the interface for a set of C++ class which encapsulate ODBC 3.x. The classes are loosely
		 based on the design of the ATL OLEDB Consumer Template classes which are provided with
		 Microsoft Visual C++.
History: PJN / 24-09-2011 1. Initial creation
		 PJN / 13-11-2011 1. Initial Public release.
		 PJN / 16-11-2011 1. Updated code to use std::vector instead of std::auto_ptr for allocating SQLTCHAR
						  temp heap arrays. Thanks to Andrey Karpov for reporting this issue.
						  2. BrowseConnect method which returns a string& parameter now uses a TCHAR* parameter
						  instead of a SQLTCHAR* parameter for the first parameter
		 PJN / 20-11-2011 1. Removed final occurrence of std::auto_ptr usage in CStatement::GetCursorName.
		 PJN / 19-02-2013 1. Updated copyright details.
						  2. Updated code to provide default sensible values for the StrLen_or_IndPtr parameter
						  if it is not explicitly provided by client code.
						  3. Updated the sample app to shown examples of inserting into a table, iterating across
						  the rows in a table, updating a row in a table and deleting a row from a table.
						  4. Removed some accidental usage of MFC TRACE and ASSERT functions and replaced with
						  ATL equivalents.
						  5. Pulling in MS SQL Server extensions header file sqlext.h and associated functionality
						  is now optional via a new CODBCWRAPPERS_MSSQL_EXTENSIONS preprocessor value
						  6. Sample app now does not link against MFC for demonstration purpose
		 PJN / 21-02-2013 1. Addition of a new CODBCWRAPPERS_MFC_EXTENSIONS preprocessor value which changes the
						  wrapper classes to more tightly integrate with MFC. Internally the classes will then use
						  the MFC collection classes and expose a CString interface
						  2. Following a customer request the code should now be compilable in VC 6 for those
						  diehards still stuck on this compiler.
		 PJN / 27-02-2015 1. Updated copyright details.
						  2. Addition of a new CCommand class based on the ATL class of the same name. This class
						  should make it easier to port ATL OLEDB code which is currently using ATL::CCommand.
						  Thanks to Serhiy Pavlov for requested this update.
		 PJN / 14-03-2015 1. Added SAL annotations to all the code.
						  2. Renamed the parameters to the methods of the classes to be consistent with the
						  parameter names in the latest Visual Studio 2013 header files.
		 PJN / 05-11-2015 1. Updated code to compile cleanly on VC 2015.
						  2. Fixed compiler warnings in CHandle::SetAttr, CHandle::SetAttrU, CConnection::SetAttr,
						  CConnection::SetAttrU, CStatement::SetAttr & CStatement::SetAttrU related to incorrect
						  reinterpret_cast's.
						  3. Implemented support for SQLCancelHandle via CHandle::Cancel.
						  4. Implemented support for SQLCompleteAsync via CHandle:CompleteAsync
						  5. Implemented support for setting attributes in CCommandBase::SetAttributes,
						  CCommandBase::Create & CCommand::Open. Thanks to Serhiy Pavlov for providing this new feature.
		 PJN / 29-04-2017 1. Updated copyright details.
						  2. Updated the code to compile cleanly using /permissive-.
		 PJN / 03-12-2017 1. Replaced NULL throughout the codebase with nullptr. This means that the minimum requirement
						  for the framework is now VC 2010.
						  2. Replaced BOOL throughout the codebase with bool.
						  3. Replaced CString::operator LPC*STR() calls throughout the codebase with CString::GetString calls
						  4. Renamed CHandle::Cancel method to CHandle::CancelHandle
						  5. Fixed problems in CAccessor::BindColumns and CAccessor::BindParameters with the
						  use of __if_exists and /permissive-.
						  6. Fixed problems in the MFC code path of CDynamicColumnAccessor::BindColumns with
						  the use of "m_ColumnIndicators" and /permissive-

Copyright (c) 2011 - 2017 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise)
when your product is released in binary form. You are allowed to modify the source code in any way you want
except you cannot modify the copyright details at the top of each module. If you want to distribute source
code with your application, then you are only allowed to distribute versions released by the author. This is
to maintain a single distribution point for the source code.

*/


//////////////////////////// Macros / Includes ////////////////////////////////

#ifndef __ODBCWRAPPERS_H__
#define __ODBCWRAPPERS_H__

#ifndef CODBCWRAPPERS_EXT_CLASS
#define CODBCWRAPPERS_EXT_CLASS
#endif //#ifndef CODBCWRAPPERS_EXT_CLASS

#ifndef CODBCWRAPPERS_EXT_API
#define CODBCWRAPPERS_EXT_API
#endif //#ifndef CODBCWRAPPERS_EXT_API

#pragma once

#ifndef __SQL
#pragma message("To avoid this message, please put sql.h in your pre compiled header (normally stdafx.h)")
#include <sql.h>
#endif //#ifndef __SQL

#ifdef CODBCWRAPPERS_MSSQL_EXTENSIONS
#ifndef __SQLEXT
#pragma message("To avoid this message, please put sqlext.h in your pre compiled header (normally stdafx.h)")
#include <sqlext.h>
#endif //#ifndef __SQLEXT
#endif //#ifdef CODBCWRAPPERS_MSSQL_EXTENSIONS


//Handle SAL annotations
#ifndef _Out_writes_opt_
#define _Out_writes_opt_(size)
#endif //#ifndef _Out_writes_opt_

#ifndef _Out_writes_
#define _Out_writes_(size)
#endif //#ifndef _Out_writes_

#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(size)
#endif //#ifndef _In_reads_bytes_opt_

#ifndef _In_reads_
#define _In_reads_(size)
#endif //#ifndef _In_reads_

#ifndef _In_reads_opt_
#define _In_reads_opt_(size)
#endif //#ifndef _In_reads_opt_

#ifndef _Inout_updates_bytes_opt_
#define _Inout_updates_bytes_opt_(size)
#endif //#ifndef _Inout_updates_bytes_opt_

#ifndef _Out_writes_bytes_opt_
#define _Out_writes_bytes_opt_(X)
#endif //#ifndef _Out_writes_bytes_opt_

#ifndef _Inout_updates_opt_
#define _Inout_updates_opt_(size)
#endif //#ifndef _Inout_updates_opt_


#pragma comment(lib, "odbc32.lib")


#define DEFINE_ODBC_SQL_TYPE_FUNCTION(ctype, odbctype) \
	inline SQLSMALLINT _GetODBCSQLType(ctype&) throw() \
  { \
  return odbctype; \
  }

inline SQLSMALLINT _GetODBCSQLType(BYTE[]) throw()
{
	return SQL_VARBINARY;
}
inline SQLSMALLINT _GetODBCSQLType(char[]) throw()
{
	return SQL_VARCHAR;
}
inline SQLSMALLINT _GetODBCSQLType(wchar_t[]) throw()
{
	return SQL_WVARCHAR;
}

DEFINE_ODBC_SQL_TYPE_FUNCTION(char, SQL_CHAR)
DEFINE_ODBC_SQL_TYPE_FUNCTION(wchar_t, SQL_CHAR)
DEFINE_ODBC_SQL_TYPE_FUNCTION(bool, SQL_BIT)
DEFINE_ODBC_SQL_TYPE_FUNCTION(BYTE, SQL_TINYINT)
DEFINE_ODBC_SQL_TYPE_FUNCTION(short, SQL_SMALLINT)
DEFINE_ODBC_SQL_TYPE_FUNCTION(long, SQL_INTEGER)
DEFINE_ODBC_SQL_TYPE_FUNCTION(float, SQL_REAL)
DEFINE_ODBC_SQL_TYPE_FUNCTION(double, SQL_DOUBLE)
DEFINE_ODBC_SQL_TYPE_FUNCTION(__int64, SQL_BIGINT)
DEFINE_ODBC_SQL_TYPE_FUNCTION(TIMESTAMP_STRUCT, SQL_TYPE_TIMESTAMP)
DEFINE_ODBC_SQL_TYPE_FUNCTION(SQL_NUMERIC_STRUCT, SQL_NUMERIC)
DEFINE_ODBC_SQL_TYPE_FUNCTION(GUID, SQL_GUID)

#define DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(ctype, defaultIndicatorValue) \
	inline void _GetODBCDefaultIndicatorValue(ctype&, SQLLEN* pDefaultIndicatorValue) throw() \
{ \
	if (pDefaultIndicatorValue != nullptr) \
	*pDefaultIndicatorValue = defaultIndicatorValue; \
}

inline void _GetODBCDefaultIndicatorValue(BYTE[], SQLLEN* /*pDefaultIndicatorValue*/) throw()
{
	ATLASSERT(false); //If you pass a byte array, then you should explicitly specify the length yourself
}
inline void _GetODBCDefaultIndicatorValue(char[], SQLLEN* pDefaultIndicatorValue) throw()
{
	*pDefaultIndicatorValue = SQL_NTS; //Lets assume null terminated data for ASCII data
}
inline void _GetODBCDefaultIndicatorValue(wchar_t[], SQLLEN* pDefaultIndicatorValue) throw()
{
	*pDefaultIndicatorValue = SQL_NTS; //Lets assume null terminated data for Unicode data
}

DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(char, sizeof(char))
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(wchar_t, sizeof(wchar_t))
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(bool, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(BYTE, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(short, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(long, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(float, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(double, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(__int64, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(TIMESTAMP_STRUCT, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(SQL_NUMERIC_STRUCT, 0)
DEFINE_ODBC_DEFAULT_INDICATOR_VALUE_FUNCTION(GUID, 0)

//The ODBC_CHECK_RETURN macro checks the return value from ODBC calls
#define ODBC_CHECK_RETURN(nRet, handle) \
	handle.ValidateReturnValue(nRet); \
	if ((nRet != SQL_SUCCESS) && (nRet != SQL_SUCCESS_WITH_INFO)) \
	return nRet;

	//The ODBC_CHECK_RETURN_PTR macro checks the return value from ODBC calls
#define ODBC_CHECK_RETURN_PTR(nRet, handle) \
	handle->ValidateReturnValue(nRet); \
	if ((nRet != SQL_SUCCESS) && (nRet != SQL_SUCCESS_WITH_INFO)) \
	return nRet;

#define DEFINE_ODBC_C_TYPE_FUNCTION(ctype, odbctype) \
	inline SQLSMALLINT _GetODBCCType(ctype&) throw() \
{ \
	return odbctype; \
}
	inline SQLSMALLINT _GetODBCCType(BYTE[]) throw()
{
	return SQL_C_BINARY;
}
inline SQLSMALLINT _GetODBCCType(char[]) throw()
{
	return SQL_C_CHAR;
}
inline SQLSMALLINT _GetODBCCType(wchar_t[]) throw()
{
	return SQL_C_WCHAR;
}

DEFINE_ODBC_C_TYPE_FUNCTION(char, SQL_C_CHAR)
DEFINE_ODBC_C_TYPE_FUNCTION(wchar_t, SQL_C_WCHAR)
DEFINE_ODBC_C_TYPE_FUNCTION(bool, SQL_C_BIT)
DEFINE_ODBC_C_TYPE_FUNCTION(BYTE, SQL_C_TINYINT)
DEFINE_ODBC_C_TYPE_FUNCTION(short, SQL_C_SSHORT)
DEFINE_ODBC_C_TYPE_FUNCTION(long, SQL_C_SLONG)
DEFINE_ODBC_C_TYPE_FUNCTION(float, SQL_C_FLOAT)
DEFINE_ODBC_C_TYPE_FUNCTION(double, SQL_C_DOUBLE)
DEFINE_ODBC_C_TYPE_FUNCTION(__int64, SQL_C_SBIGINT)
DEFINE_ODBC_C_TYPE_FUNCTION(TIMESTAMP_STRUCT, SQL_C_TYPE_TIMESTAMP)
DEFINE_ODBC_C_TYPE_FUNCTION(SQL_NUMERIC_STRUCT, SQL_C_NUMERIC)
DEFINE_ODBC_C_TYPE_FUNCTION(GUID, SQL_C_GUID)

#define _ODBC_C_TYPE(data) _GetODBCCType((static_cast<_classtype*>(nullptr))->data)
#define _ODBC_SQL_TYPE(data) _GetODBCSQLType((static_cast<_classtype*>(nullptr))->data)
#define _ODBC_SIZE_TYPE(data) sizeof((static_cast<_classtype*>(nullptr))->data)


#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
#define BEGIN_ODBC_PARAM_MAP(x) \
  public: \
  typedef x _classtype; \
  SQLRETURN _GetBindParametersEntries(CODBC::CStatement* pStmt, int* pnColumns, CArray<SQLLEN, SQLLEN>* pIndicators) \
{ \
	(pStmt); \
	(pIndicators); \
	SQLSMALLINT ioType = SQL_PARAM_INPUT; \
	(ioType); \
	SQLRETURN nRet = SQL_SUCCESS; \
	if (pnColumns != nullptr) \
	*pnColumns = 0;
#else
#define BEGIN_ODBC_PARAM_MAP(x) \
  public: \
  typedef x _classtype; \
  SQLRETURN _GetBindParametersEntries(CODBC::CStatement* pStmt, int* pnColumns, std::vector<SQLLEN>* pIndicators) \
{ \
	(pStmt); \
	(pIndicators); \
	SQLSMALLINT ioType = SQL_PARAM_INPUT; \
	(ioType); \
	SQLRETURN nRet = SQL_SUCCESS; \
	if (pnColumns != nullptr) \
	*pnColumns = 0;
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

#define END_ODBC_PARAM_MAP() \
	return nRet; \
}

#define _ODBC_PARAM_ENTRY_CODE(ParameterNumber, ValueType, ParameterType, ColumnSize, DecimalDigits, ParameterValuePtr, BufferLength, StrLen_or_IndPtr, data) \
	if (pnColumns != nullptr) \
	*pnColumns = ParameterNumber; \
	if (pStmt != nullptr) \
{ \
	SQLLEN* pIndicator = StrLen_or_IndPtr; \
	if (pIndicator == nullptr) \
{ \
	ATLASSUME(pIndicators != nullptr); \
	SQLLEN& nParameterIndicator = (*pIndicators)[ParameterNumber - 1]; \
	_GetODBCDefaultIndicatorValue(data, &nParameterIndicator); \
	pIndicator = &nParameterIndicator; \
} \
	nRet = pStmt->BindParameter(ParameterNumber, ioType, ValueType, ParameterType, ColumnSize, DecimalDigits, ParameterValuePtr, BufferLength, pIndicator); \
	ODBC_CHECK_RETURN_PTR(nRet, pStmt); \
}

#define ODBC_PARAM_ENTRY(ParameterNumber, data) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), _ODBC_SIZE_TYPE(data), 0, &data, _ODBC_SIZE_TYPE(data), nullptr, data);

#define ODBC_PARAM_LENGTH(ParameterNumber, data, length) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), length, 0, &data, length, nullptr, data);

#define ODBC_PARAM_ENTRY_TYPE(ParameterNumber, data, cType, SQLType) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, cType, sqlType, _ODBC_SIZE_TYPE(data), 0, &data, _ODBC_SIZE_TYPE(data), nullptr, data);

#define ODBC_PARAM_ENTRY_STATUS(ParameterNumber, data, status) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), _ODBC_SIZE_TYPE(data), 0, &data, _ODBC_SIZE_TYPE(data), &status, data);

#define ODBC_PARAM_LENGTH_STATUS(ParameterNumber, data, length, status) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), length, 0, &data, length, &status, data);

#define ODBC_PARAM_ENTRY_PS(ParameterNumber, nPrecision, data) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), _ODBC_SIZE_TYPE(data), nPrecision, &data, _ODBC_SIZE_TYPE(data), nullptr, data);

#define ODBC_PARAM_PS_LENGTH(ParameterNumber, data, nPrecision, length) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), length, nPrecision, &data, length, nullptr, data);

#define ODBC_PARAM_ENTRY_PS_TYPE(ParameterNumber, data, nPrecision, cType, SQLType) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, cType, sqlType, _ODBC_SIZE_TYPE(data), nPrecision, &data, _ODBC_SIZE_TYPE(data), nullptr, data);

#define ODBC_PARAM_ENTRY_PS_STATUS(ParameterNumber, data, nPrecision, status) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), _ODBC_SIZE_TYPE(data), nPrecision, &data, _ODBC_SIZE_TYPE(data), &status, data);

#define ODBC_PARAM_PS_LENGTH_STATUS(ParameterNumber, data, nPrecision, length, status) \
	_ODBC_PARAM_ENTRY_CODE(ParameterNumber, _ODBC_C_TYPE(data), _ODBC_SQL_TYPE(data), length, nPrecision, &data, length, &status, data);

#define SET_ODBC_PARAM_TYPE(type) \
	ioType = type;

#define DEFINE_ODBC_COMMAND(x, szCommand) \
	static SQLTCHAR* GetDefaultCommand() throw() \
{ \
	return const_cast<SQLTCHAR*>(reinterpret_cast<const SQLTCHAR*>(szCommand)); \
}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
#define BEGIN_ODBC_COLUMN_MAP(x) \
	public: \
	typedef x _classtype; \
	SQLRETURN _GetBindColumnEntries(CODBC::CStatement* pStmt, int* pnColumns, CArray<SQLLEN, SQLLEN>* pIndicators) \
{ \
	(pStmt); \
	(pIndicators); \
	SQLRETURN nRet = SQL_SUCCESS; \
	if (pnColumns != nullptr) \
	*pnColumns = 0;
#else
#define BEGIN_ODBC_COLUMN_MAP(x) \
	public: \
	typedef x _classtype; \
	SQLRETURN _GetBindColumnEntries(CODBC::CStatement* pStmt, int* pnColumns, std::vector<SQLLEN>* pIndicators) \
{ \
	(pStmt); \
	(pIndicators); \
	SQLRETURN nRet = SQL_SUCCESS; \
	if (pnColumns != nullptr) \
	*pnColumns = 0;
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

#define END_ODBC_COLUMN_MAP() \
	return nRet; \
}

#define _ODBC_COLUMN_ENTRY_CODE(ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind) \
	if (pnColumns != nullptr) \
	*pnColumns = ColumnNumber; \
	if (pStmt != nullptr) \
{ \
	SQLLEN* pIndicator = StrLen_or_Ind; \
	if (pIndicator == nullptr) \
{ \
	ATLASSUME(pIndicators != nullptr); \
	SQLLEN& nParameterIndicator = (*pIndicators)[ColumnNumber - 1]; \
	pIndicator = &nParameterIndicator; \
} \
	nRet = pStmt->BindCol(ColumnNumber, TargetType, TargetValuePtr, BufferLength, pIndicator); \
	ODBC_CHECK_RETURN_PTR(nRet, pStmt); \
}

#define ODBC_COLUMN_ENTRY(ColumnNumber, data) \
	_ODBC_COLUMN_ENTRY_CODE(ColumnNumber, _ODBC_C_TYPE(data), &data, _ODBC_SIZE_TYPE(data), nullptr);

#define ODBC_COLUMN_ENTRY_LENGTH(ColumnNumber, data, length) \
	_ODBC_COLUMN_ENTRY_CODE(ColumnNumber, _ODBC_C_TYPE(data), &data, length, nullptr);

#define ODBC_COLUMN_ENTRY_TYPE(ColumnNumber, data, type) \
	_ODBC_COLUMN_ENTRY_CODE(ColumnNumber, type, &data, _ODBC_SIZE_TYPE(data), nullptr);

#define ODBC_COLUMN_ENTRY_STATUS(ColumnNumber, data, status) \
	_ODBC_COLUMN_ENTRY_CODE(ColumnNumber, _ODBC_C_TYPE(data), &data, _ODBC_SIZE_TYPE(data), &status);

#define ODBC_COLUMN_ENTRY_LENGTH_STATUS(ColumnNumber, data, length, status) \
	_ODBC_COLUMN_ENTRY_CODE(ColumnNumber, _ODBC_C_TYPE(data), &data, length, &status);


/////////////////////////// Classes ///////////////////////////////////////////

namespace CODBC
{
#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
	typedef CString String;
#else
#ifdef _UNICODE
	typedef std::wstring String;
#else
	typedef std::string String;
#endif //#ifdef _UNICODE
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
	inline void CODBCWRAPPERS_EXT_API MultiSZToStringVector(_In_z_ const SQLTCHAR* pszString, _Out_ CArray<String, String&>& elements)
	{
		//Clear the vector
		elements.RemoveAll();

		//Add each element we find
		const TCHAR* pszValue = reinterpret_cast<const TCHAR*>(pszString);
		while (*pszValue != _T('\0'))
		{
			CString sValue(pszValue);
			elements.Add(sValue);
			pszValue += (_tcslen(pszValue) + 1);
		}
	}
#else
	inline void CODBCWRAPPERS_EXT_API MultiSZToStringVector(_In_z_ const SQLTCHAR* pszString, _Out_ std::vector<String>& elements)
	{
		//Clear the vector
		elements.clear();

		//Add each element we find
		const TCHAR* pszValue = reinterpret_cast<const TCHAR*>(pszString);
		while (*pszValue != _T('\0'))
		{
			elements.push_back(pszValue);
			pszValue += (_tcslen(pszValue) + 1);
		}
	}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS


	//Class which encapsulates a generic ODBC handle
	class CODBCWRAPPERS_EXT_CLASS CHandle
	{
	public:
		//Constructors / Destructors
		CHandle() : m_h(SQL_NULL_HANDLE),
			m_Type(0)
		{
		}

		CHandle(_In_ CHandle& h) : m_h(SQL_NULL_HANDLE),
			m_Type(0)
		{
			Attach(h.m_Type, h.Detach());
		}

		explicit CHandle(_In_ SQLSMALLINT Type, _In_opt_ SQLHANDLE h) : m_h(h),
			m_Type(Type)
		{
		}

		~CHandle()
		{
			if (m_h != SQL_NULL_HANDLE)
				Close();
		}

		CHandle& operator=(_In_ CHandle& h)
		{
			if (this != &h)
			{
				if (m_h != SQL_NULL_HANDLE)
					Close();
				Attach(h.m_Type, h.Detach());
			}

			return *this;
		}

		// Methods
		SQLRETURN Create(_In_ SQLSMALLINT Type, _In_opt_ SQLHANDLE InputHandle)
		{
			//Validate our parameters
			ATLASSERT(m_h == SQL_NULL_HANDLE);

			m_Type = Type;
			return SQLAllocHandle(Type, InputHandle, &m_h);
		}

		void Close()
		{
			if (m_h != SQL_NULL_HANDLE)
			{
				SQLFreeHandle(m_Type, m_h);
				m_h = SQL_NULL_HANDLE;
				m_Type = 0;
			}
		}

		operator SQLHANDLE() const
		{
			return m_h;
		}

		void Attach(_In_ SQLSMALLINT Type, _In_ SQLHANDLE h)
		{
			//Validate our parameters
			ATLASSERT(m_h == SQL_NULL_HANDLE);

			m_Type = Type;
			m_h = h;
		}

		SQLHANDLE Detach()
		{
			SQLHANDLE h = m_h;
			m_h = SQL_NULL_HANDLE;
			m_Type = 0;
			return h;
		}

		SQLRETURN GetDiagField(_In_ SQLSMALLINT iRecord, _In_ SQLSMALLINT fDiagField, _Out_writes_opt_(_Inexpressible_(cbBufferLength)) SQLPOINTER rgbDiagInfo, _In_ SQLSMALLINT cbBufferLength, _Out_opt_ SQLSMALLINT* pcbStringLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetDiagField(m_Type, m_h, iRecord, fDiagField, rgbDiagInfo, cbBufferLength, pcbStringLength);
		}

		SQLRETURN GetDiagRec(_In_ SQLSMALLINT iRecord, _Out_writes_opt_(6) SQLTCHAR* szSqlState, SQLINTEGER* pfNativeError, _Out_writes_opt_(cchErrorMsgMax) SQLTCHAR* szErrorMsg, _In_ SQLSMALLINT cchErrorMsgMax, _Out_opt_ SQLSMALLINT* pcchErrorMsg)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetDiagRec(m_Type, m_h, iRecord, szSqlState, pfNativeError, szErrorMsg, cchErrorMsgMax, pcchErrorMsg);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		void GetDiagRecords(_Out_ CArray<String, String&>& records)
		{
			//clear the vector
			records.RemoveAll();

			bool bContinue = true;
			for (SQLSMALLINT i = 1; bContinue; i++)
			{
				SQLTCHAR szState[6];
				szState[0] = _T('\0');
				SQLINTEGER nNativeError = 0;
				SQLTCHAR szMessageText[SQL_MAX_MESSAGE_LENGTH];
				szMessageText[0] = _T('\0');
				SQLSMALLINT nTextLength = 0;
				SQLRETURN nGetDiagRet = GetDiagRec(i, szState, &nNativeError, szMessageText, sizeof(szMessageText) / sizeof(SQLTCHAR), &nTextLength);
				if (nGetDiagRet == SQL_SUCCESS)
				{
					CString sLine;
					sLine.Format(_T("State:%s, Native Error Code:%d, Message Text:%s\n"), szState, nNativeError, szMessageText);
					records.Add(sLine);
				}
				else if (nGetDiagRet == SQL_NO_DATA)
					bContinue = false;
			}
		}
#else
		void GetDiagRecords(_Out_ std::vector<String>& records)
		{
			//clear the vector
			records.clear();

			bool bContinue = true;
			for (SQLSMALLINT i = 1; bContinue; i++)
			{
				SQLTCHAR szState[6];
				szState[0] = _T('\0');
				SQLINTEGER nNativeError = 0;
				SQLTCHAR szMessageText[SQL_MAX_MESSAGE_LENGTH];
				szMessageText[0] = _T('\0');
				SQLSMALLINT nTextLength = 0;
				SQLRETURN nGetDiagRet = GetDiagRec(i, szState, &nNativeError, szMessageText, sizeof(szMessageText) / sizeof(SQLTCHAR), &nTextLength);
				if (nGetDiagRet == SQL_SUCCESS)
				{
					TCHAR szLine[SQL_MAX_MESSAGE_LENGTH + 64];
					szLine[0] = _T('\0');
#ifdef _stprintf_s
					_stprintf_s(szLine, sizeof(szLine) / sizeof(TCHAR), _T("State:%s, Native Error Code:%d, Message Text:%s\n"), szState, nNativeError, szMessageText);
#else
					_stprintf(szLine, _T("State:%s, Native Error Code:%d, Message Text:%s\n"), szState, nNativeError, szMessageText);
#endif
					records.push_back(szLine);
				}
				else if (nGetDiagRet == SQL_NO_DATA)
					bContinue = false;
			}
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

#if (ODBCVER >= 0x0380)
		__if_exists(SQLCompleteAsync)
		{
			SQLRETURN CancelHandle()
			{
				//Validate our parameters
				ATLASSERT(m_h != SQL_NULL_HANDLE);

				return SQLCancelHandle(m_Type, m_h);
			}
		}
#endif //#if (ODBCVER >= 0x0380)

#if (ODBCVER >= 0x0380)
		__if_exists(SQLCompleteAsync)
		{
			SQLRETURN CompleteAsync(_Out_ RETCODE* AsyncRetCodePtr)
			{
				//Validate our parameters
				ATLASSERT(m_h != SQL_NULL_HANDLE);

				return SQLCompleteAsync(m_Type, m_h, AsyncRetCodePtr);
			}
		}
#endif //#if (ODBCVER >= 0x0380)

		void ValidateReturnValue(_In_ SQLRETURN nRet)
		{
#ifdef _DEBUG
			if ((nRet != SQL_SUCCESS) && (nRet != SQL_SUCCESS_WITH_INFO))
			{
				//Report the bad return value
				ATLTRACE(_T("ODBC API Failure, Return Value:%d\n"), nRet);

				//Report all the diag records
				bool bContinue = true;
				for (SQLSMALLINT i = 1; bContinue; i++)
				{
					SQLTCHAR szState[6];
					szState[0] = _T('\0');
					SQLINTEGER nNativeError = 0;
					SQLTCHAR szMessageText[SQL_MAX_MESSAGE_LENGTH];
					szMessageText[0] = _T('\0');
					SQLSMALLINT nTextLength = 0;
					SQLRETURN nGetDiagRet = GetDiagRec(i, szState, &nNativeError, szMessageText, sizeof(szMessageText) / sizeof(SQLTCHAR), &nTextLength);
					if (nGetDiagRet == SQL_SUCCESS)
					{
#ifdef CODBCWRAPPERS_MSSQL_EXTENSIONS
						//Report on the MS SQL Extension values if necessary
						SQLLEN nRowNumber = 0;
						GetDiagField(i, SQL_DIAG_ROW_NUMBER, &nRowNumber, SQL_IS_INTEGER, nullptr);

						USHORT nLineNumber = 0;
						GetDiagField(i, SQL_DIAG_SS_LINE, &nLineNumber, SQL_IS_INTEGER, nullptr);

						SDWORD nMsgState = 0;
						GetDiagField(i, SQL_DIAG_SS_MSGSTATE, &nMsgState, SQL_IS_INTEGER, nullptr);

						SDWORD nSeverity = 0;
						GetDiagField(i, SQL_DIAG_SS_SEVERITY, &nSeverity, SQL_IS_INTEGER, nullptr);

						TCHAR szProcName[4096];
						szProcName[0] = _T('\0');
						SQLSMALLINT nProcName = 0;
						GetDiagField(i, SQL_DIAG_SS_PROCNAME, &szProcName, sizeof(szProcName), &nProcName);

#ifdef SQL_MAX_SQLSEVERNAME
						TCHAR szServName[SQL_MAX_SQLSERVERNAME];
#else
						TCHAR szServName[128];
#endif
						szServName[0] = _T('\0');
						SQLSMALLINT nServName = 0;
						GetDiagField(i, SQL_DIAG_SS_SRVNAME, &szServName, sizeof(szServName), &nServName);

						if (_tcslen(szProcName))
							ATLTRACE(_T(" Diag Record: State:%s, Native Error Code:%d, Message Text:%s, Row Number:%d, Message State:%d, Severity:%d, Proc Name:%s, Line Number:%d, Server Name:%s\n"),
								szState, nNativeError, szMessageText, nRowNumber, nMsgState, nSeverity, szProcName, nLineNumber, szServName);
						else
							ATLTRACE(_T(" Diag Record details: State:%s, Native Error Code:%d, Message Text:%s, Row Number:%d, Message State:%d, Severity:%d, Server Name:%s\n"),
								szState, nNativeError, szMessageText, nRowNumber, nMsgState, nSeverity, szServName);
#else       
						ATLTRACE(_T(" Diag Record: State:%s, Native Error Code:%d, Message Text:%s\n"), szState, nNativeError, szMessageText);
#endif //CODBCWRAPPERS_MSSQL_EXTENSIONS
					}
					else if (nGetDiagRet == SQL_NO_DATA)
						bContinue = false;
				}
			}
#else
			UNREFERENCED_PARAMETER(nRet);
#endif //#ifdef _DEBUG
		}

		//Member variables
		SQLHANDLE m_h;
		SQLSMALLINT m_Type;
	};


	//Class which encapsulates an ODBC environment handle
	class CEnvironment : public CHandle
	{
	public:
		// Methods
		SQLRETURN Create()
		{
			//Delegate to the base class
			return CHandle::Create(SQL_HANDLE_ENV, SQL_NULL_HANDLE);
		}

		SQLRETURN DataSources(_In_ SQLUSMALLINT fDirection, _Out_writes_opt_(cchDSNMax) SQLTCHAR* szDSN, _In_ SQLSMALLINT cchDSNMax, _Out_opt_ SQLSMALLINT* pcchDSN,
			_Out_writes_opt_(cchDescriptionMax) SQLTCHAR* szDescription, _In_ SQLSMALLINT cchDescriptionMax, _Out_opt_ SQLSMALLINT* pcchDescription)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLDataSources(m_h, fDirection, szDSN, cchDSNMax, pcchDSN, szDescription, cchDescriptionMax, pcchDescription);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN DataSources(_In_ SQLUSMALLINT fDirection, _Out_ String& sDSN, _Out_ String& sDescription)
		{
			SQLSMALLINT nDSNLength = 0;
			SQLSMALLINT nDescLength = 0;
			SQLRETURN nRet = DataSources(fDirection, nullptr, 0, &nDSNLength, nullptr, 0, &nDescLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszTempDSN = reinterpret_cast<SQLTCHAR*>(sDSN.GetBufferSetLength(nDSNLength + 1));
				SQLTCHAR* pszTempDesc = reinterpret_cast<SQLTCHAR*>(sDescription.GetBufferSetLength(nDescLength + 1));
				nRet = DataSources(fDirection, pszTempDSN, static_cast<SQLSMALLINT>(nDSNLength + 1), &nDSNLength, pszTempDesc, static_cast<SQLSMALLINT>(nDescLength + 1), &nDescLength);
				sDescription.ReleaseBuffer();
				sDSN.ReleaseBuffer();
			}

			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN DataSources(_In_ SQLUSMALLINT Direction, _Out_ String& sDSN, _Out_ String& sDescription)
		{
			SQLSMALLINT nDSNLength = 0;
			SQLSMALLINT nDescLength = 0;
			SQLRETURN nRet = DataSources(Direction, nullptr, 0, &nDSNLength, nullptr, 0, &nDescLength);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempDSN;
				sTempDSN.resize(nDSNLength + 1);
				std::vector<SQLTCHAR> sTempDesc;
				sTempDesc.resize(nDescLength + 1);
				nRet = DataSources(Direction, &(sTempDSN[0]), nDSNLength + 1, &nDSNLength, &(sTempDesc[0]), nDescLength + 1, &nDescLength);
				if (SQL_SUCCEEDED(nRet))
				{
					sDSN = reinterpret_cast<TCHAR*>(&(sTempDSN[0]));
					sDescription = reinterpret_cast<TCHAR*>(&(sTempDesc[0]));
				}
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN Drivers(_In_ SQLUSMALLINT fDirection, _Out_writes_opt_(cchDriverDescMax) SQLTCHAR* szDriverDesc, _In_ SQLSMALLINT cchDriverDescMax, _Out_opt_ SQLSMALLINT* pcchDriverDesc,
			_Out_writes_opt_(cchDrvrAttrMax) SQLTCHAR* szDriverAttributes, _In_ SQLSMALLINT cchDrvrAttrMax, _Out_opt_ SQLSMALLINT* pcchDrvrAttr)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLDrivers(m_h, fDirection, szDriverDesc, cchDriverDescMax, pcchDriverDesc, szDriverAttributes, cchDrvrAttrMax, pcchDrvrAttr);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN Drivers(_In_ SQLUSMALLINT fDirection, _Out_ String& sDriverDesc, _Out_ CArray<String, String&>& attributes)
		{
			SQLSMALLINT nDescriptionLength = 0;
			SQLSMALLINT nAttributesLength = 0;
			SQLRETURN nRet = Drivers(fDirection, nullptr, 0, &nDescriptionLength, nullptr, 0, &nAttributesLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszTempDriverDesc = reinterpret_cast<SQLTCHAR*>(sDriverDesc.GetBufferSetLength(nDescriptionLength + 1));
				CString sTempAttributes;
				SQLTCHAR* pszTempAttributes = reinterpret_cast<SQLTCHAR*>(sTempAttributes.GetBufferSetLength(nAttributesLength + 1));
				nRet = Drivers(fDirection, pszTempDriverDesc, static_cast<SQLSMALLINT>(nDescriptionLength + 1), &nDescriptionLength, pszTempAttributes, static_cast<SQLSMALLINT>(nAttributesLength + 1), &nAttributesLength);
				if (SQL_SUCCEEDED(nRet))
					MultiSZToStringVector(pszTempAttributes, attributes);
				sTempAttributes.ReleaseBuffer();
				sDriverDesc.ReleaseBuffer();
			}

			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN Drivers(_In_ SQLUSMALLINT fDirection, _Out_ String& sDriverDesc, _Out_ std::vector<String>& attributes)
		{
			SQLSMALLINT nDriverDescLength = 0;
			SQLSMALLINT nAttributesLength = 0;
			SQLRETURN nRet = Drivers(fDirection, nullptr, 0, &nDriverDescLength, nullptr, 0, &nAttributesLength);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempDriverDesc;
				sTempDriverDesc.resize(nDriverDescLength + 1);
				std::vector<SQLTCHAR> sTempAttributes;
				sTempAttributes.resize(nAttributesLength + 1);
				nRet = Drivers(fDirection, &(sTempDriverDesc[0]), nDriverDescLength + 1, &nDriverDescLength, &(sTempAttributes[0]), nAttributesLength + 1, &nAttributesLength);
				if (SQL_SUCCEEDED(nRet))
				{
					sDriverDesc = reinterpret_cast<TCHAR*>(&(sTempDriverDesc[0]));
					MultiSZToStringVector(&(sTempAttributes[0]), attributes);
				}
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN EndTran(_In_ SQLSMALLINT CompletionType)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLEndTran(SQL_HANDLE_ENV, m_h, CompletionType);
		}

		SQLRETURN CommitTran()
		{
			return EndTran(SQL_COMMIT);
		}

		SQLRETURN RollbackTran()
		{
			return EndTran(SQL_ROLLBACK);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER Attribute, _Out_writes_(_Inexpressible_(BufferLength)) SQLPOINTER Value, _In_ SQLINTEGER BufferLength, _Out_opt_ SQLINTEGER* StringLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetEnvAttr(m_h, Attribute, Value, BufferLength, StringLength);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER Attribute, _Out_ SQLINTEGER& nValue)
		{
			return GetAttr(Attribute, &nValue, SQL_IS_INTEGER, nullptr);
		}

		SQLRETURN GetAttrU(_In_ SQLINTEGER Attribute, _Out_ SQLUINTEGER& nValue)
		{
			return GetAttr(Attribute, &nValue, SQL_IS_UINTEGER, nullptr);
		}

		SQLRETURN SetAttr(_In_ SQLINTEGER Attribute, _In_reads_bytes_opt_(StringLength) SQLPOINTER Value, _In_ SQLINTEGER StringLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSetEnvAttr(m_h, Attribute, Value, StringLength);
		}

		SQLRETURN SetAttr(_In_ SQLINTEGER Attribute, _In_ SQLINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(Attribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_INTEGER);
#pragma warning(pop)
		}

		SQLRETURN SetAttrU(_In_ SQLINTEGER Attribute, _In_ SQLUINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(Attribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_UINTEGER);
#pragma warning(pop)
		}
	};


	//Class which encapsulates an ODBC connection handle
	class CConnection : public CHandle
	{
	public:
		// Methods
		SQLRETURN Create(_In_ CEnvironment& environment)
		{
			//Delegate to the base class
			return CHandle::Create(SQL_HANDLE_DBC, environment);
		}

		void Attach(_In_ SQLHANDLE h)
		{
			CHandle::Attach(SQL_HANDLE_DBC, h);
		}

		SQLRETURN BrowseConnect(_In_reads_(cchConnStrIn) SQLTCHAR* szConnStrIn, _In_ SQLSMALLINT cchConnStrIn, _Out_writes_opt_(cchConnStrOutMax) SQLTCHAR* szConnStrOut, _In_ SQLSMALLINT cchConnStrOutMax, _Out_opt_ SQLSMALLINT* pcchConnStrOut)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLBrowseConnect(m_h, szConnStrIn, cchConnStrIn, szConnStrOut, cchConnStrOutMax, pcchConnStrOut);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN BrowseConnect(_In_opt_z_ TCHAR* szConnStrIn, _Out_ String& sConnStrOut)
		{
			SQLSMALLINT nStringLength = 0;
			SQLRETURN nRet = BrowseConnect(reinterpret_cast<SQLTCHAR*>(szConnStrIn), SQL_NTS, nullptr, 0, &nStringLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszConnStrOut = reinterpret_cast<SQLTCHAR*>(sConnStrOut.GetBufferSetLength(nStringLength + 1));
				nRet = BrowseConnect(reinterpret_cast<SQLTCHAR*>(szConnStrIn), SQL_NTS, pszConnStrOut, static_cast<SQLSMALLINT>(nStringLength + 1), &nStringLength);
				sConnStrOut.ReleaseBuffer();
			}

			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN BrowseConnect(_In_opt_z_ TCHAR* szConnStrIn, _Out_ String& sConnStrOut)
		{
			SQLSMALLINT nStringLength = 0;
			SQLRETURN nRet = BrowseConnect(reinterpret_cast<SQLTCHAR*>(szConnStrIn), SQL_NTS, nullptr, 0, &nStringLength);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempOutConnectionString;
				sTempOutConnectionString.resize(nStringLength + 1);
				nRet = BrowseConnect(reinterpret_cast<SQLTCHAR*>(szConnStrIn), SQL_NTS, &(sTempOutConnectionString[0]), nStringLength + 1, &nStringLength);
				if (SQL_SUCCEEDED(nRet))
					sConnStrOut = reinterpret_cast<TCHAR*>(&(sTempOutConnectionString[0]));
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN Connect(_In_reads_(cchDSN) SQLTCHAR* szDSN, _In_ SQLSMALLINT cchDSN, _In_reads_(cchUID) SQLTCHAR* szUID, _In_ SQLSMALLINT cchUID, _In_reads_(cchAuthStr) SQLTCHAR* szAuthStr, _In_ SQLSMALLINT cchAuthStr)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLConnect(m_h, szDSN, cchDSN, szUID, cchUID, szAuthStr, cchAuthStr);
		}

		SQLRETURN Connect(_In_opt_z_ TCHAR* ServerName, _In_opt_z_ TCHAR* UserName, _In_opt_z_ TCHAR* Authentication)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return Connect(reinterpret_cast<SQLTCHAR*>(ServerName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(UserName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(Authentication), SQL_NTS);
		}

		SQLRETURN DriverConnect(_In_opt_ SQLHWND hWnd, _In_reads_(cchConnStrIn) SQLTCHAR* szConnStrIn, _In_ SQLSMALLINT cchConnStrIn, _Out_writes_opt_(cchConnStrOutMax) SQLTCHAR* szConnStrOut, _In_ SQLSMALLINT cchConnStrOutMax,
			_Out_opt_ SQLSMALLINT* pcchConnStrOut, _In_ SQLUSMALLINT fDriverCompletion)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLDriverConnect(m_h, hWnd, szConnStrIn, cchConnStrIn, szConnStrOut, cchConnStrOutMax, pcchConnStrOut, fDriverCompletion);
		}

		__success(SQL_SUCCEEDED(return)) SQLRETURN DriverConnect(_In_z_ SQLTCHAR* szConnStrIn, _Inout_ String& sConnStrOut, _In_opt_ SQLHWND hWnd = nullptr, _In_ SQLUSMALLINT fDriverCompletion = SQL_DRIVER_NOPROMPT)
		{
			SQLTCHAR szTempConnStrOut[4096];
			szTempConnStrOut[0] = _T('\0');
			SQLSMALLINT nOutConnectionLength = 0;
			SQLRETURN nRet = DriverConnect(hWnd, szConnStrIn, static_cast<SQLSMALLINT>(_tcslen(reinterpret_cast<TCHAR*>(szConnStrIn))), szTempConnStrOut, sizeof(szTempConnStrOut) / sizeof(SQLTCHAR), &nOutConnectionLength, fDriverCompletion);
			if (SQL_SUCCEEDED(nRet))
				sConnStrOut = reinterpret_cast<TCHAR*>(szTempConnStrOut);
			return nRet;
		}

		SQLRETURN Disconnect()
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLDisconnect(m_h);
		}

		SQLRETURN EndTran(_In_ SQLSMALLINT CompletionType)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLEndTran(SQL_HANDLE_DBC, m_h, CompletionType);
		}

		SQLRETURN CommitTran()
		{
			return EndTran(SQL_COMMIT);
		}

		SQLRETURN RollbackTran()
		{
			return EndTran(SQL_ROLLBACK);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER fAttribute, _Out_writes_opt_(_Inexpressible_(cbValueMax)) SQLPOINTER rgbValue, _In_ SQLINTEGER cbValueMax, _Out_opt_ SQLINTEGER* pcbValue)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetConnectAttr(m_h, fAttribute, rgbValue, cbValueMax, pcbValue);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER fAttribute, _Out_ SQLINTEGER& nValue)
		{
			return GetAttr(fAttribute, &nValue, SQL_IS_INTEGER, nullptr);
		}

		SQLRETURN GetAttrU(_In_ SQLINTEGER fAttribute, _Out_ SQLUINTEGER& nValue)
		{
			return GetAttr(fAttribute, &nValue, SQL_IS_UINTEGER, nullptr);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN GetAttr(_In_ SQLINTEGER fAttribute, _Out_ String& sValue)
		{
			SQLINTEGER nLength = 0;
			SQLRETURN nRet = GetAttr(fAttribute, nullptr, 0, &nLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLINTEGER nCharacters = nLength / sizeof(SQLTCHAR);
				SQLTCHAR* pszTempValue = reinterpret_cast<SQLTCHAR*>(sValue.GetBufferSetLength(nCharacters + 1));
				nRet = GetAttr(fAttribute, pszTempValue, nLength + sizeof(SQLTCHAR), &nLength);
				sValue.ReleaseBuffer();
			}
			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN GetAttr(_In_ SQLINTEGER fAttribute, _Out_ String& sValue)
		{
			SQLINTEGER nLength = 0;
			SQLRETURN nRet = GetAttr(fAttribute, nullptr, 0, &nLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLINTEGER nCharacters = nLength / sizeof(SQLTCHAR);
				std::vector<SQLTCHAR> sTempValue;
				sTempValue.resize(nCharacters + 1);
				nRet = GetAttr(fAttribute, &(sTempValue[0]), nLength + sizeof(SQLTCHAR), &nLength);
				if (SQL_SUCCEEDED(nRet))
					sValue = reinterpret_cast<TCHAR*>(&(sTempValue[0]));
			}
			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		_Success_(return == SQL_SUCCESS) SQLRETURN GetInfo(_In_ SQLUSMALLINT fInfoType, _Out_writes_bytes_opt_(cbInfoValueMax) SQLPOINTER rgbInfoValue, _In_ SQLSMALLINT cbInfoValueMax, _Out_opt_ SQLSMALLINT* pcbInfoValue)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetInfo(m_h, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);
		}

		SQLRETURN NativeSql(_In_reads_(cchSqlStrIn) SQLTCHAR* szSqlStrIn, _In_ SQLINTEGER cchSqlStrIn, _Out_writes_opt_(cchSqlStrMax) SQLTCHAR* szSqlStr, _In_ SQLINTEGER cchSqlStrMax, _Out_ SQLINTEGER* pcchSqlStr)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLNativeSql(m_h, szSqlStrIn, cchSqlStrIn, szSqlStr, cchSqlStrMax, pcchSqlStr);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN NativeSql(_In_z_ TCHAR* szSqlStrIn, _Out_ String& sSqlStr)
		{
			SQLINTEGER nSqlStrLength = 0;
			SQLRETURN nRet = NativeSql(reinterpret_cast<SQLTCHAR*>(szSqlStrIn), SQL_NTS, nullptr, 0, &nSqlStrLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszTempSqlStr = reinterpret_cast<SQLTCHAR*>(sSqlStr.GetBufferSetLength(nSqlStrLength + 1));
				nRet = NativeSql(reinterpret_cast<SQLTCHAR*>(szSqlStrIn), SQL_NTS, pszTempSqlStr, nSqlStrLength + 1, &nSqlStrLength);
				sSqlStr.ReleaseBuffer();
			}

			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN NativeSql(_In_z_ TCHAR* szSqlStrIn, _Out_ String& sSqlStr)
		{
			SQLINTEGER nSqlStrLength = 0;
			SQLRETURN nRet = NativeSql(reinterpret_cast<SQLTCHAR*>(szSqlStrIn), SQL_NTS, nullptr, 0, &nSqlStrLength);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempSqlStr;
				sTempSqlStr.resize(nSqlStrLength + 1);
				nRet = NativeSql(reinterpret_cast<SQLTCHAR*>(szSqlStrIn), SQL_NTS, &(sTempSqlStr[0]), nSqlStrLength + 1, &nSqlStrLength);
				if (SQL_SUCCEEDED(nRet))
					sSqlStr = reinterpret_cast<TCHAR*>(&(sTempSqlStr[0]));
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN SetAttr(_In_ SQLINTEGER fAttribute, _In_reads_bytes_opt_(cbValue) SQLPOINTER rgbValue, _In_ SQLINTEGER cbValue)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSetConnectAttr(m_h, fAttribute, rgbValue, cbValue);
		}

		SQLRETURN SetAttr(_In_ SQLINTEGER fAttribute, _In_ SQLINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(fAttribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_INTEGER);
#pragma warning(pop)
		}

		SQLRETURN SetAttrU(_In_ SQLINTEGER fAttribute, _In_ SQLUINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(fAttribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_UINTEGER);
#pragma warning(pop)
		}

		SQLRETURN SetAttr(_In_ SQLINTEGER fAttribute, _In_ TCHAR* pszValue)
		{
			return SetAttr(fAttribute, reinterpret_cast<SQLTCHAR*>(pszValue), static_cast<SQLINTEGER>(_tcslen(reinterpret_cast<TCHAR*>(pszValue)) * sizeof(SQLTCHAR)));
		}

		SQLRETURN GetFunctions(_In_ SQLUSMALLINT fFunctionId, _Out_writes_opt_(_Inexpressible_("Buffer length pfExists points to depends on fFunction value.")) SQLUSMALLINT* Supported)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetFunctions(m_h, fFunctionId, Supported);
		}
	};


	//Class which encapsulates an ODBC statement handle
	class CStatement : public CHandle
	{
	public:
		// Methods
		SQLRETURN Create(_In_ CConnection& connection)
		{
			//Delegate to the base class
			return CHandle::Create(SQL_HANDLE_STMT, connection);
		}

		void Attach(_In_ SQLHANDLE h)
		{
			CHandle::Attach(SQL_HANDLE_STMT, h);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _In_ SQLSMALLINT TargetType, _Inout_updates_opt_(_Inexpressible_(BufferLength)) SQLPOINTER TargetValue, _In_ SQLLEN BufferLength, _Inout_opt_ SQLLEN* StrLen_or_Ind)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLBindCol(m_h, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ char& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ BYTE& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ long& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ bool& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ float& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ double& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ LONGLONG& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_updates_opt_(_Inexpressible_(BufferLength)) BYTE* value, _In_ SQLINTEGER BufferLength, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), value, BufferLength, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_updates_opt_(_Inexpressible_(BufferLength)) char* value, _In_ SQLINTEGER BufferLength, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), value, BufferLength, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_updates_opt_(_Inexpressible_(BufferLength)) wchar_t* value, _In_ SQLINTEGER BufferLength, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), value, BufferLength, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ TIMESTAMP_STRUCT& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindCol(_In_ SQLUSMALLINT ColumnNumber, _Inout_ SQL_NUMERIC_STRUCT& value, _Inout_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			return BindCol(ColumnNumber, _GetODBCCType(value), &value, 0, StrLen_or_IndPtr);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
			SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLINTEGER cbValueMax, SQLLEN* pcbValue)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLBindParameter(m_h, iPar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue, cbValueMax, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, char& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, BYTE& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, long& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, bool& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, float& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, double& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, LONGLONG& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, BYTE* value, SQLINTEGER BufferLength, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), BufferLength, 0, value, BufferLength, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, char* value, SQLINTEGER BufferLength, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), BufferLength, 0, value, BufferLength, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, wchar_t* value, SQLINTEGER BufferLength, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), BufferLength, 0, value, BufferLength, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, TIMESTAMP_STRUCT& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BindParameter(SQLUSMALLINT iPar, SQLSMALLINT fParamType, SQL_NUMERIC_STRUCT& value, SQLLEN* pcbValue = nullptr)
		{
			return BindParameter(iPar, fParamType, _GetODBCCType(value), _GetODBCSQLType(value), 0, 0, &value, 0, pcbValue);
		}

		SQLRETURN BulkOperations(_In_ SQLUSMALLINT Operation)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLBulkOperations(m_h, Operation);
		}

		SQLRETURN Cancel()
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLCancel(m_h);
		}

		SQLRETURN CloseCursor()
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLCloseCursor(m_h);
		}

		SQLRETURN ColAttribute(_In_ SQLUSMALLINT iCol, _In_ SQLUSMALLINT iField, _Out_writes_bytes_opt_(cbDescMax) SQLPOINTER pCharAttr, SQLSMALLINT cbDescMax,
			_Out_opt_ SQLSMALLINT* pcbCharAttr, _Out_opt_ SQLLEN* pNumAttr)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLColAttribute(m_h, iCol, iField, pCharAttr, cbDescMax, pcbCharAttr, pNumAttr);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN ColAttribute(_In_ SQLUSMALLINT iCol, _In_ SQLUSMALLINT iField, _Out_ String& sValue, _Out_opt_ SQLLEN* pNumAttr = nullptr)
		{
			SQLSMALLINT nLength = 0;
			SQLRETURN nRet = ColAttribute(iCol, iField, nullptr, 0, &nLength, pNumAttr);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLINTEGER nCharacters = nLength / sizeof(SQLTCHAR);
				SQLTCHAR* pszTempValue = reinterpret_cast<SQLTCHAR*>(sValue.GetBufferSetLength(nCharacters + 1));
				nRet = ColAttribute(iCol, iField, pszTempValue, static_cast<SQLSMALLINT>(nLength + sizeof(SQLTCHAR)), &nLength, pNumAttr);
				sValue.ReleaseBuffer();
			}
			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN ColAttribute(_In_ SQLUSMALLINT iCol, _In_ SQLUSMALLINT iField, _Out_ String& sValue, _Out_opt_ SQLLEN* pNumAttr = nullptr)
		{
			SQLSMALLINT nLength = 0;
			SQLRETURN nRet = ColAttribute(iCol, iField, nullptr, 0, &nLength, pNumAttr);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLINTEGER nCharacters = nLength / sizeof(SQLTCHAR);
				std::vector<SQLTCHAR> sTempValue;
				sTempValue.resize(nCharacters + 1);
				nRet = ColAttribute(iCol, iField, &(sTempValue[0]), nLength + sizeof(SQLTCHAR), &nLength, pNumAttr);
				if (SQL_SUCCEEDED(nRet))
					sValue = reinterpret_cast<TCHAR*>(&(sTempValue[0]));
			}
			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN ColumnPrivileges(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchTableName) SQLTCHAR* szTableName,
			_In_ SQLSMALLINT cchTableName, _In_reads_opt_(cchColumnName) SQLTCHAR* szColumnName, _In_ SQLSMALLINT cchColumnName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLColumnPrivileges(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szTableName, cchTableName, szColumnName, cchColumnName);
		}

		SQLRETURN ColumnPrivileges(_In_opt_z_ TCHAR* szCatalogName, _In_opt_z_ TCHAR* szSchemaName, _In_opt_z_ TCHAR* szTableName, _In_opt_z_ TCHAR* szColumnName)
		{
			return ColumnPrivileges(reinterpret_cast<SQLTCHAR*>(szCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szTableName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szColumnName), SQL_NTS);
		}

		SQLRETURN Columns(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchTableName) SQLTCHAR* szTableName,
			_In_ SQLSMALLINT cchTableName, _In_reads_opt_(cchColumnName) SQLTCHAR* szColumnName, _In_ SQLSMALLINT cchColumnName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLColumns(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szTableName, cchTableName, szColumnName, cchColumnName);
		}

		SQLRETURN Columns(_In_opt_z_ TCHAR* CatalogName, _In_opt_z_ TCHAR* SchemaName, _In_opt_z_ TCHAR* TableName, _In_opt_z_ TCHAR* ColumnName)
		{
			return Columns(reinterpret_cast<SQLTCHAR*>(CatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(SchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(TableName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(ColumnName), SQL_NTS);
		}

		SQLRETURN DescribeCol(_In_ SQLSMALLINT iCol, _Out_writes_opt_(cchColNameMax) SQLTCHAR* szColName, _In_ SQLSMALLINT cchColNameMax, _Out_opt_ SQLSMALLINT* pcchColName,
			_Out_opt_ SQLSMALLINT* pfSqlType, _Out_opt_ SQLULEN* pcbColDef, _Out_opt_ SQLSMALLINT* pibScale, _Out_opt_ SQLSMALLINT* pfNullable)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLDescribeCol(m_h, iCol, szColName, cchColNameMax, pcchColName, pfSqlType, pcbColDef, pibScale, pfNullable);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN DescribeCol(_In_ SQLSMALLINT iCol, _Out_ String& sColName, _Out_opt_ SQLSMALLINT* pfSqlType, _Out_opt_ SQLULEN* pcbColDef, _Out_opt_ SQLSMALLINT* pibScale, _Out_opt_ SQLSMALLINT* pfNullable)
		{
			SQLSMALLINT nNameLength = 0;
			SQLRETURN nRet = DescribeCol(iCol, nullptr, 0, &nNameLength, pfSqlType, pcbColDef, pibScale, pfNullable);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszTempColName = reinterpret_cast<SQLTCHAR*>(sColName.GetBufferSetLength(nNameLength + 1));
				nRet = DescribeCol(iCol, pszTempColName, static_cast<SQLSMALLINT>(nNameLength + 1), &nNameLength, pfSqlType, pcbColDef, pibScale, pfNullable);
				sColName.ReleaseBuffer();
			}

			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN DescribeCol(_In_ SQLSMALLINT ColumnNumber, _Out_ String& sColumnName, _Out_opt_ SQLSMALLINT* DataTypePtr, _Out_opt_ SQLULEN* ColumnSizePtr, _Out_opt_ SQLSMALLINT* DecimalDigitsPtr, _Out_opt_ SQLSMALLINT* NullablePtr)
		{
			SQLSMALLINT nNameLength = 0;
			SQLRETURN nRet = DescribeCol(ColumnNumber, nullptr, 0, &nNameLength, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempColumnName;
				sTempColumnName.resize(nNameLength + 1);
				nRet = DescribeCol(ColumnNumber, &(sTempColumnName[0]), nNameLength + 1, &nNameLength, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
				if (SQL_SUCCEEDED(nRet))
					sColumnName = reinterpret_cast<TCHAR*>(&(sTempColumnName[0]));
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN DescribeParam(_In_ SQLUSMALLINT iPar, _Out_opt_ SQLSMALLINT* pfSqlType, _Out_opt_ SQLULEN* pcbParamDef, _Out_opt_ SQLSMALLINT* pibScale, _Out_opt_ SQLSMALLINT* pfNullable)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLDescribeParam(m_h, iPar, pfSqlType, pcbParamDef, pibScale, pfNullable);
		}

		SQLRETURN ExecDirect(_In_reads_opt_(TextLength) SQLTCHAR* StatementText, SQLINTEGER TextLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLExecDirect(m_h, StatementText, TextLength);
		}

		SQLRETURN ExecDirect(_In_opt_z_ TCHAR* StatementText)
		{
			return ExecDirect(reinterpret_cast<SQLTCHAR*>(StatementText), SQL_NTS);
		}

		SQLRETURN Execute()
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLExecute(m_h);
		}

		SQLRETURN Fetch()
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLFetch(m_h);
		}

		SQLRETURN FetchScroll(_In_ SQLSMALLINT FetchOrientation, _In_ SQLLEN FetchOffset)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLFetchScroll(m_h, FetchOrientation, FetchOffset);
		}

		SQLRETURN FetchNext()
		{
			return FetchScroll(SQL_FETCH_NEXT, 0);
		}

		SQLRETURN FetchPrior()
		{
			return FetchScroll(SQL_FETCH_PRIOR, 0);
		}

		SQLRETURN FetchFirst()
		{
			return FetchScroll(SQL_FETCH_FIRST, 0);
		}

		SQLRETURN FetchLast()
		{
			return FetchScroll(SQL_FETCH_LAST, 0);
		}

		SQLRETURN FetchBookmark(_In_ SQLLEN FetchOffset)
		{
			return FetchScroll(SQL_FETCH_BOOKMARK, FetchOffset);
		}

		SQLRETURN FetchAbsolute(_In_ SQLLEN FetchOffset)
		{
			return FetchScroll(SQL_FETCH_ABSOLUTE, FetchOffset);
		}

		SQLRETURN FetchRelative(_In_ SQLLEN FetchOffset)
		{
			return FetchScroll(SQL_FETCH_RELATIVE, FetchOffset);
		}

		SQLRETURN ForeignKeys(_In_reads_opt_(cchPkCatalogName) SQLTCHAR* szPkCatalogName, _In_ SQLSMALLINT cchPkCatalogName, _In_reads_opt_(cchPkSchemaName) SQLTCHAR* szPkSchemaName, _In_ SQLSMALLINT cchPkSchemaName, _In_reads_opt_(cchPkTableName) SQLTCHAR* szPkTableName, _In_ SQLSMALLINT cchPkTableName,
			_In_reads_opt_(cchFkCatalogName) SQLTCHAR* szFkCatalogName, _In_ SQLSMALLINT cchFkCatalogName, _In_reads_opt_(cchFkSchemaName) SQLTCHAR* szFkSchemaName, _In_ SQLSMALLINT cchFkSchemaName, _In_reads_opt_(cchFkTableName) SQLTCHAR* szFkTableName, _In_ SQLSMALLINT cchFkTableName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLForeignKeys(m_h, szPkCatalogName, cchPkCatalogName, szPkSchemaName, cchPkSchemaName, szPkTableName, cchPkTableName, szFkCatalogName, cchFkCatalogName, szFkSchemaName, cchFkSchemaName, szFkTableName, cchFkTableName);
		}

		SQLRETURN ForeignKeys(_In_opt_z_ TCHAR* szPkCatalogName, _In_opt_z_ TCHAR* szPkSchemaName, _In_opt_z_ TCHAR* szPkTableName, _In_opt_z_ TCHAR* szFkCatalogName, _In_opt_z_ TCHAR* szFkSchemaName, _In_opt_z_ TCHAR* szFkTableName)
		{
			return ForeignKeys(reinterpret_cast<SQLTCHAR*>(szPkCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szPkSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szPkTableName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szFkCatalogName),
				SQL_NTS, reinterpret_cast<SQLTCHAR*>(szFkSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szFkTableName), SQL_NTS);
		}

		SQLRETURN Free(_In_ SQLUSMALLINT Option)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLFreeStmt(m_h, Option);
		}

		SQLRETURN GetCursorName(_Out_writes_opt_(BufferLength) SQLTCHAR* CursorName, SQLSMALLINT BufferLength, _Out_opt_ SQLSMALLINT* NameLengthPtr)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetCursorName(m_h, CursorName, BufferLength, NameLengthPtr);
		}

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		__success(SQL_SUCCEEDED(return)) SQLRETURN GetCursorName(_Out_ String& sCursorName)
		{
			SQLSMALLINT nLength = 0;
			SQLRETURN nRet = GetCursorName(nullptr, 0, &nLength);
			if (SQL_SUCCEEDED(nRet))
			{
				SQLTCHAR* pszTempCursorName = reinterpret_cast<SQLTCHAR*>(sCursorName.GetBufferSetLength(nLength + 1));
				nRet = GetCursorName(pszTempCursorName, static_cast<SQLSMALLINT>(nLength + 1), &nLength);
				sCursorName.ReleaseBuffer();
			}
			return nRet;
		}
#else
		__success(SQL_SUCCEEDED(return)) SQLRETURN GetCursorName(_Out_ String& sCursorName)
		{
			SQLSMALLINT nLength = 0;
			SQLRETURN nRet = GetCursorName(nullptr, 0, &nLength);
			if (SQL_SUCCEEDED(nRet))
			{
				std::vector<SQLTCHAR> sTempCursorName;
				sTempCursorName.resize(nLength + 1);
				nRet = GetCursorName(&(sTempCursorName[0]), nLength + 1, &nLength);
				if (SQL_SUCCEEDED(nRet))
					sCursorName = reinterpret_cast<TCHAR*>(&(sTempCursorName[0]));
			}
			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		SQLRETURN GetData(_In_ SQLUSMALLINT ColumnNumber, _In_ SQLSMALLINT TargetType, _Out_writes_opt_(_Inexpressible_(BufferLength)) SQLPOINTER TargetValue, _In_ SQLLEN BufferLength, _Out_opt_ SQLLEN* StrLen_or_IndPtr)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetData(m_h, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER Attribute, _Out_writes_opt_(_Inexpressible_(BufferLength)) SQLPOINTER Value, _In_ SQLINTEGER BufferLength, _Out_opt_ SQLINTEGER* StringLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetStmtAttr(m_h, Attribute, Value, BufferLength, StringLength);
		}

		SQLRETURN GetAttr(_In_ SQLINTEGER Attribute, _Out_ SQLINTEGER& nValue)
		{
			return GetAttr(Attribute, &nValue, SQL_IS_INTEGER, nullptr);
		}

		SQLRETURN GetAttrU(_In_ SQLINTEGER Attribute, _Out_ SQLUINTEGER& nValue)
		{
			return GetAttr(Attribute, &nValue, SQL_IS_UINTEGER, nullptr);
		}

		SQLRETURN GetTypeInfo(_In_ SQLSMALLINT DataType)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLGetTypeInfo(m_h, DataType);
		}

		SQLRETURN MoreResults()
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLMoreResults(m_h);
		}

		SQLRETURN NumParams(_Out_opt_ SQLSMALLINT* pcpar)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLNumParams(m_h, pcpar);
		}

		SQLRETURN NumResultCols(_Out_ SQLSMALLINT* ColumnCount)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLNumResultCols(m_h, ColumnCount);
		}

		SQLRETURN ParamData(_Out_opt_ SQLPOINTER* Value)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLParamData(m_h, Value);
		}

		SQLRETURN Prepare(_In_reads_(TextLength) SQLTCHAR* StatementText, SQLINTEGER TextLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLPrepare(m_h, StatementText, TextLength);
		}

		SQLRETURN Prepare(_In_z_ SQLTCHAR* StatementText)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLPrepare(m_h, StatementText, SQL_NTS);
		}

		SQLRETURN PrimaryKeys(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchTableName) SQLTCHAR* szTableName, _In_ SQLSMALLINT cchTableName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLPrimaryKeys(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szTableName, cchTableName);
		}

		SQLRETURN PrimaryKeys(_In_opt_z_ TCHAR* szCatalogName, _In_opt_z_ TCHAR* szSchemaName, _In_opt_z_ TCHAR* szTableName)
		{
			return PrimaryKeys(reinterpret_cast<SQLTCHAR*>(szCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szTableName), SQL_NTS);
		}

		SQLRETURN ProcedureColumns(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchProcName) SQLTCHAR* szProcName, _In_ SQLSMALLINT cchProcName, _In_reads_opt_(cchColumnName) SQLTCHAR* szColumnName, _In_ SQLSMALLINT cchColumnName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLProcedureColumns(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szProcName, cchProcName, szColumnName, cchColumnName);
		}

		SQLRETURN ProcedureColumns(_In_opt_z_ TCHAR* szCatalogName, _In_opt_z_ TCHAR* szSchemaName, _In_opt_z_ TCHAR* szProcName, _In_opt_z_ TCHAR* szColumnName)
		{
			return ProcedureColumns(reinterpret_cast<SQLTCHAR*>(szCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szProcName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szColumnName), SQL_NTS);
		}

		SQLRETURN Procedures(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchProcName) SQLTCHAR* szProcName, _In_ SQLSMALLINT cchProcName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLProcedures(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szProcName, cchProcName);
		}

		SQLRETURN Procedures(_In_opt_z_ TCHAR* szCatalogName, _In_opt_z_ TCHAR* szSchemaName, _In_opt_z_ TCHAR* szProcName)
		{
			return Procedures(reinterpret_cast<SQLTCHAR*>(szCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szProcName), SQL_NTS);
		}

		SQLRETURN PutData(_In_reads_(_Inexpressible_(StrLen_or_Ind)) SQLPOINTER Data, _In_ SQLLEN StrLen_or_Ind)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLPutData(m_h, Data, StrLen_or_Ind);
		}

		SQLRETURN RowCount(_Out_ SQLLEN* RowCount)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLRowCount(m_h, RowCount);
		}

		SQLRETURN SetCursorName(_In_reads_(NameLength) SQLTCHAR* CursorName, _In_ SQLSMALLINT NameLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSetCursorName(m_h, CursorName, NameLength);
		}

		SQLRETURN SetCursorName(_In_opt_z_ SQLTCHAR* CursorName)
		{
			return SetCursorName(CursorName, SQL_NTS);
		}

		SQLRETURN SetPos(_In_ SQLSETPOSIROW iRow, _In_ SQLUSMALLINT fOption, _In_ SQLUSMALLINT fLock)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSetPos(m_h, iRow, fOption, fLock);
		}

		SQLRETURN SetAttr(_In_ SQLINTEGER Attribute, _In_reads_(_Inexpressible_(StringLength)) SQLPOINTER Value, _In_ SQLINTEGER StringLength)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSetStmtAttr(m_h, Attribute, Value, StringLength);
		}

		SQLRETURN SetAttr(SQLINTEGER Attribute, SQLINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(Attribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_INTEGER);
#pragma warning(pop)
		}

		SQLRETURN SetAttrU(SQLINTEGER Attribute, SQLUINTEGER nValue)
		{
#pragma warning(push)
#pragma warning(disable : 4312)
			return SetAttr(Attribute, reinterpret_cast<SQLPOINTER>(nValue), SQL_IS_UINTEGER);
#pragma warning(pop)
		}

		SQLRETURN SpecialColumns(_In_ SQLSMALLINT IdentifierType, _In_reads_opt_(NameLength1) SQLTCHAR* CatalogName, _In_ SQLSMALLINT NameLength1, _In_reads_opt_(NameLength2) SQLTCHAR* SchemaName, _In_ SQLSMALLINT NameLength2,
			_In_reads_opt_(NameLength3) SQLTCHAR* TableName, _In_ SQLSMALLINT NameLength3, _In_ SQLSMALLINT Scope, _In_ SQLSMALLINT Nullable)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLSpecialColumns(m_h, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable);
		}

		SQLRETURN SpecialColumns(_In_ SQLSMALLINT IdentifierType, _In_opt_z_ TCHAR* CatalogName, _In_opt_z_ TCHAR* SchemaName, _In_opt_z_ TCHAR* TableName, _In_ SQLSMALLINT Scope, _In_ SQLSMALLINT Nullable)
		{
			return SpecialColumns(IdentifierType, reinterpret_cast<SQLTCHAR*>(CatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(SchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(TableName), SQL_NTS, Scope, Nullable);
		}

		SQLRETURN Statistics(_In_reads_opt_(NameLength1) SQLTCHAR* CatalogName, _In_ SQLSMALLINT NameLength1, _In_reads_opt_(NameLength2) SQLTCHAR* SchemaName, _In_ SQLSMALLINT NameLength2, _In_reads_opt_(NameLength3) SQLTCHAR* TableName,
			_In_ SQLSMALLINT NameLength3, _In_ SQLUSMALLINT Unique, _In_ SQLUSMALLINT Reserved)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLStatistics(m_h, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Unique, Reserved);
		}

		SQLRETURN Statistics(_In_opt_z_ TCHAR* CatalogName, _In_opt_z_ TCHAR* SchemaName, _In_opt_z_ TCHAR* TableName, _In_ SQLUSMALLINT Unique, _In_ SQLUSMALLINT Reserved)
		{
			return Statistics(reinterpret_cast<SQLTCHAR*>(CatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(SchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(TableName), SQL_NTS, Unique, Reserved);
		}

		SQLRETURN TablePrivileges(_In_reads_opt_(cchCatalogName) SQLTCHAR* szCatalogName, _In_ SQLSMALLINT cchCatalogName, _In_reads_opt_(cchSchemaName) SQLTCHAR* szSchemaName, _In_ SQLSMALLINT cchSchemaName, _In_reads_opt_(cchTableName) SQLTCHAR* szTableName, _In_ SQLSMALLINT cchTableName)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLTablePrivileges(m_h, szCatalogName, cchCatalogName, szSchemaName, cchSchemaName, szTableName, cchTableName);
		}

		SQLRETURN TablePrivileges(_In_opt_z_ TCHAR* szCatalogName, _In_opt_z_ TCHAR* szSchemaName, _In_opt_z_ TCHAR* szTableName)
		{
			return TablePrivileges(reinterpret_cast<SQLTCHAR*>(szCatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szSchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(szTableName), SQL_NTS);
		}

		SQLRETURN Tables(_In_reads_opt_(NameLength1) SQLTCHAR* CatalogName, _In_ SQLSMALLINT NameLength1, _In_reads_opt_(NameLength2) SQLTCHAR* SchemaName, _In_ SQLSMALLINT NameLength2, _In_reads_opt_(NameLength3) SQLTCHAR* TableName, _In_ SQLSMALLINT NameLength3,
			_In_reads_opt_(NameLength4) SQLTCHAR* TableType, _In_ SQLSMALLINT NameLength4)
		{
			//Validate our parameters
			ATLASSERT(m_h != SQL_NULL_HANDLE);

			return SQLTables(m_h, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
		}

		SQLRETURN Tables(_In_opt_z_ TCHAR* CatalogName, _In_opt_z_ TCHAR* SchemaName, _In_opt_z_ TCHAR* TableName, _In_opt_z_ TCHAR* TableType)
		{
			return Tables(reinterpret_cast<SQLTCHAR*>(CatalogName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(SchemaName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(TableName), SQL_NTS, reinterpret_cast<SQLTCHAR*>(TableType), SQL_NTS);
		}
	};


	//Class which encapsulates an ODBC descriptor handle
	class CDescriptor : public CHandle
	{
	public:
		// Methods
		SQLRETURN Create(_In_ CConnection& connection)
		{
			//Delegate to the base class
			return CHandle::Create(SQL_HANDLE_DESC, connection);
		}

		void Attach(_In_ SQLHANDLE h)
		{
			CHandle::Attach(SQL_HANDLE_DESC, h);
		}

		SQLRETURN Copy(_In_ SQLHDESC TargetDescHandle)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLCopyDesc(m_h, TargetDescHandle);
		}

		SQLRETURN GetField(_In_ SQLSMALLINT RecNumber, _In_ SQLSMALLINT FieldIdentifier, _Out_writes_opt_(_Inexpressible_(BufferLength)) SQLPOINTER ValuePtr, _In_ SQLINTEGER BufferLength, _Out_opt_ SQLINTEGER* StringLengthPtr)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLGetDescField(m_h, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr);
		}

		SQLRETURN GetRec(_In_ SQLSMALLINT RecNumber, _Out_writes_opt_(BufferLength) SQLTCHAR* Name, _In_ SQLSMALLINT BufferLength, _Out_opt_ SQLSMALLINT* StringLengthPtr, _Out_opt_ SQLSMALLINT* TypePtr, _Out_opt_ SQLSMALLINT* SubTypePtr,
			_Out_opt_ SQLLEN* LengthPtr, _Out_opt_ SQLSMALLINT* PrecisionPtr, _Out_opt_ SQLSMALLINT* ScalePtr, _Out_opt_ SQLSMALLINT* NullablePtr)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLGetDescRec(m_h, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
		}

		SQLRETURN SetField(_In_ SQLSMALLINT RecNumber, _In_ SQLSMALLINT FieldIdentifier, _In_reads_(_Inexpressible_(BufferLength)) SQLPOINTER Value, _In_ SQLINTEGER BufferLength)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLSetDescField(m_h, RecNumber, FieldIdentifier, Value, BufferLength);
		}

		SQLRETURN SetRec(_In_ SQLSMALLINT RecNumber, _In_ SQLSMALLINT Type, _In_ SQLSMALLINT SubType, _In_ SQLLEN Length, _In_ SQLSMALLINT Precision, _In_ SQLSMALLINT Scale, _Inout_updates_bytes_opt_(Length) SQLPOINTER Data,
			_Inout_opt_ SQLLEN* StringLength, _Inout_opt_ SQLLEN* Indicator)
		{
			//Validate our parameters
			ATLASSUME(m_h != SQL_NULL_HANDLE);

			return SQLSetDescRec(m_h, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLength, Indicator);
		}
	};


	//The base class which accessor classes derive from. T is the class that contains the data that will be accessed
	template<class T>
	class CAccessor : public T
	{
	public:
		// Methods
#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		SQLRETURN BindColumns(_In_ CStatement& statement)
		{
			UNREFERENCED_PARAMETER(statement); //To eliminate potential unreferenced variables warnings

			//What will be the return value from this method
			SQLRETURN nRet = SQL_SUCCESS;

			__if_exists(T::_GetBindColumnEntries)
			{
				//Work out the number of bound columns we have
				int nColumns = 0;
				nRet = T::_GetBindColumnEntries(nullptr, &nColumns, nullptr);
				ODBC_CHECK_RETURN(nRet, statement);

				//Allocate the parameter indicators array
				m_ColumnIndicators.RemoveAll();
				m_ColumnIndicators.SetSize(nColumns);

				//Bind the columns
				nRet = T::_GetBindColumnEntries(&statement, nullptr, &m_ColumnIndicators);
				ODBC_CHECK_RETURN(nRet, statement);
			}
			return nRet;
		}
#else
		SQLRETURN BindColumns(_In_ CStatement& statement)
		{
			UNREFERENCED_PARAMETER(statement);

			//What will be the return value from this method
			SQLRETURN nRet = SQL_SUCCESS;

			__if_exists(T::_GetBindColumnEntries)
			{
				//Work out the number of bound columns we have
				int nColumns = 0;
				nRet = T::_GetBindColumnEntries(nullptr, &nColumns, nullptr);
				ODBC_CHECK_RETURN(nRet, statement);

				//Allocate the parameter indicators array
				m_ColumnIndicators.clear();
				m_ColumnIndicators.insert(m_ColumnIndicators.end(), nColumns, 0);

				//Bind the columns
				nRet = T::_GetBindColumnEntries(&statement, nullptr, &m_ColumnIndicators);
				ODBC_CHECK_RETURN(nRet, statement);
			}
			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		SQLRETURN BindParameters(_In_ CStatement& statement)
		{
			UNREFERENCED_PARAMETER(statement); //To eliminate potential unreferenced variables warnings

			//What will be the return value from this method
			SQLRETURN nRet = SQL_SUCCESS;

			__if_exists(T::_GetBindParametersEntries)
			{
				//Work out the number of bound parameters we have
				int nColumns = 0;
				nRet = T::_GetBindParametersEntries(nullptr, &nColumns, nullptr);
				ODBC_CHECK_RETURN(nRet, statement);

				//Allocate the temporary array of indicators based on the parameter count
				m_ParameterIndicators.SetSize(nColumns, 0);

				//Bind the parameters
				nRet = T::_GetBindParametersEntries(&statement, nullptr, &m_ParameterIndicators);
				ODBC_CHECK_RETURN(nRet, statement);
			}

			return nRet;
		}
#else
		SQLRETURN BindParameters(_In_ CStatement& statement)
		{
			UNREFERENCED_PARAMETER(statement);

			//What will be the return value from this method
			SQLRETURN nRet = SQL_SUCCESS;

			__if_exists(T::_GetBindParametersEntries)
			{
				//Work out the number of bound parameters we have
				int nColumns = 0;
				nRet = T::_GetBindParametersEntries(nullptr, &nColumns, nullptr);
				ODBC_CHECK_RETURN(nRet, statement);

				//Allocate the temporary array of indicators based on the parameter count
				m_ParameterIndicators.clear();
				m_ParameterIndicators.insert(m_ParameterIndicators.end(), nColumns, 0);

				//Bind the parameters
				nRet = T::_GetBindParametersEntries(&statement, nullptr, &m_ParameterIndicators);
				ODBC_CHECK_RETURN(nRet, statement);
			}

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		//Member variables
#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		CArray<SQLLEN, SQLLEN> m_ColumnIndicators;
		CArray<SQLLEN, SQLLEN> m_ParameterIndicators;
#else
		std::vector<SQLLEN> m_ColumnIndicators;
		std::vector<SQLLEN> m_ParameterIndicators;
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
	};


	struct SQL_ATTRIBUTE
	{
		SQLINTEGER m_Attribute;
		SQLPOINTER m_Value;
		SQLINTEGER m_StringLength;
	};


	//Class which CCommand class defined later on uses as a base class
	class CCommandBase
	{
	public:
		CCommandBase()
		{
		}

		~CCommandBase()
		{
			ReleaseCommand();
		}

		//Create the command
		SQLRETURN CreateCommand(_In_ CConnection& connection) throw()
		{
			//Before creating the command, release the old one if necessary.
			ReleaseCommand();

			return m_Command.Create(connection);
		}

		SQLRETURN SetAttributes(_In_opt_ SQL_ATTRIBUTE* pAttributes, ULONG nAttributes) throw()
		{
			for (ULONG i = 0; i < nAttributes; i++)
			{
				//Validate our parameters
				ATLASSUME(pAttributes != nullptr);

				SQLRETURN nRet = m_Command.SetAttr(pAttributes[i].m_Attribute, pAttributes[i].m_Value, pAttributes[i].m_StringLength);
				if (!SQL_SUCCEEDED(nRet))
					return nRet;
			}

			return SQL_SUCCESS;
		}

		//Prepare the command
		SQLRETURN Prepare(_In_z_ SQLTCHAR* szCommand) throw()
		{
			return m_Command.Prepare(szCommand);
		}

		//Create the command and set the command text
		SQLRETURN Create(_In_ CConnection& connection, _In_z_ SQLTCHAR* szCommand, _In_opt_ SQL_ATTRIBUTE* pAttributes = nullptr, _In_ ULONG nAttributes = 0) throw()
		{
			SQLRETURN nRet = CreateCommand(connection);
			if (SQL_SUCCEEDED(nRet))
			{
				nRet = SetAttributes(pAttributes, nAttributes);
				if (SQL_SUCCEEDED(nRet))
					nRet = m_Command.Prepare(szCommand);
			}

			return nRet;
		}

		//Release the command
		void ReleaseCommand() throw()
		{
			m_Command.Close();
		}

		//Member variables
		CStatement m_Command;
	};


	//Class modelled on ATL::CCommand which encapsulates command processing
	template <class TAccessor>
	class CCommand :
		public CAccessor<TAccessor>,
		public CCommandBase
	{
	public:
		SQLRETURN Open(_In_ CConnection& connection, _In_z_ SQLTCHAR* szCommand, _In_ bool bBind = true, _In_opt_ SQL_ATTRIBUTE* pAttributes = nullptr, _In_ ULONG nAttributes = 0) throw()
		{
			SQLRETURN nRet = Create(connection, szCommand, pAttributes, nAttributes);
			if (!SQL_SUCCEEDED(nRet))
				return nRet;

			return Open(bBind);
		}

		//Used if you have previously created the command
		SQLRETURN Open(_In_ bool bBind = true) throw()
		{
			//Bind the parameters in the accessor if they haven't already been bound
			SQLRETURN nRet = this->BindParameters(m_Command);
			if (!SQL_SUCCEEDED(nRet))
				return nRet;

			return ExecuteAndBind(bBind);
		}

		//Implementation
		SQLRETURN ExecuteAndBind(_In_ bool bBind = true) throw()
		{
			SQLRETURN nRet = Execute();
			if (!SQL_SUCCEEDED(nRet))
				return nRet;

			if (bBind)
				return this->BindColumns(m_Command);
			else
				return nRet;
		}

		SQLRETURN Execute() throw()
		{
			return m_Command.Execute();
		}
	};


	//An accessor where the column data is bound dynamically
	template<class T>
	class CDynamicColumnAccessor : public CAccessor<T>
	{
	public:
		//Constructors / Destructors
		CDynamicColumnAccessor() : m_nColumns(0)
		{
		}

		// Methods
#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		SQLRETURN BindColumns(_In_ CStatement& statement)
		{
			//Work out how many columns there are in the recordset
			m_nColumns = 0;
			SQLRETURN nRet = statement.NumResultCols(&m_nColumns);
			ODBC_CHECK_RETURN(nRet, statement);

			//Allocate the column indicators array
			this->m_ColumnIndicators.SetSize(m_nColumns);

			//Also get the column details
			m_ColumnNames.SetSize(0, m_nColumns);
			m_DataTypes.SetSize(0, m_nColumns);
			m_ColumnSizes.SetSize(0, m_nColumns);
			m_DecimalDigits.SetSize(0, m_nColumns);
			m_Nullables.SetSize(0, m_nColumns);
			for (SQLSMALLINT i = 1; i <= m_nColumns; i++)
			{
				String sColumnName;
				SQLSMALLINT nDataType = 0;
				SQLULEN nColumnSize = 0;
				SQLSMALLINT nDecimalDigits = 0;
				SQLSMALLINT nNullable = 0;
				nRet = statement.DescribeCol(i, sColumnName, &nDataType, &nColumnSize, &nDecimalDigits, &nNullable);
				ODBC_CHECK_RETURN(nRet, statement);
				m_ColumnNames.Add(sColumnName);
#pragma warning(suppress: 6102)
				m_DataTypes.Add(nDataType);
				m_ColumnSizes.Add(nColumnSize);
				m_DecimalDigits.Add(nDecimalDigits);
				m_Nullables.Add(nNullable);
			}

			ASSERT(static_cast<SQLSMALLINT>(this->m_ColumnIndicators.GetSize()) == m_nColumns);
			ASSERT(static_cast<SQLSMALLINT>(m_ColumnNames.GetSize()) == m_nColumns);
			ASSERT(static_cast<SQLSMALLINT>(m_DataTypes.GetSize()) == m_nColumns);
			ASSERT(static_cast<SQLSMALLINT>(m_ColumnSizes.GetSize()) == m_nColumns);
			ASSERT(static_cast<SQLSMALLINT>(m_DecimalDigits.GetSize()) == m_nColumns);
			ASSERT(static_cast<SQLSMALLINT>(m_Nullables.GetSize()) == m_nColumns);

			return nRet;
		}
#else
		SQLRETURN BindColumns(_In_ CStatement& statement)
		{
			//Work out how many columns there are in the recordset
			m_nColumns = 0;
			SQLRETURN nRet = statement.NumResultCols(&m_nColumns);
			ODBC_CHECK_RETURN(nRet, statement);

			//Allocate the column indicators array
			this->m_ColumnIndicators.clear();
			this->m_ColumnIndicators.insert(this->m_ColumnIndicators.end(), m_nColumns, 0);

			//Also get the column details
			m_ColumnNames.reserve(m_nColumns);
			m_DataTypes.reserve(m_nColumns);
			m_ColumnSizes.reserve(m_nColumns);
			m_DecimalDigits.reserve(m_nColumns);
			m_Nullables.reserve(m_nColumns);
			for (SQLSMALLINT i = 1; i <= m_nColumns; i++)
			{
				String sColumnName;
				SQLSMALLINT nDataType = 0;
				SQLULEN nColumnSize = 0;
				SQLSMALLINT nDecimalDigits = 0;
				SQLSMALLINT nNullable = 0;
				nRet = statement.DescribeCol(i, sColumnName, &nDataType, &nColumnSize, &nDecimalDigits, &nNullable);
				ODBC_CHECK_RETURN(nRet, statement);
				m_ColumnNames.push_back(sColumnName);
				m_DataTypes.push_back(nDataType);
				m_ColumnSizes.push_back(nColumnSize);
				m_DecimalDigits.push_back(nDecimalDigits);
				m_Nullables.push_back(nNullable);
			}

			ATLASSERT(static_cast<SQLSMALLINT>(this->m_ColumnIndicators.size()) == m_nColumns);
			ATLASSERT(static_cast<SQLSMALLINT>(m_ColumnNames.size()) == m_nColumns);
			ATLASSERT(static_cast<SQLSMALLINT>(m_DataTypes.size()) == m_nColumns);
			ATLASSERT(static_cast<SQLSMALLINT>(m_ColumnSizes.size()) == m_nColumns);
			ATLASSERT(static_cast<SQLSMALLINT>(m_DecimalDigits.size()) == m_nColumns);
			ATLASSERT(static_cast<SQLSMALLINT>(m_Nullables.size()) == m_nColumns);

			return nRet;
		}
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS

		String GetColumnName(_In_ SQLSMALLINT nColumn) const
		{
			return m_ColumnNames[nColumn - 1]; //nColumn is 1 based
		}

		SQLSMALLINT GetColumnType(_In_ SQLSMALLINT nColumn) const
		{
			return m_ColumnDataTypes[nColumn - 1]; //nColumn is 1 based
		}

		SQLULEN GetColumnSize(_In_ SQLSMALLINT nColumn) const
		{
			return m_ColumnSizes[nColumn - 1]; //nColumn is 1 based
		}

		SQLSMALLINT GetColumnDecimalDigits(_In_ SQLSMALLINT nColumn) const
		{
			return m_ColumnDecimalDigits[nColumn - 1]; //nColumn is 1 based
		}

		SQLSMALLINT GetColumnNullables(_In_ SQLSMALLINT nColumn) const
		{
			return m_ColumnNullables[nColumn - 1]; //nColumn is 1 based
		}

		SQLSMALLINT GetColumnNo(_In_z_ LPCTSTR pszColumnName) const
		{
			//What will be the return value from this function
			SQLSMALLINT nColumn = 0;

			for (SQLSMALLINT i = 0; (i < m_nColumns) && (nColumn == 0); i++)
			{
				if (m_ColumnNames[i] == pszColumnName)
					nColumn = static_cast<SQLSMALLINT>(i + 1); //Columns are one based
			}

			return nColumn;
		}

		template <class ctype>
		SQLRETURN GetValue(_In_ CStatement& statement, _In_ SQLSMALLINT nColumn, ctype& data, _Out_opt_ SQLLEN* StrLen_or_IndPtr = nullptr)
		{
			SQLLEN* pIndicator = StrLen_or_IndPtr;
			if (pIndicator == nullptr)
			{
				SQLLEN& nColumnIndicator = this->m_ColumnIndicators[nColumn - 1];
				pIndicator = &nColumnIndicator;
			}
			return statement.GetData(nColumn, _GetODBCCType(data), &data, sizeof(ctype), pIndicator);
		}

		//Member variables
		SQLSMALLINT                       m_nColumns;
#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
		CArray<String, String&>           m_ColumnNames;
		CArray<SQLSMALLINT, SQLSMALLINT>  m_DataTypes;
		CArray<SQLULEN, SQLULEN>          m_ColumnSizes;
		CArray<SQLSMALLINT, SQLSMALLINT>  m_DecimalDigits;
		CArray<SQLSMALLINT, SQLSMALLINT>  m_Nullables;
#else
		std::vector<String>               m_ColumnNames;
		std::vector<SQLSMALLINT>          m_DataTypes;
		std::vector<SQLULEN>              m_ColumnSizes;
		std::vector<SQLSMALLINT>          m_DecimalDigits;
		std::vector<SQLSMALLINT>          m_Nullables;
#endif //#ifdef CODBCWRAPPERS_MFC_EXTENSIONS
	};


}; //namespace COLEDB


#endif //__ODBCWRAPPERS_H__