#pragma once
#include <windows.h>
#include <mutex>
#include "mysql.h"

using MYSQL_RES_DELETER = void(_stdcall*)(MYSQL_RES*);
using MYSQL_RES_PTR = std::unique_ptr<MYSQL_RES, MYSQL_RES_DELETER>;

class QueryFactory
{
public:
	struct BufInfo
	{
		char* pBuf;
		char* nextPos;
		int size;
		MYSQL* connection;
		MYSQL conn;
		__forceinline void Clear()
		{
			this->nextPos = this->pBuf;
		}
	};
	static constexpr int INITIAL_SIZE = 500;
	DWORD tlsIdx_;
	BufInfo* GetBufInfo();
private:
	QueryFactory();
	void Resize(BufInfo* pBI, char* end);
	static inline std::once_flag flag;
public:
	MYSQL_RES_PTR ExecuteReadQuery();
	void ExcuteWriteQuery();
	const char* MAKE_QUERY(const char* pStr, ...);
	const char* MAKE_SELECT_FROM(const char* pColumnList, const char* pTableName, const char* pOptConditionFormatStr, ...);
	const char* MAKE_UPDATE_SET(bool bEnd, const char* pTableName, const char* pSetFormatString, ...);
	const char* MAKE_WHERE(bool bEnd, const char* pWhereFormatStr, ...);
	const char* MAKE_INSERT_INTO(bool bEnd, const char* pTableName, const char* pColumnList, const char* pColumnFormatStr, ...);

	__forceinline void Clear()
	{
		BufInfo* pBI = GetBufInfo();
		pBI->nextPos = pBI->pBuf;
	}

	static inline QueryFactory* pInstance;
	static QueryFactory* GetInstance()
	{
		std::call_once(QueryFactory::flag, []() {
			InitializeCriticalSection(&connCS);
			pInstance = new QueryFactory;
		});
		return pInstance;
	};

	static inline CRITICAL_SECTION connCS;
	friend bool ASSERT_INSUF_BUFFER(QueryFactory* pQF, QueryFactory::BufInfo* pBufInfo, char* end, HRESULT hr);
};
