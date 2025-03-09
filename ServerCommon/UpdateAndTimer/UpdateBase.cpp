#include <WinSock2.h>
#include <windows.h>
#include "Scheduler.h"
#include "UpdateBase.h"
#include "MYOVERLAPPED.h"

MYOVERLAPPED updatePostOverlapped = { WSAOVERLAPPED{},OVERLAPPED_REASON::POST };

#pragma warning(disable : 26495)
UpdateBase::UpdateBase(DWORD tickPerFrame, HANDLE hCompletionPort, LONG pqcsLimit)
	:TICK_PER_FRAME_{ tickPerFrame }, singleThreadGate_{ 0 }, fps_{ 0 }, unProcessedPqcsCnt_{ 0 },
	hcp_{ hCompletionPort }, AllowedUnProcessedPqcsLimit{ pqcsLimit } 
{
}
#pragma warning(default : 26495)

UpdateBase::~UpdateBase()
{
}

void UpdateBase::firstTimeInit()
{
	firstTimeCheck_ = timeGetTime();
	oldFrameTick_ = firstTimeCheck_;
	timeStamp_ = firstTimeCheck_;
}

void UpdateBase::Update()
{
	if (InterlockedIncrement(&singleThreadGate_) - 1 > 0)
	{
		InterlockedDecrement(&unProcessedPqcsCnt_);
		return;
	}

	if (bFirst_ == false)
	{
		firstTimeInit();
		bFirst_ = true;
	}

	while (1)
	{
		Update_IMPL(); // �Ļ�Ŭ�������� �ش� �����Լ��� �������̵��ϸ� UPDATE_IMPL���� ������� �ϳ��� �����忡 ���ؼ� ����ó������ ������.
		InterlockedIncrement(&fps_);
		timeStamp_ = timeGetTime();
		if (timeStamp_ > oldFrameTick_ + TICK_PER_FRAME_)  
		{
			oldFrameTick_ = timeStamp_ - ((timeStamp_ - firstTimeCheck_) % TICK_PER_FRAME_); // �������� �з�����
			InterlockedExchange(&singleThreadGate_, 1);
			continue;
		}

		oldFrameTick_ += TICK_PER_FRAME_;
		if (InterlockedDecrement(&singleThreadGate_) > 0)
		{
			InterlockedExchange(&singleThreadGate_, 1);
			continue;
		}

		InterlockedDecrement(&unProcessedPqcsCnt_);
		break;
	}
}



#pragma warning(disable : 26495)
MonitoringUpdate::MonitoringUpdate(HANDLE hCompletionPort, DWORD tickPerFrame, LONG pqcsLimit)
	:UpdateBase{ tickPerFrame,hCompletionPort,pqcsLimit }
{}
#pragma warning(default : 26495)

void MonitoringUpdate::RegisterMonitor(const Monitorable* pTargetToMonitor)
{
	if (curNum_ >= len)	__debugbreak();
	pArr_[curNum_++] = const_cast<Monitorable*>(pTargetToMonitor);
}

void MonitoringUpdate::Update_IMPL()
{
	printf("--------------------------\n");
	for (int i = 0; i < curNum_; ++i)
	{
		pArr_[i]->OnMonitor();
	}
	printf("--------------------------\n\n");
}


