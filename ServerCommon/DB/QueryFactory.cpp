#include<strsafe.h>
#include"QueryFactory.h"
#include "Logger.h"
#include "mysql.h"
#include"Assert.h"
#pragma comment(lib,"LoggerMT.lib")

__forceinline bool ASSERT_INSUF_BUFFER(QueryFactory* pQF, QueryFactory::BufInfo* pBufInfo, char* end , HRESULT hr)
{
	if (FAILED(hr))
	{
		pQF->Resize(pBufInfo, end);
		return true;
	}
	else
	{
		pBufInfo->nextPos = end;
	}
	return false;
}

// size�� nts �����̹Ƿ�, buf + size�� nts �ٷ� ���������� �ּ���
// nextPost�� �������� ���� ������ �ּ�, ���� �� ����� nts
#define GetRemainSize(pBufInfo) ((pBufInfo->pBuf + pBufInfo->size) - pBI->nextPos)

#define ConclusionQueryPart(hr,pBI,ppszDestEnd,bEnd) do\
{\
if(bEnd)\
{\
	STRING_CCH_PRINTF_EXA(hr,pBI,ppszDestEnd,";");\
}\
else\
{\
	STRING_CCH_PRINTF_EXA(hr,pBI,ppszDestEnd," ");\
}\
}while(ASSERT_INSUF_BUFFER(this,pBI,*ppszDestEnd,hr))\


#define STRING_CCH_PRINTF_EXA(hr,pBI,ppszDestEnd,pszFormat,...) do{\
hr = StringCchPrintfExA(pBI->nextPos,GetRemainSize(pBI),ppszDestEnd,NULL,0,pszFormat,__VA_ARGS__);\
}while(ASSERT_INSUF_BUFFER(this,pBI,*ppszDestEnd,hr))\

#define STRING_CCH_VPRINTF_EXA(hr,pBI,ppszDestEnd,pszFormat,va)do{\
hr = StringCchVPrintfExA(pBI->nextPos,GetRemainSize(pBI),ppszDestEnd,NULL,0,pszFormat,va);\
}while(ASSERT_INSUF_BUFFER(this,pBI,*ppszDestEnd,hr))\

#define MakeVariadicPart(hr,pBI,ppszDestEnd,formatStr) do\
{\
va_list va;\
va_start(va, formatStr);\
STRING_CCH_VPRINTF_EXA(hr,pBI,ppszDestEnd,formatStr,va);\
va_end(va);\
}while(0)\

QueryFactory::QueryFactory()
{
}

QueryFactory::BufInfo* QueryFactory::GetBufInfo()
{
	thread_local BufInfo bi;
	return &bi;
}

void QueryFactory::Resize(BufInfo* pBI,char* end)
{
	int newSize = 2 * pBI->size;
	char* pTemp = new char[newSize];
	memcpy(pTemp, pBI->pBuf, pBI->size);

	pBI->size = newSize;
	pBI->nextPos = pTemp + (pBI->nextPos - pBI->pBuf);
	delete[] pBI->pBuf;
	pBI->pBuf = pTemp;
	LOG(L"LogBufResize", SYSTEM, TEXTFILE, L"Thread ID : %u BufSize %d -> %d", GetCurrentThreadId(), newSize / 2, newSize);
}


MYSQL_RES_PTR QueryFactory::ExecuteReadQuery()
{
	BufInfo* pBI = GetBufInfo();
	ASSERT_QUERY(mysql_query(pBI->connection, pBI->pBuf));
	pBI->Clear();
	return MYSQL_RES_PTR{ mysql_store_result(pBI->connection),mysql_free_result };
}

int QueryFactory::ExcuteWriteQuery()
{
	BufInfo* pBI = GetBufInfo();
	int ret = mysql_query(pBI->connection, pBI->pBuf);
	if (ret != 0)
	{
		pBI->Clear();
		return mysql_errno(pBI->connection);
	}
	pBI->Clear();
	return ret;
}

const char* QueryFactory::MAKE_QUERY(const char* pStr, ...)
{
	BufInfo* pBI = GetBufInfo();
	HRESULT hr;
	char* end;
	MakeVariadicPart(hr, pBI, &end, pStr);
	ConclusionQueryPart(hr, pBI, &end, pStr);
	return pBI->pBuf;
}


const char* QueryFactory::MAKE_SELECT_FROM(const char* pColumnList, const char* pTableName, const char* pOptConditionFormatStr, ...)
{
	char* end;
	BufInfo* pBI = GetBufInfo();

	HRESULT hr;
	STRING_CCH_PRINTF_EXA(hr, pBI, &end, "SELECT %s FROM %s", pColumnList, pTableName);
	if (pOptConditionFormatStr)
	{
		STRING_CCH_PRINTF_EXA(hr, pBI, &end, " WHERE ");
		MakeVariadicPart(hr, pBI, &end, pOptConditionFormatStr);
	}
	ConclusionQueryPart(hr, pBI, &end, true);
	return pBI->pBuf;
}


const char* QueryFactory::MAKE_UPDATE_SET(bool bEnd, const char* pTableName, const char* pSetFormatString, ...)
{
	char* end;
	BufInfo* pBI = GetBufInfo();
	HRESULT hr;
	STRING_CCH_PRINTF_EXA(hr, pBI, &end, "UPDATE %s SET ", pTableName);
	MakeVariadicPart(hr, pBI, &end, pSetFormatString);
	ConclusionQueryPart(hr, pBI, &end, bEnd);
	return pBI->pBuf;
}

const char* QueryFactory::MAKE_WHERE(bool bEnd, const char* pWhereFormatStr, ...)
{
	char* end;
	BufInfo* pBI = GetBufInfo();

	// ������ġ �ʱ�ȭ
	HRESULT hr;
	STRING_CCH_PRINTF_EXA(hr, pBI, &end, "WHERE ");
	MakeVariadicPart(hr, pBI, &end, pWhereFormatStr);
	ConclusionQueryPart(hr, pBI, &end, bEnd);
	return pBI->pBuf;
}

const char* QueryFactory::MAKE_INSERT_INTO(bool bEnd, const char* pTableName, const char* pColumnList, const char* pColumnFormatStr, ...)
{
	char* end;
	BufInfo* pBI = GetBufInfo();
	HRESULT hr;
	STRING_CCH_PRINTF_EXA(hr, pBI, &end, "INSERT INTO %s (%s) VALUES (", pTableName, pColumnList);
	MakeVariadicPart(hr, pBI, &end, pColumnFormatStr);
	STRING_CCH_PRINTF_EXA(hr, pBI, &end, ")");
	ConclusionQueryPart(hr, pBI, &end, bEnd);
	return pBI->pBuf;
}

void QueryFactory::WriteMutiQueryFreeResult()
{
	BufInfo* pBI = GetBufInfo();
	MYSQL_RES* res = mysql_store_result(pBI->connection);
	mysql_free_result(res);
}

QueryFactory::BufInfo::BufInfo()
{
	nextPos = pBuf = new char[INITIAL_SIZE];
	size = INITIAL_SIZE;

	EnterCriticalSection(&connCS);
	mysql_init(&conn);
	connection = mysql_real_connect(&conn, "localhost", "root", "vector0812@!", "logdb", 3306, (char*)NULL, 0);
	LeaveCriticalSection(&connCS);

	if (connection == NULL)
	{
		LOG(L"ERROR", SYSTEM, TEXTFILE, L"Mysql connection error : %d", mysql_errno(connection));
		__debugbreak();
	}

	if (0 != mysql_set_server_option(connection, MYSQL_OPTION_MULTI_STATEMENTS_ON))
	{
		LOG(L"ERROR", SYSTEM, TEXTFILE, L"Mysql Multiple Statement Failed : %s", mysql_error(connection));
		__debugbreak();
	}
}

QueryFactory::BufInfo::~BufInfo()
{
	EnterCriticalSection(&connCS);
	mysql_close(connection);
	LeaveCriticalSection(&connCS);
	delete[] pBuf;
}
