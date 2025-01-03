#include <WinSock2.h>
#include <windows.h>
#include "Timer.h"
#include "Logger.h"
#include "Assert.h"
#include "process.h"

#include "UpdateBase.h"
#pragma comment(lib,"LoggerMt.lib")

namespace Timer
{
	const MYOVERLAPPED TimerPostOverlapped{ OVERLAPPED{},OVERLAPPED_REASON::UPDATE };
	HANDLE hThread;
    unsigned __stdcall threadFunc(void* pParam);
	UpdatePQCSInfo uPI;
}

unsigned __stdcall Timer::threadFunc(void* pParam)
{
	int curNum = uPI.currentNum_;
	UpdateBase** ppUpdateArr = uPI.pUpdateArr_;
	int arr[UpdatePQCSInfo::len];

	uPI.firstTimeInit(timeGetTime());

	// 첫 PQCS
	for (int i = 0; i < curNum; ++i)
	{
		UpdateBase* pBase = ppUpdateArr[i];
		InterlockedIncrement(&pBase->pqcsUpdateCnt_);
		PostQueuedCompletionStatus(pBase->hcp_, 2, (ULONG_PTR)pBase, (LPOVERLAPPED)&TimerPostOverlapped);
	}

	// 가장 먼저 깨워야할 사람찾기
	DWORD tempTimeStamp = timeGetTime();
	for (int i = 0; i < curNum; ++i)
	{
		UpdateBase* pBase = ppUpdateArr[i];
		arr[i] = pBase->scdr.UpdateAndGetTimeToSleep(tempTimeStamp, pBase->TICK_PER_FRAME_);
	}

	int min = *std::min_element(arr, arr + curNum);
	if (WaitForSingleObject(uPI.hTerminateEvent_, min) != WAIT_TIMEOUT)
		return 0;

	while (1)
	{
		DWORD tempTime = timeGetTime();
		for (int i = 0; i < curNum; ++i)
		{
			arr[i] -= min;
			UpdateBase* pBase = ppUpdateArr[i];
			if (arr[i] <= 0)
			{
				if (pBase->pqcsUpdateCnt_ < pBase->pqcsLimit_)
				{
					InterlockedIncrement(&pBase->pqcsUpdateCnt_);
					PostQueuedCompletionStatus(pBase->hcp_, 2, (ULONG_PTR)pBase, (LPOVERLAPPED)&Timer::TimerPostOverlapped);
				}
				arr[i] = pBase->scdr.UpdateAndGetTimeToSleep(tempTime, pBase->TICK_PER_FRAME_);
			}
		}
		min = *std::min_element(arr, arr + curNum);
		if (WaitForSingleObject(uPI.hTerminateEvent_, min) != WAIT_TIMEOUT)
			break;
	}
    return 0;
}

// 최대 15개
void Timer::Reigster_UPDATE(UpdateBase* pUpdate)
{
	uPI.pUpdateArr_[uPI.currentNum_++] = pUpdate;
	if (uPI.currentNum_ >= UpdatePQCSInfo::len) __debugbreak();
}


void Timer::Init()
{
	hThread = (HANDLE)_beginthreadex(nullptr, 0, threadFunc, nullptr, CREATE_SUSPENDED, nullptr);
}

void Timer::Release_TimerThread()
{
	// Lan,Net 서버에서의 ShutDown에서 각각호출시 한번만 처리되도록 예외처리
	// INVALID_HANDLE_VALUE에 대해 WaitFor을 하면 무한대기하는 현상을 발견
	if (hThread == INVALID_HANDLE_VALUE)
		return;
	SetEvent(uPI.hTerminateEvent_);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	hThread = INVALID_HANDLE_VALUE;
}


void Timer::Start()
{
	ResumeThread(hThread);
}

const MYOVERLAPPED* Timer::GetUpdateOverlapped()
{
	return &TimerPostOverlapped;
}

