#pragma once
#include "UpdateBase.h"
#include "MYOVERLAPPED.h"

namespace Timer
{
	void Reigster_UPDATE(UpdateBase* pUpdate);
	void Init();
	void Release_TimerThread();
	void Start();
	const MYOVERLAPPED* GetUpdateOverlapped();
}
