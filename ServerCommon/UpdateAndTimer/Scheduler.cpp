#include <WinSock2.h>
#include <windows.h>
#include "Scheduler.h"
#include "Logger.h"
#include "Assert.h"
#include "process.h"

#include "UpdateBase.h"
#pragma comment(lib,"LoggerMt.lib")

namespace Scheduler
{
	const MYOVERLAPPED TimerPostOverlapped{ OVERLAPPED{},OVERLAPPED_REASON::UPDATE };
	HANDLE hThread;
	UpdatePQCSInfo uPI;
}

unsigned __stdcall Scheduler::threadFunc(void* pParam)
{
	int curNum = uPI.currentNum_;
	UpdateBase** ppUpdateArr = uPI.pUpdateArr_;
	int arr[UpdatePQCSInfo::len];

	uPI.firstTimeInit(timeGetTime());

	// ù PQCS
	for (int i = 0; i < curNum; ++i)
	{
		UpdateBase* pBase = ppUpdateArr[i];
		InterlockedIncrement(&pBase->unProcessedPqcsCnt_);
		PostQueuedCompletionStatus(pBase->hcp_, 2, (ULONG_PTR)pBase, (LPOVERLAPPED)&TimerPostOverlapped);
	}

	int min;
	{
		// ���� ���� �������� ���ã��
		DWORD tempTimeStamp = timeGetTime();
		for (int i = 0; i < curNum; ++i)
		{
			UpdateBase* pBase = ppUpdateArr[i];
			arr[i] = pBase->scdr.UpdateAndGetTimeToSleep(tempTimeStamp, pBase->TICK_PER_FRAME_);
		}

		min = *std::min_element(arr, arr + curNum);
		if (WaitForSingleObject(uPI.hTerminateEvent_, min) != WAIT_TIMEOUT)
			return 0;
	}

	while (1)
	{
		DWORD tempTime = timeGetTime();
		for (int i = 0; i < curNum; ++i)
		{
			arr[i] -= min;
			UpdateBase* pBase = ppUpdateArr[i];
			if (arr[i] <= 0) // �迭�� ��ϵ� ���ð����� ������ ���� ���ð��� �ּҰ��� ���� 0���ϰ� �Ǵ� ��Ҵ� ���� �����ٸ� �����
			{
				if (pBase->unProcessedPqcsCnt_ < pBase->AllowedUnProcessedPqcsLimit) // IOCP ť�� ���� ���� ��������ʾƼ� ó���������� �Ϸ���Ŷ�� ���� �������� �Ѿ�� PQCS�� ���� �ʴ´�
				{
					InterlockedIncrement(&pBase->unProcessedPqcsCnt_);
					PostQueuedCompletionStatus(pBase->hcp_, 2, (ULONG_PTR)pBase, (LPOVERLAPPED)&Scheduler::TimerPostOverlapped);
				}
				arr[i] = pBase->scdr.UpdateAndGetTimeToSleep(tempTime, pBase->TICK_PER_FRAME_); // ���� PQCS���� �����ð��� ����ؼ� �迭�� ����
			}
		}
		min = *std::min_element(arr, arr + curNum); // ���� PQCS�� ���ð��� ���� ª���ð��� ���ؼ� �׋����� ����
		if (WaitForSingleObject(uPI.hTerminateEvent_, min) != WAIT_TIMEOUT)
			break;
	}
    return 0;
}

void Scheduler::Register_UPDATE(UpdateBase* pUpdate)
{
	uPI.pUpdateArr_[uPI.currentNum_++] = pUpdate;
	if (uPI.currentNum_ >= UpdatePQCSInfo::len) __debugbreak();
}


void Scheduler::Init()
{
	hThread = (HANDLE)_beginthreadex(nullptr, 0, threadFunc, nullptr, CREATE_SUSPENDED, nullptr);
}

void Scheduler::Release_SchedulerThread()
{
	// Lan,Net ���������� ShutDown���� ����ȣ��� �ѹ��� ó���ǵ��� ����ó��
	// INVALID_HANDLE_VALUE�� ���� WaitFor�� �ϸ� ���Ѵ���ϴ� ������ �߰�
	if (hThread == INVALID_HANDLE_VALUE)
		return;
	SetEvent(uPI.hTerminateEvent_);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	hThread = INVALID_HANDLE_VALUE;
}


void Scheduler::Start()
{
	ResumeThread(hThread);
}

const MYOVERLAPPED* Scheduler::GetUpdateOverlapped()
{
	return &TimerPostOverlapped;
}

