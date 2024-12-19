#include <WinSock2.h>
#include "MYOVERLAPPED.h"
#include "DBWriteThreadBase.h"
#include "Timer.h"

MYOVERLAPPED DBWriteThreadBase::DBWriteOverlapped{ WSAOVERLAPPED{},OVERLAPPED_REASON::DB_WRITE };


DBWriteThreadBase::DBWriteThreadBase(DWORD timeOutInterval, HANDLE hCompletionPort, LONG timeOutPqcsLimit)
	:UpdateBase{ timeOutInterval,hCompletionPort,timeOutPqcsLimit }, bAlreadyWorking_{ FALSE }
{
}

void DBWriteThreadBase::RegisterTimeOut()
{
	Timer::Reigster_UPDATE(static_cast<UpdateBase*>(this));
}

void DBWriteThreadBase::ProcessDBWrite()
{
	while (1)
	{
		auto&& opt = jobQ_.Dequeue();
		if (!opt.has_value())
			break;

		Packet* pPacket = opt.value();
		OnWrite(pPacket);
		jobPool_.Free(pPacket);
	}
	InterlockedExchange((LONG*)&bAlreadyWorking_, FALSE);
}

void DBWriteThreadBase::ReqDBWriteJob(Packet* pPacket)
{
	jobQ_.Enqueue(pPacket);
	if (InterlockedExchange((LONG*)&bAlreadyWorking_, TRUE) == TRUE)
		return;

	PostQueuedCompletionStatus(hcp_, 2, (ULONG_PTR)pPacket, (LPOVERLAPPED)&DBWriteThreadBase::DBWriteOverlapped);
}
