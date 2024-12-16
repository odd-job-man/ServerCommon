#include<strsafe.h>
#include"QueryFactory.h"
#include "Logger.h"
#include"mysql.h"
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

// size는 nts 포함이므로, buf + size는 nts 바로 다음문자의 주소임
// nextPost는 다음번에 쓰기 시작할 주소, 현재 쓴 결과의 nts
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
	tlsIdx_ = TlsAlloc();
	TLS_ASSERT(tlsIdx_);
}

QueryFactory::BufInfo* QueryFactory::GetBufInfo()
{
	BufInfo* pBI = (BufInfo*)TlsGetValue(tlsIdx_);
	if (!pBI)
	{
		pBI = new BufInfo;
		pBI->nextPos = pBI->pBuf = new char[INITIAL_SIZE];
		pBI->size = INITIAL_SIZE;
		EnterCriticalSection(&connCS);
		mysql_init(&pBI->conn);
		pBI->connection = mysql_real_connect(&pBI->conn, "localhost", "root", "vector0812@!", "oddjobman", 3306, (char*)NULL, 0);
		LeaveCriticalSection(&connCS);
		ASSERT_MYSQL_CONNECTION(pBI->connection);
		ASSERT_MYSQL_MULTI_STATEMENT_ON(mysql_set_server_option(pBI->connection, MYSQL_OPTION_MULTI_STATEMENTS_ON));
		TlsSetValue(tlsIdx_, pBI);
	}
	return pBI;
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

void QueryFactory::ExcuteWriteQuery()
{
	BufInfo* pBI = GetBufInfo();
	ASSERT_QUERY(mysql_query(pBI->connection, pBI->pBuf));
	pBI->Clear();
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

	// 버퍼위치 초기화
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


