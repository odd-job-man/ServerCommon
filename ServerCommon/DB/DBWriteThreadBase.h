#pragma once
#include <windows.h>
#include "CLockFreeQueue.h"
#include "CTlsObjectPool.h"
#include "Packet.h"
#include "UpdateBase.h"

struct MYOVERLAPPED;

class DBWriteThreadBase : public UpdateBase
{
public:
	DBWriteThreadBase(DWORD timeOutInterval, HANDLE hCompletionPort, LONG timeOutPqcsLimit);
	void RegisterTimeOut();
	void ProcessDBWrite();
	virtual void OnWrite(Packet* pPacket) = 0;

	__forceinline static Packet* ALLOC()
	{
		Packet* pRet = jobPool_.Alloc();
		pRet->Clear<Lan>();
		return pRet;
	}

	virtual void Update_IMPL() override 
	{
		if (InterlockedExchange((LONG*)&bAlreadyWorking_, TRUE) == TRUE)
			return;

		ProcessDBWrite(); 
	}

	void ReqDBWriteJob(Packet* pPacket);
	CLockFreeQueue<Packet*> jobQ_;
protected:
	BOOL bAlreadyWorking_;
	static inline CTlsObjectPool<Packet, false> jobPool_;
	static MYOVERLAPPED DBWriteOverlapped;
};
